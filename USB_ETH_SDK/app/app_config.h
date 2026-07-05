#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h5xx_hal.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"

#define USB_TASK_STACK_SIZE     512
#define USB_TASK_PRIORITY       osPriorityAboveNormal

#define USE_DHCP                0
#define USE_STATIC_IP           1

// MQTT config
#define MQTT_HOST                   "broker.hivemq.com"
#define MQTT_PORT                    1883
#define MQTT_TEST_PUB_TOPIC         "log/pub/usb_eth"
#define MQTT_TEST_SUB_TOPIC         "log/sub/usb_eth"
#define MQTT_APP_CLIENT_ID          "usb_eth_dev_01"
#define MQTT_APP_KEEPALIVE_S         60
#define MQTT_APP_PUBLISH_PERIOD_MS   10000


#ifdef __cplusplus
}
#endif

#endif /* __APP_CONFIG_H__ */