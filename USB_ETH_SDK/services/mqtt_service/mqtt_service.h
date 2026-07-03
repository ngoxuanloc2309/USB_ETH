#ifndef MQTT_SERVICE_H
#define MQTT_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define WEAK __attribute__((weak))

typedef enum mqtt_err {
    MQTT_SUCCESS = 0,
    MQTT_FAIL
} mqtt_err_t;

/* Opaque handle - internal struct (MQTT_Service_t) lives in
 * mqtt_service.c only. Multiple instances are allowed (unlike
 * usb_netif, a device could talk to more than one broker). */
typedef void *MQTTHandle_t;

/* Connection parameters. All pointer fields are NOT copied - the
 * caller must keep them valid for as long as the handle exists
 * (typically static/global buffers, e.g. app/user/mqtt_app/certificate.h
 * for the TLS fields). */
typedef struct MQTTConfig {
    char *host;
    uint16_t port;
    char *username;      /* NULL if not used */
    char *password;       /* NULL if not used */
    char *client_id;
    uint16_t keep_alive;   /* seconds, 0 disables keep-alive */

    bool tls;
    uint8_t *ca;
    size_t ca_len;
    uint8_t *cli_key;
    size_t cli_key_len;
    uint8_t *cli_crt;
    size_t cli_crt_len;
} MQTTConfig_t;

/**
 * @brief Create an MQTT service instance. Does not connect yet.
 * @param config Owned by the caller, must outlive the handle.
 * @return Handle, or NULL on failure (out of memory / lwIP client
 *         creation failed).
 */
MQTTHandle_t mqtt_sv_create(MQTTConfig_t *config);

void mqtt_sv_free(MQTTHandle_t mqtt);

/**
 * @brief Connect to the broker configured in mqtt_sv_create().
 *
 * Blocks the calling task until connected or timed out. Safe to call
 * from any task - internally marshals all lwIP core calls into
 * tcpip_thread (see mqtt_service.c), does not touch lwIP core
 * directly from the caller's task.
 */
mqtt_err_t mqtt_sv_connect(MQTTHandle_t mqtt);

mqtt_err_t mqtt_sv_subscribe(MQTTHandle_t mqtt, const char *topic, int qos);

mqtt_err_t mqtt_sv_publish(MQTTHandle_t mqtt, const char *topic,
                            const void *payload, size_t payload_len,
                            int qos, int retain);

mqtt_err_t mqtt_sv_disconnect(MQTTHandle_t mqtt);

/**
 * @brief Called when a message arrives on a subscribed topic.
 * Weak default just logs it - the app layer can override this.
 */
WEAK void mqtt_sv_data_incoming(MQTTHandle_t mqtt, const char *topic,
                                 size_t top_len, const void *payload,
                                 size_t payload_len);

bool mqtt_sv_is_connected(MQTTHandle_t mqtt);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_SERVICE_H */