#ifndef LWIPOPTS_H
#define LWIPOPTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <limits.h>

/* -------------------------------------------------------------------- */
/* OS mode                                                               */
/* -------------------------------------------------------------------- */
#define NO_SYS                         0        
/* NO_SYS is 0 by default (not defined here): full lwip stack with a
 * dedicated tcpip_thread, required for the socket/sequential API used
 * by services/socket_client, services/tcp_server (lwip/sockets.h).
 * See project decision: "Option A" (tcpip_thread + socket API), chosen
 * because those services already depend on it. */

#define LWIP_TCPIP_CORE_LOCKING        1
#define LWIP_TCPIP_CORE_LOCKING_INPUT  0

#define LWIP_SOCKET   1
#define LWIP_NETCONN  1

/* -------------------------------------------------------------------- */
/* IP configuration                                                     */
/* -------------------------------------------------------------------- */
/* Static IP only: this device does not act as a DHCP client (LWIP_DHCP
 * is the client role, not needed) and does not act as a DHCP or DNS
 * server either. IP/netmask/gateway values themselves are defined in
 * board.h (IP_BYTE1.., GW_BYTE1..), consumed by components/netif. */
#define LWIP_DHCP  0

/* DNS client (resolver): needed so the device itself can resolve a
 * public host name (for example an MQTT broker) through the host's
 * shared internet connection (Windows ICS). This is unrelated to a
 * DNS server: nothing here answers DNS queries FROM the host. */
#define LWIP_DNS  1

#define LWIP_NETIF_LINK_CALLBACK    1
#define LWIP_NETIF_STATUS_CALLBACK  1

/* -------------------------------------------------------------------- */
/* Checksums                                                             */
/* -------------------------------------------------------------------- */
/* IMPORTANT: do not define CHECKSUM_BY_HARDWARE. That flag only makes
 * sense with STM32's ETH peripheral DMA (hardware checksum offload)
 * used by CubeMX's own ethernetif.c. This project has no such
 * peripheral in the USB netif path (see components/netif), so all
 * checksums must be computed in software. CHECKSUM_GEN_CHECKSUM_CHECK_*
 * already default to 1 (software) in lwip/opt.h, left at default here
 * on purpose, not overridden. */

/* -------------------------------------------------------------------- */
/* Memory                                                                */
/* -------------------------------------------------------------------- */
#define MEM_ALIGNMENT  4
#define MEM_SIZE       4096

#define MEMP_NUM_UDP_PCB  8
#define MEMP_NUM_TCP_PCB  8
#define MEMP_NUM_NETCONN  8

#define PBUF_POOL_SIZE     16
#define PBUF_POOL_BUFSIZE  1536

/* -------------------------------------------------------------------- */
/* TCP                                                                   */
/* -------------------------------------------------------------------- */
/* 1500 (standard Ethernet MTU, matches CFG_TUD_NET_MTU default 1514
 * minus the 14 byte Ethernet header) - 20 (IP) - 20 (TCP) = 1460.
 * Kept as a plain literal here: lwipopts.h is included standalone by
 * lwip core .c files with no guarantee tusb.h (CFG_TUD_NET_MTU) is
 * visible in that translation unit. */
#define TCP_MSS       1460
#define TCP_SND_BUF   (4 * TCP_MSS)
#define TCP_WND       (4 * TCP_MSS)

/* -------------------------------------------------------------------- */
/* OS thread configuration (system/OS/sys_arch.c, CMSIS-OS2 port)       */
/* -------------------------------------------------------------------- */
/* Raw FreeRTOS priority numbers (configMAX_PRIORITIES = 56 in this
 * project, see Core/Inc/FreeRTOSConfig.h). TCPIP_THREAD_PRIO is set
 * well above board_usb_task_start()'s USB task (idle+3) so lwip core
 * processing is not starved. Revisit together with all other task
 * priorities (shell, tcp_server...) once they exist. */
#define TCPIP_THREAD_STACKSIZE  2048
#define TCPIP_THREAD_PRIO       40
#define TCPIP_MBOX_SIZE         8

#define DEFAULT_THREAD_STACKSIZE   1024
#define DEFAULT_THREAD_PRIO        3
#define DEFAULT_UDP_RECVMBOX_SIZE  6
#define DEFAULT_TCP_RECVMBOX_SIZE  6
#define DEFAULT_ACCEPTMBOX_SIZE    6

/* -------------------------------------------------------------------- */
/* Misc                                                                  */
/* -------------------------------------------------------------------- */
#define LWIP_ETHERNET      1
#define LWIP_STATS         0
#define MEMP_NUM_SYS_TIMEOUT  16

/* -------------------------------------------------------------------- */
/* Debug - ETHARP only, temporary, for the MQTT/TCP connect timeout     */
/* investigation (board's ARP request to the gateway never gets a      */
/* reply applied - see project handoff notes).                         */
/*                                                                      */
/* LWIP_DEBUGF() is a no-op unless LWIP_DEBUG is #defined (existence   */
/* check, not value check - see lwip/debug.h) REGARDLESS of            */
/* ETHARP_DEBUG/LWIP_DBG_MIN_LEVEL being set. Missing this is why the   */
/* previous attempt produced no output at all.                         */
/*                                                                      */
/* printf() is not retargeted to any UART in this project (no          */
/* _write()/__io_putchar() found in board/) - the default              */
/* LWIP_PLATFORM_DIAG(x) calling printf(x) would go nowhere. Route      */
/* through utils/logger instead (components/CMakeLists.txt: lwip now   */
/* links logger for this).                                             */
#define LWIP_DEBUG
#include "logger.h"
#define LWIP_PLATFORM_DIAG(x)   do { lwip_diag_log x; } while (0)

#define LWIP_DBG_MIN_LEVEL       LWIP_DBG_LEVEL_ALL
#define ETHARP_DEBUG             LWIP_DBG_ON

#ifdef __cplusplus
}
#endif

#endif /* LWIPOPTS_H */