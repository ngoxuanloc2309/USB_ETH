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

/* Locally administered, unicast MAC (bit0 of byte1 = 0, bit1 = 1).
 * Used by components/usb for the USB network interface (ECM/RNDIS).
 * TODO: replace with STM32 96-bit UID derived value if multiple
 * boards need to run on the same network at the same time. */
#define MAC_BYTE1  0x02
#define MAC_BYTE2  0x00
#define MAC_BYTE3  0x00
#define MAC_BYTE4  0x00
#define MAC_BYTE5  0x00
#define MAC_BYTE6  0x00

#define IP_BYTE1   192
#define IP_BYTE2   168
#define IP_BYTE3   110
#define IP_BYTE4   2

#define GW_BYTE1   192
#define GW_BYTE2   168
#define GW_BYTE3   110
#define GW_BYTE4   1

#define DNS_BYTE1  GW_BYTE1
#define DNS_BYTE2  GW_BYTE2
#define DNS_BYTE3  GW_BYTE3
#define DNS_BYTE4  GW_BYTE4

#define NETMASK_BYTE1  255
#define NETMASK_BYTE2  255
#define NETMASK_BYTE3  255
#define NETMASK_BYTE4  0



void board_init(void);

void uart_log_write(const char *str);

void board_get_mac(uint8_t mac[6]);

/**
 * @brief Start the FreeRTOS task that runs the TinyUSB device stack.
 *
 * Creates a task that calls tusb_init() then loops tud_task() forever.
 * Generic USB stack bring-up only: this function does not know about
 * any specific class (CDC, network...). Any class-level init that
 * must run before tud_task() starts processing events (for example
 * usb_cdc_transport_init()) must be called by the composition layer
 * (app_init()) BEFORE calling this function.
 */
void board_usb_task_start(void);

#ifdef __cplusplus
}
#endif

#endif