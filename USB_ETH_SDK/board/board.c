#include "board.h"
#include "string.h"
#include "stdio.h"
#include "logger.h"

const char *TAG = "BOARD";

void uart_log_write(const char *str)
{
    HAL_UART_Transmit(UART_LOGGER, (uint8_t *)str, strlen(str), 25);
}

void board_init(void){
    logger_init(LOGGER_DEBUG, uart_log_write);
    log_info(TAG, "Board init");
} 