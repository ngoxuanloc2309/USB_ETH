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
    LOGI(TAG, "Board init");
}

void board_get_mac(uint8_t mac[6])
{
    mac[0] = MAC_BYTE1;
    mac[1] = MAC_BYTE2;
    mac[2] = MAC_BYTE3;
    mac[3] = MAC_BYTE4;
    mac[4] = MAC_BYTE5;
    mac[5] = MAC_BYTE6;

    LOGD(TAG, "mac address provided: %02X:%02X:%02X:%02X:%02X:%02X",
         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}