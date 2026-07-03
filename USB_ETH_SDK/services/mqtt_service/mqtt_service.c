/* SPDX-License-Identifier: MIT */

/*
 * MQTT service - thin wrapper around lwIP's raw mqtt client
 * (lwip/apps/mqtt.h). Presents a blocking-style API (connect/publish/
 * subscribe return only once the operation completes or times out)
 * safe to call from any task.
 *
 * Threading model: this project sets LWIP_TCPIP_CORE_LOCKING=1, so
 * raw lwIP core calls (dns_gethostbyname, mqtt_client_connect,
 * mqtt_publish, mqtt_sub_unsub, mqtt_disconnect) made from an
 * application task are wrapped in LOCK_TCPIP_CORE()/UNLOCK_TCPIP_CORE().
 * Callbacks invoked BY lwIP itself (mqtt_connection_cb, dns_found_cb,
 * request/incoming callbacks) already run inside tcpip_thread, which
 * already holds that lock - they must NOT lock again (non-recursive
 * mutex, would deadlock).
 */

#include "mqtt_service.h"

#include <string.h>

#include "lwip/apps/mqtt.h"
#include "lwip/dns.h"
#include "lwip/tcpip.h"
#include "lwip/ip_addr.h"
#if LWIP_ALTCP && LWIP_ALTCP_TLS
#include "lwip/altcp_tls.h"
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "semphr.h"

#include "logger.h"

static const char *TAG = "mqtt-sv";

#define MQTT_CONNECT_SUCCESS_BIT    (1 << 0)
#define MQTT_CONNECT_FAIL_BIT       (1 << 1)
#define MQTT_PUBLISH_SUCCESS_BIT    (1 << 2)
#define MQTT_PUBLISH_FAIL_BIT       (1 << 3)
#define MQTT_SUBSCRIBE_SUCCESS_BIT  (1 << 4)
#define MQTT_SUBSCRIBE_FAIL_BIT     (1 << 5)
#define MQTT_DNS_SUCCESS_BIT        (1 << 6)
#define MQTT_DNS_FAIL_BIT           (1 << 7)

#define MQTT_OP_TIMEOUT_MS    5000
#define MQTT_DNS_TIMEOUT_MS   60000
#define MQTT_TOPIC_BUF_LEN    64

typedef struct MQTT_Service {
    MQTTConfig_t *config;
    struct mqtt_connect_client_info_t ci;
    mqtt_client_t *client;
    EventGroupHandle_t event;
    SemaphoreHandle_t lock;    /* serializes connect/publish/subscribe/disconnect calls */
    ip_addr_t ip;

    /* topic of the publish currently being delivered, copied out of
     * the publish callback (see mqtt_incoming_publish_cb - the
     * original const char *topic is only valid for the duration of
     * that callback, it cannot be stored as a raw pointer). */
    char topic_buf[MQTT_TOPIC_BUF_LEN];
    size_t topic_len;
} MQTT_Service_t;

#define MQTT_LOCK(mqtt)     xSemaphoreTake((mqtt)->lock, portMAX_DELAY)
#define MQTT_UNLOCK(mqtt)   xSemaphoreGive((mqtt)->lock)

static err_t mqtt_do_connect(MQTT_Service_t *mqtt);
static bool mqtt_config_tls(MQTT_Service_t *mqtt);

/* -------------------------------------------------------------------- */
/* Callbacks invoked BY lwIP - already running inside tcpip_thread,     */
/* must not LOCK_TCPIP_CORE() again here.                               */
/* -------------------------------------------------------------------- */

static void dns_found_cb(const char *name, const ip_addr_t *ipaddr, void *arg)
{
    MQTT_Service_t *mqtt = (MQTT_Service_t *)arg;

    if (ipaddr != NULL) {
        LOGD(TAG, "resolved %s -> %s", name, ipaddr_ntoa(ipaddr));
        mqtt->ip = *ipaddr;
        xEventGroupSetBits(mqtt->event, MQTT_DNS_SUCCESS_BIT);
    } else {
        LOGE(TAG, "failed to resolve host %s", name);
        xEventGroupSetBits(mqtt->event, MQTT_DNS_FAIL_BIT);
    }
}

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    MQTT_Service_t *mqtt = (MQTT_Service_t *)arg;
    (void)client;

    if (status == MQTT_CONNECT_ACCEPTED) {
        LOGI(TAG, "connected to broker");
        xEventGroupSetBits(mqtt->event, MQTT_CONNECT_SUCCESS_BIT);
    } else {
        LOGE(TAG, "connection failed/closed, status=%d", (int)status);
        xEventGroupSetBits(mqtt->event, MQTT_CONNECT_FAIL_BIT);
    }
}

