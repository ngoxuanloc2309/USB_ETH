#include <string.h>
#include "tusb.h"
#include "class/net/net_device.h"
#include "usb_descriptor.h"
#include "logger.h"
#include "board.h"

static const char *TAG = "USB_DESC";

/* -------------------------------------------------------------------- */
/* Device descriptor                                                    */
/* -------------------------------------------------------------------- */

static tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = USB_BCD,

    /* Interface Association Descriptor selects Misc/IAD/IAD class at
     * device level, required for multi function composite device. */
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,

    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = USB_VID,
    .idProduct          = 0x4000,
    .bcdDevice          = 0x0100,

    .iManufacturer      = STRID_MANUFACTURER,
    .iProduct           = STRID_PRODUCT,
    .iSerialNumber      = STRID_SERIAL,

    .bNumConfigurations = CONFIG_ID_COUNT
};

uint8_t const *tud_descriptor_device_cb(void)
{
    LOGD(TAG, "device descriptor requested");
    return (uint8_t const *)&desc_device;
}

/* -------------------------------------------------------------------- */
/* Configuration descriptors (dual config: RNDIS then ECM)              */
/* -------------------------------------------------------------------- */

#define RNDIS_CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_RNDIS_DESC_LEN + TUD_CDC_DESC_LEN)
#define ECM_CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + TUD_CDC_ECM_DESC_LEN + TUD_CDC_DESC_LEN)

static uint8_t const desc_configuration_rndis[] = {
    TUD_CONFIG_DESCRIPTOR(CONFIG_ID_RNDIS + 1, ITF_NUM_TOTAL, 0,
                           RNDIS_CONFIG_TOTAL_LEN, 0, 100),

    TUD_RNDIS_DESCRIPTOR(ITF_NUM_NET, STRID_MAC,
                          EPNUM_NET_NOTIF, 8,
                          EPNUM_NET_OUT, EPNUM_NET_IN, USB_FS_MAX_PACKET_SIZE),

    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, STRID_SHELL,
                        EPNUM_CDC_NOTIF, 8,
                        EPNUM_CDC_OUT, EPNUM_CDC_IN, USB_FS_MAX_PACKET_SIZE),
};

static uint8_t const desc_configuration_ecm[] = {
    TUD_CONFIG_DESCRIPTOR(CONFIG_ID_ECM + 1, ITF_NUM_TOTAL, 0,
                           ECM_CONFIG_TOTAL_LEN, 0, 100),

    TUD_CDC_ECM_DESCRIPTOR(ITF_NUM_NET, STRID_MAC, STRID_MAC,
                            EPNUM_NET_NOTIF, 8,
                            EPNUM_NET_OUT, EPNUM_NET_IN, USB_FS_MAX_PACKET_SIZE,
                            CFG_TUD_NET_MTU),

    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, STRID_SHELL,
                        EPNUM_CDC_NOTIF, 8,
                        EPNUM_CDC_OUT, EPNUM_CDC_IN, USB_FS_MAX_PACKET_SIZE),
};

static uint8_t const *const desc_configurations[CONFIG_ID_COUNT] = {
    [CONFIG_ID_RNDIS] = desc_configuration_rndis,
    [CONFIG_ID_ECM]   = desc_configuration_ecm,
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    if (index >= CONFIG_ID_COUNT) {
        LOGE(TAG, "configuration descriptor index out of range: %u", index);
        return NULL;
    }

    LOGD(TAG, "configuration descriptor requested, index=%u", index);
    return desc_configurations[index];
}

/* -------------------------------------------------------------------- */
/* String descriptors                                                   */
/* -------------------------------------------------------------------- */

static char const *string_desc_arr[STRID_COUNT] = {
    [STRID_MANUFACTURER] = "Synaptix",
    [STRID_PRODUCT]      = "USB_ETH",
    [STRID_SERIAL]       = "000001",
    [STRID_MAC]          = NULL, /* built dynamically, see below */
    [STRID_SHELL]        = "USB_ETH Shell",
};

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    static uint16_t desc_str[32];
    static char mac_str[12 + 1];
    uint8_t chr_count;

    (void)langid;

    if (index == STRID_LANGID) {
        desc_str[1] = 0x0409; /* English (US) */
        chr_count = 1;
    } else if (index == STRID_MAC) {
        /* Build the 12-char hex ASCII MAC string from the actual
         * tud_network_mac_address[] array, so the string descriptor
         * the host reads always matches the real device MAC. */
        static char const hex_digits[] = "0123456789ABCDEF";

        for (uint8_t i = 0; i < 6; i++) {
            mac_str[2 * i]     = hex_digits[(tud_network_mac_address[i] >> 4) & 0x0F];
            mac_str[2 * i + 1] = hex_digits[tud_network_mac_address[i] & 0x0F];
        }
        mac_str[12] = '\0';

        chr_count = 12;
        for (uint8_t i = 0; i < chr_count; i++) {
            desc_str[1 + i] = mac_str[i];
        }
    } else {
        if (index >= STRID_COUNT || string_desc_arr[index] == NULL) {
            LOGW(TAG, "string descriptor index not defined: %u", index);
            return NULL;
        }

        char const *str = string_desc_arr[index];
        chr_count = (uint8_t)strlen(str);
        if (chr_count > 31) {
            chr_count = 31;
        }

        for (uint8_t i = 0; i < chr_count; i++) {
            desc_str[1 + i] = str[i];
        }
    }

    desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));

    LOGD(TAG, "string descriptor requested, index=%u", index);
    return desc_str;
}

/* -------------------------------------------------------------------- */
/* Network MAC address (required by net_device class driver)            */
/* -------------------------------------------------------------------- */

/* net_device.h declares this as an extern global variable, not a
 * callback function. TinyUSB core reads this array directly when
 * building the RNDIS/ECM descriptors and when answering ARP on the
 * internal (host-side) virtual MAC. A function with this name is
 * never called by the stack. */
uint8_t tud_network_mac_address[6] = {
    MAC_BYTE1, MAC_BYTE2, MAC_BYTE3, MAC_BYTE4, MAC_BYTE5, MAC_BYTE6
};