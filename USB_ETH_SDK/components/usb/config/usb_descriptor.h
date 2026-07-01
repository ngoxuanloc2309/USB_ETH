#ifndef USB_DESCRIPTOR_H_
#define USB_DESCRIPTOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* -------------------------------------------------------------------- */
/* Interface numbering                                                  */
/* -------------------------------------------------------------------- */
/* Network class (ECM/RNDIS) uses 2 interfaces: control + data.
 * CDC-ACM (shell transport) uses 2 interfaces: control + data.
 * See the endpoint numbering section below for the hardware channel
 * budget (5 of 8 channels used). */

enum {
    ITF_NUM_NET = 0,
    ITF_NUM_NET_DATA,
    ITF_NUM_CDC,
    ITF_NUM_CDC_DATA,
    ITF_NUM_TOTAL
};

/* -------------------------------------------------------------------- */
/* String descriptor indices                                            */
/* -------------------------------------------------------------------- */

enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
    STRID_MAC,
    STRID_SHELL,
    STRID_COUNT
};

/* -------------------------------------------------------------------- */
/* Configuration descriptor id (dual config)                            */
/* -------------------------------------------------------------------- */
/* Config 1: RNDIS  -> selected by Windows hosts
 * Config 2: ECM     -> selected by Linux and macOS hosts
 * Host picks the config it supports during enumeration. */

enum {
    CONFIG_ID_RNDIS = 0,
    CONFIG_ID_ECM,
    CONFIG_ID_COUNT
};

/* -------------------------------------------------------------------- */
/* Endpoint numbering                                                    */
/* -------------------------------------------------------------------- */
/* USB_DRD_FS is Full Speed only: max packet size for bulk/interrupt
 * endpoints is 64 bytes (no 512 byte high speed variant).
 *
 * USB_DRD_FS has 8 hardware channels (CHEP0R..CHEP7R). Each channel has
 * ONE type field (USB_CHEP_UTYPE) shared by both its IN and OUT
 * direction, so a channel cannot mix Interrupt and Bulk at the same
 * time. Bulk OUT and Bulk IN of the same function CAN share one
 * channel (same type), but the notification (Interrupt) endpoint of a
 * class must use its own separate channel.
 *
 * Channel map:
 *   ch0 EP0             control      (mandatory)
 *   ch1 EPNUM_NET_NOTIF interrupt IN
 *   ch2 EPNUM_NET_OUT/IN bulk        (shared channel, same type)
 *   ch3 EPNUM_CDC_NOTIF interrupt IN
 *   ch4 EPNUM_CDC_OUT/IN bulk        (shared channel, same type)
 *   5 of 8 channels used, 3 free for future expansion. */

#define EPNUM_NET_NOTIF  0x81
#define EPNUM_NET_OUT    0x02
#define EPNUM_NET_IN     0x82

#define EPNUM_CDC_NOTIF  0x83
#define EPNUM_CDC_OUT    0x04
#define EPNUM_CDC_IN     0x84

#define USB_FS_MAX_PACKET_SIZE  64

/* -------------------------------------------------------------------- */
/* Device identity                                                      */
/* -------------------------------------------------------------------- */

#define USB_VID   0xCafe
#define USB_BCD   0x0200

#ifdef __cplusplus
}
#endif

#endif /* USB_DESCRIPTOR_H_ */