static void mqtt_sub_request_cb(void *arg, err_t result)
{
    MQTT_Service_t *mqtt = (MQTT_Service_t *)arg;

    LOGD(TAG, "subscribe result: %d", result);
    xEventGroupSetBits(mqtt->event, (result == ERR_OK) ? MQTT_SUBSCRIBE_SUCCESS_BIT : MQTT_SUBSCRIBE_FAIL_BIT);
}

static void mqtt_pub_request_cb(void *arg, err_t result)
{
    MQTT_Service_t *mqtt = (MQTT_Service_t *)arg;

    LOGD(TAG, "publish result: %d", result);
    xEventGroupSetBits(mqtt->event, (result == ERR_OK) ? MQTT_PUBLISH_SUCCESS_BIT : MQTT_PUBLISH_FAIL_BIT);
}

static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
    MQTT_Service_t *mqtt = (MQTT_Service_t *)arg;
    size_t len = strlen(topic); /* NOT tot_len - tot_len is the payload length, see mqtt.h */

    if (len >= sizeof(mqtt->topic_buf)) {
        LOGW(TAG, "topic too long (%u bytes), truncated", (unsigned)len);
        len = sizeof(mqtt->topic_buf) - 1;
    }

    /* topic is only valid during this callback - copy it out now,
     * mqtt_incoming_data_cb (called after this returns) cannot use
     * the original pointer. */
    memcpy(mqtt->topic_buf, topic, len);
    mqtt->topic_buf[len] = '\0';
    mqtt->topic_len = len;

    LOGD(TAG, "incoming publish: topic=%s, payload_len=%lu", mqtt->topic_buf, (unsigned long)tot_len);
}

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    MQTT_Service_t *mqtt = (MQTT_Service_t *)arg;

    (void)flags;

    if (data == NULL) {
        /* end-of-message marker (see mqtt.h: "NULL is passed when all
         * publish data are delivered") - nothing to forward. */
        return;
    }

    mqtt_sv_data_incoming(mqtt, mqtt->topic_buf, mqtt->topic_len, data, len);
}

/* -------------------------------------------------------------------- */
/* TLS config (not wired up yet - config->tls is accepted but ignored   */
/* until LWIP_ALTCP/LWIP_ALTCP_TLS + mbedtls integration is done as a   */
/* separate step)                                                       */
/* -------------------------------------------------------------------- */

#if LWIP_ALTCP && LWIP_ALTCP_TLS
static bool mqtt_config_tls(MQTT_Service_t *mqtt)
{
    if (mqtt->config->tls) {
        mqtt->ci.tls_config = altcp_tls_create_config_client_2wayauth(
            mqtt->config->ca, mqtt->config->ca_len,
            mqtt->config->cli_key, mqtt->config->cli_key_len,
            NULL, 0,
            mqtt->config->cli_crt, mqtt->config->cli_crt_len);

        if (mqtt->ci.tls_config == NULL) {
            LOGE(TAG, "altcp_tls_create_config_client_2wayauth failed");
            return false;
        }
    } else {
        mqtt->ci.tls_config = NULL;
    }

    return true;
}
#else
static bool mqtt_config_tls(MQTT_Service_t *mqtt)
{
    if (mqtt->config->tls) {
        LOGW(TAG, "TLS requested but LWIP_ALTCP/LWIP_ALTCP_TLS not enabled - connecting plaintext");
    }

    return true;
}
#endif

/* -------------------------------------------------------------------- */
/* Connect - runs in the caller's task, locks around each raw call      */
/* -------------------------------------------------------------------- */

