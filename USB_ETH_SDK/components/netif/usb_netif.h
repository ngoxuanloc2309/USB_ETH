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
 * tud_mount_cb() has run (see usb_netif.c). This only reflects the
 * link layer (cable/USB present) - it says nothing about whether the
 * netif has a usable IP address yet (see usb_netif_wait_ip_ready()
 * below, which most callers doing DNS/sockets/MQTT should use instead).
 *
 * @param timeout_ms  Timeout in milliseconds, or 0 to wait forever.
 * @return true if the link came up before the timeout, false otherwise.
 */
bool usb_netif_wait_link_up(uint32_t timeout_ms);

/**
 * @brief Block until the netif has a valid (non-zero) IP address, or
 *        until timeout.
 *
 * With USE_STATIC_IP this is true almost as soon as the link is up,
 * since the address is already known at netif_add() time. With
 * USE_DHCP the address is only assigned after the DHCP handshake
 * (discover/offer/request/ack) completes, which can take noticeably
 * longer than link-up - code that only waits on
 * usb_netif_wait_link_up() and then immediately does DNS/connect
 * will race ahead of DHCP and fail. Anything that needs a working
 * IP stack (DNS, sockets, MQTT...) should wait on this, not on
 * usb_netif_wait_link_up().
 *
 * @param timeout_ms  Timeout in milliseconds, or 0 to wait forever.
 * @return true if a valid IP was assigned before the timeout, false otherwise.
 */
bool usb_netif_wait_ip_ready(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif