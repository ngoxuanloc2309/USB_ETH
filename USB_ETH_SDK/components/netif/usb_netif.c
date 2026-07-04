/* SPDX-License-Identifier: MIT */

/*
 * Bridge between the TinyUSB network class (RNDIS/ECM) and lwIP.
 * This is the only module allowed to include both components/usb
 * and components/lwip headers - see the layering rules in the
 * project handoff notes.
 *
 * Contains: tcpip_thread startup + netif_add, RX path
 * (tud_network_recv_cb), TX path (linkoutput + tud_network_xmit_cb),
 * and link state sync (tud_mount_cb/tud_umount_cb <-> netif link up/down).
 */

#include "usb_netif.h"

#include <string.h>

#include "tusb.h"

#include "FreeRTOS.h"
#include "event_groups.h"

#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/dns.h"
#include "lwip/etharp.h"
#include "lwip/pbuf.h"

#include "board.h"
#include "logger.h"

static const char *TAG = "usb-netif";

/* lwIP netif instance backing the USB network interface. Single
 * instance: this project only ever exposes one USB network class. */
static struct netif usb_netif_data;

/* Signals USB link state (set by usb_netif_link_cb(), running in
 * tcpip_thread) to any task that needs to wait for the interface to
 * actually be usable before sending traffic - e.g. mqtt_app_task,
 * which used to call dns_gethostbyname() before USB enumeration even
 * finished, guaranteeing a DNS timeout on every boot. */
static EventGroupHandle_t s_link_event;
#define USB_NETIF_LINK_UP_BIT   (1 << 0)

/* -------------------------------------------------------------------- */
/* RX path: host -> device, USB -> lwip                                 */
/* -------------------------------------------------------------------- */
/* Runs in the USB task context (called from tud_task(), see
 * board_usb_task_start()), NOT in tcpip_thread.
 *
 * Return value contract (see class/net/net_device.h and
 * class/net/ecm_rndis_device.c): if this returns false, TinyUSB
 * calls tud_network_recv_renew() on our behalf (packet dropped,
 * ready for the next one). If it returns true, WE are responsible
 * for calling tud_network_recv_renew() ourselves once we are done
 * reading src - forgetting it stalls the OUT endpoint permanently. */
bool tud_network_recv_cb(const uint8_t *src, uint16_t size)
{
    struct pbuf *p;

    if (size == 0) {
        return true;
    }

    /* src only points into TinyUSB's internal RX buffer, valid only
     * until tud_network_recv_renew() is called - must copy out now. */
    p = pbuf_alloc(PBUF_RAW, size, PBUF_POOL);
    if (p == NULL) {
        LOGW(TAG, "rx: pbuf_alloc failed, size=%u, packet dropped", (unsigned)size);
        return false; /* TinyUSB will renew for us */
    }

    pbuf_take(p, src, size);

    if (usb_netif_data.input == NULL) {
        /* netif_add() (in usb_netif_startup_cb, running inside
         * tcpip_thread) has not finished yet. Extremely unlikely in
         * practice - USB enumeration takes far longer than
         * tcpip_thread startup - but cheap to guard against a NULL
         * deref rather than assume the timing always holds. */
        LOGW(TAG, "rx: netif not ready yet, packet dropped, size=%u", (unsigned)size);
        pbuf_free(p);
        tud_network_recv_renew();
        return true;
    }

    /* usb_netif_data.input == tcpip_input here (set in netif_add()):
     * this only posts the pbuf to tcpip_thread's mailbox and returns,
     * it does not process the packet in this task. */
    if (usb_netif_data.input(p, &usb_netif_data) != ERR_OK) {
        LOGW(TAG, "rx: netif input failed (mailbox full?), size=%u", (unsigned)size);
        pbuf_free(p);
    } else {
        LOGD(TAG, "rx: queued %u bytes to tcpip_thread", (unsigned)size);
    }

    /* We took ownership of the buffer (accepted the packet, whether
     * netif input ultimately succeeded or not) - must renew ourselves. */
    tud_network_recv_renew();

    return true;
}

/* -------------------------------------------------------------------- */
/* TX path: lwip -> device, lwip -> USB                                 */
/* -------------------------------------------------------------------- */
/* Runs in tcpip_thread context (called by lwip core, ip_output ->
 * etharp_output -> netif->linkoutput), NOT in the USB task.
 *
 * IMPORTANT: unlike the reference project, this must NOT call
 * tud_task() itself. In this project tud_task() already runs in its
 * own dedicated loop (board_usb_task_start()). Calling it again from
 * here (tcpip_thread) would touch TinyUSB's internal state from two
 * tasks at once - not safe, TinyUSB is only meant to be driven from
 * a single task. If TX is momentarily busy we simply fail the send
 * (ERR_WOULDBLOCK) and let it be retried:
 *   - TCP: lwip's own retransmission timer resends the segment later,
 *     no data lost, only delayed.
 *   - UDP: the datagram is lost (no automatic retry) - acceptable
 *     trade-off for now, see design discussion (Option A chosen over
 *     a bounded retry-with-delay Option B). */
static err_t usb_netif_linkoutput(struct netif *netif, struct pbuf *p)
{
    (void)netif;

    /* Diagnostic: identify what is actually being transmitted (ARP vs
     * IP, and for IP which protocol + destination) rather than relying
     * on byte-count-only logs, which turned out to be unreliable to
     * read by eye once several tasks are logging over the same UART
     * at once (lines get interleaved/garbled). Ethertype is at offset
     * 12-13 of the Ethernet frame; for IPv4, protocol is at offset 23
     * and destination address at offset 30-33 (14 byte eth header +
     * fixed 20 byte IPv4 header, no options assumed for this printout
     * - fine for a diagnostic, real header length is not re-derived
     * here since nothing downstream depends on this log). */
    if (p->tot_len >= 14) {
        uint8_t hdr[34];
        uint16_t copied = pbuf_copy_partial(p, hdr, sizeof(hdr) > p->tot_len ? p->tot_len : sizeof(hdr), 0);
        uint16_t ethertype = (copied >= 14) ? ((hdr[12] << 8) | hdr[13]) : 0;

        if (ethertype == 0x0806) {
            LOGI(TAG, "tx: ARP, len=%u", (unsigned)p->tot_len);
        } else if (ethertype == 0x0800 && copied >= 34) {
            LOGI(TAG, "tx: IP proto=%u dst=%u.%u.%u.%u, len=%u",
                 (unsigned)hdr[23], (unsigned)hdr[30], (unsigned)hdr[31],
                 (unsigned)hdr[32], (unsigned)hdr[33], (unsigned)p->tot_len);
        } else {
            LOGI(TAG, "tx: ethertype=0x%04x, len=%u", (unsigned)ethertype, (unsigned)p->tot_len);
        }
    }

    if (!tud_ready()) {
        LOGW(TAG, "tx: usb not ready, packet dropped, len=%u", (unsigned)p->tot_len);
        return ERR_USE;
    }

    if (!tud_network_can_xmit(p->tot_len)) {
        LOGD(TAG, "tx: usb busy, packet dropped, len=%u", (unsigned)p->tot_len);
        return ERR_WOULDBLOCK;
    }

    /* Hold a reference until tud_network_xmit_cb() has actually copied
     * the data out - tud_network_xmit() only schedules the transfer,
     * it does not copy synchronously. */
    pbuf_ref(p);
    tud_network_xmit(p, 0);

    return ERR_OK;
}

/* Called by TinyUSB once it is ready to copy the actual bytes out of
 * our pbuf into its own USB TX buffer (dst). Runs in the USB task
 * context (from tud_task()). */
uint16_t tud_network_xmit_cb(uint8_t *dst, void *ref, uint16_t arg)
{
    struct pbuf *p = (struct pbuf *)ref;
    uint16_t n;

    (void)arg;

    n = pbuf_copy_partial(p, dst, p->tot_len, 0);
    pbuf_free(p); /* release the reference taken in usb_netif_linkoutput() */

    LOGD(TAG, "tx: copied %u bytes to usb", (unsigned)n);

    return n;
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
    netif->linkoutput = usb_netif_linkoutput;
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
        /* ipaddr_ntoa() returns a pointer into a single shared static
         * buffer (not reentrant) - calling it 3 times as arguments to
         * one printf-style call makes all 3 %s print the same (last
         * evaluated) value. Use the reentrant ip4addr_ntoa_r() with a
         * separate buffer per address instead. */
        char ip_str[16], mask_str[16], gw_str[16];
        ip4addr_ntoa_r(netif_ip4_addr(netif), ip_str, sizeof(ip_str));
        ip4addr_ntoa_r(netif_ip4_netmask(netif), mask_str, sizeof(mask_str));
        ip4addr_ntoa_r(netif_ip4_gw(netif), gw_str, sizeof(gw_str));
        LOGI(TAG, "netif up, ip=%s mask=%s gw=%s", ip_str, mask_str, gw_str);
    } else {
        LOGI(TAG, "netif down");
    }
}

