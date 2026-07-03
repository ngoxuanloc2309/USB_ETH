/* SPDX-License-Identifier: MIT */

/*
 * Bridge between the TinyUSB network class (RNDIS/ECM) and lwIP.
 * This is the only module allowed to include both components/usb
 * and components/lwip headers - see the layering rules in the
 * project handoff notes.
 *
 * Step 2a: init/startup only (tcpip_init + netif_add). RX path, TX
 * path and tud_mount_cb/tud_umount_cb wiring are added in later
 * steps, on top of this file.
 */

#include "usb_netif.h"

#include <string.h>

#include "tusb.h"

#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/dns.h"
#include "lwip/etharp.h"

#include "board.h"
#include "logger.h"

static const char *TAG = "usb-netif";

/* lwIP netif instance backing the USB network interface. Single
 * instance: this project only ever exposes one USB network class. */
static struct netif usb_netif_data;

/* -------------------------------------------------------------------- */
/* TX path placeholder (implemented in a later step)                    */
/* -------------------------------------------------------------------- */

/* TODO (next step - TX path): replace with the real implementation
 * that hands the pbuf to tud_network_xmit(). Kept as a safe stub for
 * now so netif_add() always has a valid linkoutput pointer. */
static err_t usb_netif_linkoutput_stub(struct netif *netif, struct pbuf *p)
{
    (void)netif;
    (void)p;

    LOGW(TAG, "linkoutput called before TX path is implemented");
    return ERR_USE;
}

/* -------------------------------------------------------------------- */
/* netif init callback (called once by netif_add())                    */
/* -------------------------------------------------------------------- */

static err_t usb_netif_init_cb(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", netif != NULL);

    netif->mtu = CFG_TUD_NET_MTU;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
    netif->name[0] = 'E';
    netif->name[1] = 'X';
    netif->linkoutput = usb_netif_linkoutput_stub;
    netif->output = etharp_output;

    LOGD(TAG, "netif init callback done, mtu=%u", (unsigned)netif->mtu);

    return ERR_OK;
}

/* -------------------------------------------------------------------- */
/* netif status/link callbacks (logging only)                           */
/* -------------------------------------------------------------------- */

static void usb_netif_status_cb(struct netif *netif)
{
    if (netif_is_up(netif)) {
        LOGI(TAG, "netif up, ip=%s mask=%s gw=%s",
             ipaddr_ntoa(&netif->ip_addr),
             ipaddr_ntoa(&netif->netmask),
             ipaddr_ntoa(&netif->gw));
    } else {
        LOGI(TAG, "netif down");
    }
}

static void usb_netif_link_cb(struct netif *netif)
{
    LOGD(TAG, "netif link %s", netif_is_link_up(netif) ? "up" : "down");
}

/* -------------------------------------------------------------------- */
/* tcpip_thread startup callback - runs INSIDE tcpip_thread             */
/* -------------------------------------------------------------------- */

static void usb_netif_startup_cb(void *arg)
{
    ip4_addr_t ipaddr, netmask, gw, dns_ip;

    (void)arg;

    IP4_ADDR(&ipaddr,  IP_BYTE1,  IP_BYTE2,  IP_BYTE3,  IP_BYTE4);
    IP4_ADDR(&netmask, NETMASK_BYTE1, NETMASK_BYTE2, NETMASK_BYTE3, NETMASK_BYTE4);
    IP4_ADDR(&gw,      GW_BYTE1,  GW_BYTE2,  GW_BYTE3,  GW_BYTE4);
    IP4_ADDR(&dns_ip,  DNS_BYTE1, DNS_BYTE2, DNS_BYTE3, DNS_BYTE4);

    /* The lwIP-side (virtual) MAC must differ from the MAC the host
     * sees via the USB descriptor (tud_network_mac_address[]),
     * otherwise both ends of the virtual link share the same MAC and
     * ARP breaks. Toggling the LSB of the last byte is enough. */
    usb_netif_data.hwaddr_len = sizeof(tud_network_mac_address);
    memcpy(usb_netif_data.hwaddr, tud_network_mac_address, sizeof(tud_network_mac_address));
    usb_netif_data.hwaddr[5] ^= 0x01;

    LOGD(TAG, "netif hwaddr: %02X:%02X:%02X:%02X:%02X:%02X",
         usb_netif_data.hwaddr[0], usb_netif_data.hwaddr[1],
         usb_netif_data.hwaddr[2], usb_netif_data.hwaddr[3],
         usb_netif_data.hwaddr[4], usb_netif_data.hwaddr[5]);

    /* IMPORTANT: tcpip_input, NOT ethernet_input. This project uses
     * BSD sockets (services/socket_client, tcp_server), which require
     * tcpip_thread to own the lwIP core - see architecture decision
     * mục 5.7 in the handoff notes. */
    if (netif_add(&usb_netif_data, &ipaddr, &netmask, &gw, NULL,
                  usb_netif_init_cb, tcpip_input) == NULL) {
        LOGE(TAG, "netif_add failed");
        return;
    }

    netif_set_default(&usb_netif_data);
    netif_set_status_callback(&usb_netif_data, usb_netif_status_cb);
    netif_set_link_callback(&usb_netif_data, usb_netif_link_cb);

    /* Link state (up/down) is driven from tud_mount_cb()/tud_umount_cb()
     * in a later step. netif_set_up() only marks the interface
     * administratively up; it does not imply the USB link is present. */
    netif_set_up(&usb_netif_data);

    dns_setserver(0, &dns_ip);

    LOGI(TAG, "usb netif started");
}

/* -------------------------------------------------------------------- */
/* Public API                                                            */
/* -------------------------------------------------------------------- */

void usb_netif_init(void)
{
    LOGI(TAG, "starting tcpip_thread");
    tcpip_init(usb_netif_startup_cb, NULL);
}