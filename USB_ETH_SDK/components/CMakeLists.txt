cmake_minimum_required(VERSION 3.22)

enable_language(C)

# ============================================================================
# tusb_config (INTERFACE) - exposes components/usb/config (tusb_config.h)
# Consumed by: tinyusb (vendor lib, built below) and usb_cdc.
# ============================================================================
add_library(tusb_config INTERFACE)
target_include_directories(tusb_config INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/usb/config
)

# ============================================================================
# lwip_config (INTERFACE) - exposes components/lwip (lwipopts.h)
# Consumed by: lwip (vendor lib, built below) and any module calling lwIP
# directly (usb_netif, mqtt_service, socketclient, socketserver).
# ============================================================================
add_library(lwip_config INTERFACE)
target_include_directories(lwip_config INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/lwip
)

# ============================================================================
# tinyusb (vendor library)
#
# Built from the vendor's own helper file (libs/tinyusb/src/CMakeLists.txt),
# NOT via add_subdirectory() into the submodule. Submodule itself untouched.
# ============================================================================
add_library(tinyusb STATIC)

# Vendor helper: tinyusb_sources_get()/tinyusb_target_add(). Adds the
# standard TinyUSB source list (common + device + host + typec) and public
# include dirs (already includes "lib/networking" for rndis_protocol.h,
# required by class/net/ecm_rndis_device.c). Host/typec sources compile but
# are inert, guarded internally by CFG_TUH_ENABLED (0 in tusb_config.h).
include(${CMAKE_CURRENT_SOURCE_DIR}/../libs/tinyusb/src/CMakeLists.txt)
tinyusb_target_add(tinyusb)

# Device Controller Driver (DCD): intentionally NOT included by
# tinyusb_target_add() (vendor comment: "DCD and HCD drivers are not
# included") - hardware-specific, picked by the integrator. This board has
# USB_DRD_FS (STM32H5, Full Speed only), port name "stm32_fsdev".
target_sources(tinyusb PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/tinyusb/src/portable/st/stm32_fsdev/dcd_stm32_fsdev.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/tinyusb/src/portable/st/stm32_fsdev/fsdev_common.c
)

target_link_libraries(tinyusb PUBLIC tusb_config RTOS2)

# ============================================================================
# lwIP (vendor library)
#
# Filelists.cmake (vendor's own file) auto-creates lwipcore/lwipallapps/
# lwipmbedtls targets (EXCLUDE_FROM_ALL) when included, but those are NOT
# used: lwipcore's source list includes IPv6 (LWIP_IPV6 disabled in this
# project), and lwipallapps includes every lwIP app (snmp, http, iperf,
# smtp, sntp, mdns, netbios, tftp) when only mqtt is needed. Only the
# *_SRCS variables it defines are used to build a single "lwip" target
# with exactly the sources this project needs.
# ============================================================================
set(LWIP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../libs/stm32-mw-lwip)
set(LWIP_EXCLUDE_SLIPIF TRUE)  # SLIP driver not used (USB netif only)

include(${LWIP_DIR}/src/Filelists.cmake)

add_library(lwip STATIC)
target_sources(lwip PRIVATE
    ${lwipcore_SRCS}
    ${lwipcore4_SRCS}
    ${lwipapi_SRCS}
    ${lwipnetif_SRCS}
    ${lwipmqtt_SRCS}
    ${LWIP_DIR}/system/OS/sys_arch.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lwip/lwip_port_glue.c
)

# src/include: lwIP's own public headers (lwip/*.h, netif/*.h).
# system: this project's lwIP OS abstraction port (arch/cc.h,
# arch/sys_arch.h, system/OS/sys_arch.c), CMSIS-OS2 based.
target_include_directories(lwip PUBLIC
    ${LWIP_DIR}/src/include
    ${LWIP_DIR}/system
)

target_link_libraries(lwip PUBLIC lwip_config RTOS2)

# ============================================================================
# usb_cdc (components/usb) - CDC shell transport over TinyUSB
# ============================================================================
add_library(usb_cdc STATIC)
target_sources(usb_cdc PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/usb/usb_cdc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/usb/config/usb_descriptors.c
)
target_include_directories(usb_cdc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/usb
)
target_link_libraries(usb_cdc PUBLIC
    tinyusb
    tusb_config
    cqueue
    logger
    board
)
target_link_libraries(usb_cdc PRIVATE
    RTOS2
)

# ============================================================================
# usb_netif (components/netif) - lwIP netif driver over TinyUSB ECM/RNDIS
# ============================================================================
add_library(usb_netif STATIC)
target_sources(usb_netif PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/netif/usb_netif.c
)
target_include_directories(usb_netif PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/netif
)
target_link_libraries(usb_netif PUBLIC
    tinyusb
    lwip
    lwip_config
    logger
    board
)

# ============================================================================
# components - INTERFACE umbrella target grouping all component sub-targets.
# Used by USB_ETH_SDK/CMakeLists.txt so the top-level file does not need to
# know the individual sub-target names. Add new component targets here.
# ============================================================================
add_library(components INTERFACE)
target_link_libraries(components INTERFACE
    tusb_config
    lwip_config
    tinyusb
    lwip
    usb_cdc
    usb_netif
)