static err_t mqtt_do_connect(MQTT_Service_t *mqtt)
{
    err_t err;
    EventBits_t bits;

    xEventGroupClearBits(mqtt->event, MQTT_DNS_SUCCESS_BIT | MQTT_DNS_FAIL_BIT);

    LOCK_TCPIP_CORE();
    if (ipaddr_aton(mqtt->config->host, &mqtt->ip)) {
        UNLOCK_TCPIP_CORE();
        xEventGroupSetBits(mqtt->event, MQTT_DNS_SUCCESS_BIT);
    } else {
        err = dns_gethostbyname(mqtt->config->host, &mqtt->ip, dns_found_cb, mqtt);
        UNLOCK_TCPIP_CORE();

        if (err == ERR_OK) {
            /* address was already cached, mqtt->ip filled in synchronously */
            xEventGroupSetBits(mqtt->event, MQTT_DNS_SUCCESS_BIT);
        } else if (err != ERR_INPROGRESS) {
            LOGE(TAG, "dns_gethostbyname failed: %d", err);
            return err;
        }
        /* else ERR_INPROGRESS: wait below, dns_found_cb() sets the bit */
    }

    bits = xEventGroupWaitBits(mqtt->event,
                                MQTT_DNS_SUCCESS_BIT | MQTT_DNS_FAIL_BIT,
                                pdTRUE, pdFALSE, pdMS_TO_TICKS(MQTT_DNS_TIMEOUT_MS));
    if (!(bits & MQTT_DNS_SUCCESS_BIT)) {
        LOGE(TAG, "DNS resolve timed out/failed for host %s", mqtt->config->host);
        return ERR_TIMEOUT;
    }

    mqtt->ci.client_id = mqtt->config->client_id;
    mqtt->ci.client_user = mqtt->config->username;
    mqtt->ci.client_pass = mqtt->config->password;
    mqtt->ci.keep_alive = mqtt->config->keep_alive;

    if (!mqtt_config_tls(mqtt)) {
        return ERR_ARG;
    }

    xEventGroupClearBits(mqtt->event, MQTT_CONNECT_SUCCESS_BIT | MQTT_CONNECT_FAIL_BIT);

    LOCK_TCPIP_CORE();
    err = mqtt_client_connect(mqtt->client, &mqtt->ip, mqtt->config->port,
                               mqtt_connection_cb, mqtt, &mqtt->ci);
    UNLOCK_TCPIP_CORE();

    if (err != ERR_OK) {
        LOGE(TAG, "mqtt_client_connect failed: %d", err);
        return err;
    }

    /* mqtt_set_inpub_callback() only touches the client struct, but it
     * is still a core call - do it once, right after connect request,
     * under the same lock discipline. */
    LOCK_TCPIP_CORE();
    mqtt_set_inpub_callback(mqtt->client, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, mqtt);
    UNLOCK_TCPIP_CORE();

    bits = xEventGroupWaitBits(mqtt->event,
                                MQTT_CONNECT_SUCCESS_BIT | MQTT_CONNECT_FAIL_BIT,
                                pdTRUE, pdFALSE, pdMS_TO_TICKS(MQTT_OP_TIMEOUT_MS));
    if (!(bits & MQTT_CONNECT_SUCCESS_BIT)) {
        LOGE(TAG, "connect to %s:%u timed out/failed", mqtt->config->host, mqtt->config->port);
        return ERR_TIMEOUT;
    }

    return ERR_OK;
}

/* -------------------------------------------------------------------- */
/* Public API                                                            */
/* -------------------------------------------------------------------- */

MQTTHandle_t mqtt_sv_create(MQTTConfig_t *config)
{
    if (config == NULL) {
        LOGE(TAG, "config is NULL");
        return NULL;
    }

    MQTT_Service_t *mqtt = pvPortMalloc(sizeof(MQTT_Service_t));
    if (mqtt == NULL) {
        LOGE(TAG, "out of memory");
        return NULL;
    }
    memset(mqtt, 0, sizeof(*mqtt));
    mqtt->config = config;

    mqtt->client = mqtt_client_new();
    if (mqtt->client == NULL) {
        LOGE(TAG, "mqtt_client_new failed");
        vPortFree(mqtt);
        return NULL;
    }

    mqtt->event = xEventGroupCreate();
    if (mqtt->event == NULL) {
        LOGE(TAG, "xEventGroupCreate failed");
        mqtt_client_free(mqtt->client);
        vPortFree(mqtt);
        return NULL;
    }

    mqtt->lock = xSemaphoreCreateMutex();
    if (mqtt->lock == NULL) {
        LOGE(TAG, "xSemaphoreCreateMutex failed");
        vEventGroupDelete(mqtt->event);
        mqtt_client_free(mqtt->client);
        vPortFree(mqtt);
        return NULL;
    }

    memset(&mqtt->ci, 0, sizeof(mqtt->ci));

    LOGI(TAG, "mqtt service created for %s:%u", config->host, config->port);
    return (MQTTHandle_t)mqtt;
}

void mqtt_sv_free(MQTTHandle_t mqtt_handle)
{
    MQTT_Service_t *mqtt = (MQTT_Service_t *)mqtt_handle;

    if (mqtt == NULL) {
        return;
    }

    if (mqtt->client != NULL) {
        mqtt_client_free(mqtt->client);
    }
    if (mqtt->event != NULL) {
        vEventGroupDelete(mqtt->event);
    }
    if (mqtt->lock != NULL) {
        vSemaphoreDelete(mqtt->lock);
    }

    vPortFree(mqtt);
}

