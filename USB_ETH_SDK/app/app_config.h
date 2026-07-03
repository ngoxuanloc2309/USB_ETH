#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h5xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

#define USB_TASK_STACK_SIZE     512
#define USB_TASK_PRIORITY       osPriorityAboveNormal

// MQTT config
#define MQTT_HOST               "broker.hivemq.com"
#define MQTT_PORT               1883
#define MQTT_TEST_PUB_TOPIC     "log/usb_eth"


#ifdef __cplusplus
}
#endif

#endif /* __APP_CONFIG_H__ */