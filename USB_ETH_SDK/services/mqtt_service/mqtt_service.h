#ifndef __MQTT_SERVICE_H__
#define __MQTT_SERVICE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define WEAK __attribute__((weak))

typedef enum mqtt_err{
    MQTT_SUCCESS = 0,
    MQTT_FAIL
}mqtt_err_t;

typedef void * MQTT_Handle_t;

typedef struct MQTTConfig
{
    /* data */
    char *host;
    int port;
    char *username;
    char *password;
    char *client_id;
    bool tls;
    uint8_t *ca;
    size_t ca_len;
    uint8_t *cli_key;
    size_t cli_key_len;
    uint8_t *cli_crt;
    size_t cli_crt_len;
}MQTTConfig_t;

MQTTHandle_t mqtt_sv_create(MQTTConfig_t *config);

mqtt_err_t mqtt_sv_subscribe(MQTTHandle_t mqtt,const char *topic,int qos);
mqtt_err_t mqtt_sv_publish(MQTTHandle_t mqtt,const char *topic,const void *payload,size_t payload_len,int qos, int retain);
mqtt_err_t mqtt_sv_disconnect(MQTTHandle_t mqtt);
/// @brief Virtual mqtt data incoming, User can overwrite this function
/// @param mqtt 
/// @param topic 
/// @param payload 
/// @param payload_len 
/// @return 
WEAK void mqtt_sv_data_incoming(MQTTHandle_t mqtt,const char *topic,size_t top_len,const void *payload,size_t payload_len);

bool mqtt_sv_is_connected(MQTTHandle_t mqtt);

#ifdef __cplusplus
}
#endif

#endif /* __MQTT_SERVICE_H__ */