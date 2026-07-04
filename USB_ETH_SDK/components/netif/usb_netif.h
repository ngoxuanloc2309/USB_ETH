#ifndef USB_NETIF_H
#define USB_NETIF_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bring up the USB network interface (lwIP side).
 *
 * Starts tcpip_thread (tcpip_init()) and, inside that thread, adds
 * and configures the lwIP netif backing the USB network class
 * (RNDIS/ECM). Uses static IP/netmask/gateway/DNS from board.h.
 *
 * Does NOT touch TinyUSB (tusb_init()/tud_task()) - that is owned
 * by board_usb_task_start(). Per the composition order in app_init(),
 * this must be called AFTER usb_cdc_transport_init() and BEFORE
 * board_usb_task_start().
 */
void usb_netif_init(void);

/**
 * @brief Block until the USB network link is up, or until timeout.
 *
 * Link is only up after the USB host finishes enumeration and
 * tud_mount_cb() has run (see usb_netif.c). Any code that sends
 * network traffic right after boot - e.g. mqtt_app connecting -
 * must wait on this first: calling into lwIP before the link is up
 * silently drops packets and, for DNS in particular, wastes the
 * full resolver timeout every time.
 *
 * @param timeout_ms  Timeout in milliseconds, or 0 to wait forever.
 * @return true if the link came up before the timeout, false otherwise.
 */
bool usb_netif_wait_link_up(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif