cmake_minimum_required(VERSION 3.22)

enable_language(C CXX ASM)

set(CMAKE_LIBS_NAME USB_ETH_SDK_LIBS)

add_library(${CMAKE_LIBS_NAME} INTERFACE)

add_subdirectory(services)
add_subdirectory(board)
add_subdirectory(components)
add_subdirectory(app)
add_subdirectory(utils/cqueue)
add_subdirectory(utils/logger)


target_include_directories(${CMAKE_LIBS_NAME} INTERFACE)

# tinyusb calls back into this project's own code in more than one place:
#   - class/net/ecm_rndis_device.c calls rndis_class_set_handler()
#     (usb_netif target, lib/networking/rndis_reports.c)
#   - device/usbd.c calls tud_descriptor_device_cb() /
#     _configuration_cb() / _string_cb() (usb_cdc target,
#     usb_descriptors.c)
#   - usb_netif.c itself also references tud_network_mac_address
#     (also defined in usb_cdc's usb_descriptors.c)
# Each of these is a circular reference at the static-archive level:
# libtinyusb.a needs symbols from libusb_netif.a/libusb_cdc.a, but the
# computed link order (following the PUBLIC dependency direction,
# usb_netif/usb_cdc -> tinyusb) places tinyusb AFTER them. A plain
# left-to-right link only does a single pass over static archives and
# never revisits ones it already scanned, so none of these resolve
# (verified against real failed build logs, more than once).
#
# Fix: list "components" (the umbrella INTERFACE target that already
# pulls in usb_netif, usb_cdc and tinyusb together) a second time. By
# the time ld reaches this second entry, it already knows it needs
# all of the above symbols (from having just scanned tinyusb the
# first time through), so this second pass over
# libusb_netif.a/libusb_cdc.a actually pulls those objects in. Avoids
# requiring CMake >= 3.24 (LINK_GROUP) and keeps the fix entirely
# inside USB_ETH_SDK, not the CubeMX-generated root CMakeLists.txt.
#
# This is a targeted patch, not a general solution: if a future
# module introduces another callback-from-tinyusb-into-a-component
# situation in a library NOT already covered by "components", it will
# need to be added here too.
target_link_libraries(${CMAKE_LIBS_NAME} INTERFACE
    app
    board
    components
    services
    logger
    cqueue
    components
)