mqtt_err_t mqtt_sv_connect(MQTTHandle_t mqtt_handle)
{
    MQTT_Service_t *mqtt = (MQTT_Service_t *)mqtt_handle;
    mqtt_err_t ret;

    MQTT_LOCK(mqtt);
    ret = (mqtt_do_connect(mqtt) == ERR_OK) ? MQTT_SUCCESS : MQTT_FAIL;
    MQTT_UNLOCK(mqtt);

    return ret;
}

mqtt_err_t mqtt_sv_subscribe(MQTTHandle_t mqtt_handle, const char *topic, int qos)
{
    MQTT_Service_t *mqtt = (MQTT_Service_t *)mqtt_handle;
    err_t err;
    EventBits_t bits;

    MQTT_LOCK(mqtt);

    xEventGroupClearBits(mqtt->event, MQTT_SUBSCRIBE_SUCCESS_BIT | MQTT_SUBSCRIBE_FAIL_BIT);

    LOCK_TCPIP_CORE();
    err = mqtt_subscribe(mqtt->client, topic, (u8_t)qos, mqtt_sub_request_cb, mqtt);
    UNLOCK_TCPIP_CORE();

    if (err != ERR_OK) {
        LOGE(TAG, "mqtt_subscribe failed: %d", err);
        MQTT_UNLOCK(mqtt);
        return MQTT_FAIL;
    }

    bits = xEventGroupWaitBits(mqtt->event,
                                MQTT_SUBSCRIBE_SUCCESS_BIT | MQTT_SUBSCRIBE_FAIL_BIT,
                                pdTRUE, pdFALSE, pdMS_TO_TICKS(MQTT_OP_TIMEOUT_MS));

    MQTT_UNLOCK(mqtt);

    return (bits & MQTT_SUBSCRIBE_SUCCESS_BIT) ? MQTT_SUCCESS : MQTT_FAIL;
}

mqtt_err_t mqtt_sv_publish(MQTTHandle_t mqtt_handle, const char *topic,
                            const void *payload, size_t payload_len,
                            int qos, int retain)
{
    MQTT_Service_t *mqtt = (MQTT_Service_t *)mqtt_handle;
    err_t err;
    EventBits_t bits;

    MQTT_LOCK(mqtt);

    xEventGroupClearBits(mqtt->event, MQTT_PUBLISH_SUCCESS_BIT | MQTT_PUBLISH_FAIL_BIT);

    LOCK_TCPIP_CORE();
    err = mqtt_publish(mqtt->client, topic, payload, (u16_t)payload_len,
                        (u8_t)qos, (u8_t)retain, mqtt_pub_request_cb, mqtt);
    UNLOCK_TCPIP_CORE();

    if (err != ERR_OK) {
        LOGE(TAG, "mqtt_publish failed: %d", err);
        MQTT_UNLOCK(mqtt);
        return MQTT_FAIL;
    }

    bits = xEventGroupWaitBits(mqtt->event,
                                MQTT_PUBLISH_SUCCESS_BIT | MQTT_PUBLISH_FAIL_BIT,
                                pdTRUE, pdFALSE, pdMS_TO_TICKS(MQTT_OP_TIMEOUT_MS));

    MQTT_UNLOCK(mqtt);

    return (bits & MQTT_PUBLISH_SUCCESS_BIT) ? MQTT_SUCCESS : MQTT_FAIL;
}

mqtt_err_t mqtt_sv_disconnect(MQTTHandle_t mqtt_handle)
{
    MQTT_Service_t *mqtt = (MQTT_Service_t *)mqtt_handle;

    MQTT_LOCK(mqtt);

    LOCK_TCPIP_CORE();
    mqtt_disconnect(mqtt->client);
    UNLOCK_TCPIP_CORE();

    MQTT_UNLOCK(mqtt);

    return MQTT_SUCCESS;
}

WEAK void mqtt_sv_data_incoming(MQTTHandle_t mqtt, const char *topic, size_t top_len,
                                 const void *payload, size_t payload_len)
{
    LOGD(TAG, "mqtt_sv_data_incoming: topic=%.*s, payload=%.*s",
         (int)top_len, topic, (int)payload_len, (const char *)payload);
}

bool mqtt_sv_is_connected(MQTTHandle_t mqtt_handle)
{
    MQTT_Service_t *mqtt = (MQTT_Service_t *)mqtt_handle;
    return mqtt_client_is_connected(mqtt->client) != 0;
}