#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "mqtt_app.h"
#include "mqtt_service.h"
#include "app_config.h"
#include "logger.h"

static const char *TAG = "mqtt-app";

/* TODO: derive from board UID (same reasoning as board_get_mac's
 * MAC_BYTE1.. TODO) to guarantee uniqueness if more than one board
 * connects to the same broker. A fixed id is fine for now: this is a
 * single-device test against a public broker. */
#define MQTT_APP_PAYLOAD_MAX_LEN    48

static MQTTConfig_t s_mqtt_config;
static MQTTHandle_t s_mqtt_handle;
static uint32_t s_pub_counter;

void mqtt_app_init(void)
{
    memset(&s_mqtt_config, 0, sizeof(s_mqtt_config));

    s_mqtt_config.host = MQTT_HOST;
    s_mqtt_config.port = MQTT_PORT;
    s_mqtt_config.username = NULL;
    s_mqtt_config.password = NULL;
    s_mqtt_config.client_id = MQTT_APP_CLIENT_ID;
    s_mqtt_config.keep_alive = MQTT_APP_KEEPALIVE_S;
    s_mqtt_config.tls = false; /* plaintext for now, see project TLS TODO */

    s_mqtt_handle = mqtt_sv_create(&s_mqtt_config);
    if (s_mqtt_handle == NULL) {
        LOGE(TAG, "mqtt_sv_create failed");
    }
}

static void mqtt_app_task(void *param)
{
    char payload[MQTT_APP_PAYLOAD_MAX_LEN];

    (void)param;

    if (s_mqtt_handle == NULL) {
        LOGE(TAG, "no mqtt handle (mqtt_app_init failed?), task exiting");
        vTaskDelete(NULL);
        return;
    }

    LOGI(TAG, "connecting to %s:%u", MQTT_HOST, (unsigned)MQTT_PORT);

    if (mqtt_sv_connect(s_mqtt_handle) != MQTT_SUCCESS) {
        /* Option A: no auto-reconnect in this version, see mqtt_app.h. */
        LOGE(TAG, "connect failed, task exiting");
        vTaskDelete(NULL);
        return;
    }

    LOGI(TAG, "connected to broker");

    if (mqtt_sv_subscribe(s_mqtt_handle, MQTT_TEST_PUB_TOPIC, 0) != MQTT_SUCCESS) {
        LOGW(TAG, "subscribe to '%s' failed, continuing anyway (publish-only test)", MQTT_TEST_PUB_TOPIC);
    } else {
        LOGI(TAG, "subscribed to '%s'", MQTT_TEST_PUB_TOPIC);
    }

    for (;;) {
        int n = snprintf(payload, sizeof(payload), "hello from usb_eth #%lu",
                          (unsigned long)s_pub_counter++);
        if (n < 0) {
            n = 0;
        } else if ((size_t)n >= sizeof(payload)) {
            n = (int)sizeof(payload) - 1; /* snprintf truncated, clamp length */
        }

        if (mqtt_sv_publish(s_mqtt_handle, MQTT_TEST_PUB_TOPIC, payload, (size_t)n, 0, 0) != MQTT_SUCCESS) {
            LOGW(TAG, "publish failed");
        } else {
            LOGD(TAG, "published: %s", payload);
        }

        vTaskDelay(pdMS_TO_TICKS(MQTT_APP_PUBLISH_PERIOD_MS));
    }
}

void mqtt_app_start(void)
{
    BaseType_t ret = xTaskCreate(mqtt_app_task, "mqtt_app",
                                  MQTT_APP_TASK_STACK_SIZE, NULL,
                                  MQTT_APP_TASK_PRIORITY, NULL);

    if (ret != pdPASS) {
        LOGE(TAG, "failed to create mqtt_app task");
    }
}

/* Override the weak default in mqtt_service.c: this is the round-trip
 * test - since mqtt_app subscribes to the same topic it publishes to,
 * seeing our own published messages come back here confirms publish,
 * subscribe and the broker round-trip all work. */
void mqtt_sv_data_incoming(MQTTHandle_t mqtt, const char *topic, size_t top_len,
                            const void *payload, size_t payload_len)
{
    (void)mqtt;

    LOGI(TAG, "rx: topic=%.*s payload=%.*s",
         (int)top_len, topic, (int)payload_len, (const char *)payload);
}