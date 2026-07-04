#include "board.h"
#include "string.h"
#include "stdio.h"
#include "logger.h"
#include "FreeRTOS.h"
#include "task.h"
#include "app_config.h"

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

static void usb_device_task(void *param)
{
    tusb_rhport_init_t rh_init = {
        .role  = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_FULL,
    };

    (void)param;

    LOGI(TAG, "usb device task started, rhport=%d", BOARD_TUD_RHPORT);

    if (!tusb_init(BOARD_TUD_RHPORT, &rh_init)) {
        LOGE(TAG, "tusb_init failed");
        vTaskDelete(NULL);
        return;
    }

    LOGI(TAG, "tinyusb stack initialized, entering tud_task loop");

    for (;;) {
        tud_task();
    }
}

void board_usb_task_start(void)
{
    BaseType_t ret = xTaskCreate(usb_device_task, "usb_dev",
                                  USB_TASK_STACK_SIZE, NULL,
                                  USB_TASK_PRIORITY, NULL);

    if (ret != pdPASS) {
        LOGE(TAG, "failed to create usb device task");
    }
}

void USB_DRD_FS_IRQHandler(void)
{
    dcd_int_handler(0);   // rhport 0, per TinyUSB stm32_fsdev port (dcd_int_handler signature at dcd_stm32_fsdev.c:392)
}