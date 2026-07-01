#ifndef TUSB_CONFIG_H_
#define TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "tusb_option.h"

/* -------------------------------------------------------------------- */
/* Board specific configuration                                         */
/* -------------------------------------------------------------------- */

#ifndef BOARD_TUD_RHPORT        // Root Hub Port: Use USB controller 0
#define BOARD_TUD_RHPORT 0
#endif

/* USB_DRD_FS peripheral: Full Speed only */
#ifndef BOARD_TUD_MAX_SPEED
#define BOARD_TUD_MAX_SPEED OPT_MODE_FULL_SPEED
#endif

/* -------------------------------------------------------------------- */
/* Common configuration                                                 */
/* -------------------------------------------------------------------- */

#ifndef CFG_TUSB_MCU
#define CFG_TUSB_MCU OPT_MCU_STM32H5
#endif

#define CFG_TUSB_OS OPT_OS_FREERTOS

#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG 0
#endif

#define CFG_TUD_ENABLED 1
#define CFG_TUD_MAX_SPEED BOARD_TUD_MAX_SPEED

#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))
#endif

/* -------------------------------------------------------------------- */
/* Device configuration                                                 */
/* -------------------------------------------------------------------- */

#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE 64
#endif

/* -------------------------------------------------------------------- */
/* Class configuration                                                  */
/* -------------------------------------------------------------------- */

/* Select network class: ECM/RNDIS dual config (Windows uses RNDIS,
 * Linux/macOS uses ECM). NCM is disabled. */
#define USE_ECM 1

#define CFG_TUD_ECM_RNDIS USE_ECM
#define CFG_TUD_NCM       (1 - CFG_TUD_ECM_RNDIS)

/* One CDC-ACM instance, used as shell transport (see usb_cdc.h) */
#define CFG_TUD_CDC 1

/* USB_DRD_FS is Full Speed only, max packet size 64 bytes */
#define CFG_TUD_CDC_RX_BUFSIZE 256
#define CFG_TUD_CDC_TX_BUFSIZE 256

#ifdef __cplusplus
}
#endif

#endif /* TUSB_CONFIG_H_ */