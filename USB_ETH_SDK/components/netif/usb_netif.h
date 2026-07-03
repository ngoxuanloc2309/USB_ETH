#ifndef USB_NETIF_H
#define USB_NETIF_H

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

#ifdef __cplusplus
}
#endif

#endif