#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h5xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

#define USB_TASK_STACK_SIZE  512
#define USB_TASK_PRIORITY    osPriorityAboveNormal

#ifdef __cplusplus
}
#endif

#endif /* __APP_CONFIG_H__ */