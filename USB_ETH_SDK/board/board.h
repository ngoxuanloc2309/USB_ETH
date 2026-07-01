#ifndef BOARD_H
#define BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "gpio.h"
#include "usart.h"
#include "stm32h5xx_hal.h"
#include "tusb.h"

#define UART_LOGGER &huart3

void board_init(void);

void uart_log_write(const char *str);

#ifdef __cplusplus
}
#endif

#endif
