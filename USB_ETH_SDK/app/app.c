/* SPDX-License-Identifier: MIT */

/*
 * Composition root. Wires every module together in the fixed order
 * documented in app.h. Does not contain any business logic itself -
 * each step below just calls into the module that owns that piece
 * of the system.
 */

#include "app.h"

#include "board.h"
#include "usb_cdc.h"
#include "usb_netif.h"
#include "shell_app.h"
#include "mqtt_app.h"
#include "logger.h"

static const char *TAG = "app";

void app_init(void)
{
    LOGI(TAG, "app_init: starting");

    /* 1. HAL/GPIO/UART bring-up. No dependency on anything else. */
    board_init();
    LOGI(TAG, "board_init done");

    /* 2. Class-level USB CDC init. Must run before shell_app_init()
     * (which drives the shell over this transport) and before
     * board_usb_task_start() (tud_task() must not start processing
     * events before class-level init has run). */
    usb_cdc_transport_init();
    LOGI(TAG, "usb_cdc_transport_init done");

    /* 3. Bring up the lwIP side (tcpip_thread + netif). Must exist
     * before mqtt_app_init()/start() further below. */
    usb_netif_init();
    LOGI(TAG, "usb_netif_init done");

    /* 4. Shell app: init wires the command table into the service,
     * start() spawns the task that drives it. */
    shell_app_init();
    shell_app_start();
    LOGI(TAG, "shell_app started");

    /* 5. MQTT app, after shell_app (project decision: boot order is
     * shell first, then mqtt). init() only builds the config and
     * creates the handle (non-blocking); start() spawns the task
     * that connects (blocking, inside its own task) and runs the
     * publish/subscribe test loop. */
    mqtt_app_init();
    mqtt_app_start();
    LOGI(TAG, "mqtt_app started");

    /* 6. Start tud_task(). Always last: every class-level init that
     * needs to run before the USB stack starts processing events has
     * already happened above. */
    board_usb_task_start();
    LOGI(TAG, "board_usb_task_start done, app_init complete");
}