static void usb_netif_link_cb(struct netif *netif)
{
    bool is_up = netif_is_link_up(netif);

    LOGD(TAG, "netif link %s", is_up ? "up" : "down");

    if (is_up) {
        xEventGroupSetBits(s_link_event, USB_NETIF_LINK_UP_BIT);
    } else {
        xEventGroupClearBits(s_link_event, USB_NETIF_LINK_UP_BIT);
    }

    /* Tell TinyUSB so it can send the RNDIS media status indication
     * ("cable connected"/"disconnected") to the host. ECM does not
     * use this, the call is harmless for it. */
    tud_network_link_state(BOARD_TUD_RHPORT, is_up);
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
     * (see below). netif_set_up() here only marks the interface
     * administratively up; it does not imply the USB link is present -
     * that part is netif_set_link_up()/down(), called separately once
     * the host actually finishes enumeration. */
    netif_set_up(&usb_netif_data);

    dns_setserver(0, &dns_ip);

    LOGI(TAG, "usb netif started");
}

/* rndis_class_set_handler() is now provided by the vendor's own
 * lib/networking/rndis_reports.c (see components/CMakeLists.txt,
 * target usb_netif) instead of a local stub - full RNDIS OID
 * handling (INITIALIZE/QUERY/SET/RESET/KEEPALIVE), reusing
 * tud_network_mac_address[] already defined in usb_descriptors.c. */

/* -------------------------------------------------------------------- */
/* Public API                                                            */
/* -------------------------------------------------------------------- */

void usb_netif_init(void)
{
    s_link_event = xEventGroupCreate();
    if (s_link_event == NULL) {
        LOGE(TAG, "xEventGroupCreate failed");
    }

    LOGI(TAG, "starting tcpip_thread");
    tcpip_init(usb_netif_startup_cb, NULL);
}

bool usb_netif_wait_link_up(uint32_t timeout_ms)
{
    if (s_link_event == NULL) {
        return false;
    }

    EventBits_t bits = xEventGroupWaitBits(
        s_link_event,
        USB_NETIF_LINK_UP_BIT,
        pdFALSE,                        /* do not clear on exit */
        pdTRUE,                         /* wait for all (only 1) bit */
        (timeout_ms == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms));

    return (bits & USB_NETIF_LINK_UP_BIT) != 0;
}

/* -------------------------------------------------------------------- */
/* TinyUSB device mount/unmount -> lwip link state                      */
/* -------------------------------------------------------------------- */
/* tud_mount_cb()/tud_umount_cb() run in the USB task (called from
 * tud_task()), but netif_set_link_up()/netif_set_link_down() are lwip
 * core API and must only be called from tcpip_thread (same reasoning
 * as the TX path above). tcpip_callback() safely marshals the call
 * into tcpip_thread's mailbox instead of calling it directly here. */

static void usb_netif_set_link_up_cb(void *ctx)
{
    (void)ctx;
    netif_set_link_up(&usb_netif_data);
}

static void usb_netif_set_link_down_cb(void *ctx)
{
    (void)ctx;
    netif_set_link_down(&usb_netif_data);
}

void tud_mount_cb(void)
{
    LOGI(TAG, "usb mounted (enumeration complete)");

    if (tcpip_callback(usb_netif_set_link_up_cb, NULL) != ERR_OK) {
        LOGE(TAG, "failed to schedule netif_set_link_up");
    }
}

void tud_umount_cb(void)
{
    LOGI(TAG, "usb unmounted");

    if (tcpip_callback(usb_netif_set_link_down_cb, NULL) != ERR_OK) {
        LOGE(TAG, "failed to schedule netif_set_link_down");
    }
}