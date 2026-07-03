#ifndef APP_H
#define APP_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Composition root: wires up and starts every module in the
 *        correct order, then hands off to the USB task loop.
 *
 * Call order (fixed, see project handoff doc muc 5/7):
 *   1. board_init()               - HAL/GPIO/UART bring-up
 *   2. usb_cdc_transport_init()   - class-level init, must run before
 *                                   any code that uses the CDC
 *                                   transport and before tud_task()
 *                                   starts processing events
 *   3. usb_netif_init()           - starts tcpip_thread, adds the
 *                                   lwIP netif backing the USB
 *                                   network class
 *   4. shell_app_init()/start()   - CLI shell over CDC
 *   5. mqtt_app_init()/start()    - MQTT test app, after shell_app
 *                                   (project decision: mqtt runs
 *                                   after shell in the boot sequence)
 *   6. board_usb_task_start()     - starts tud_task(); always last,
 *                                   only after every class-level init
 *                                   above has already run
 */
void app_init(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_H */