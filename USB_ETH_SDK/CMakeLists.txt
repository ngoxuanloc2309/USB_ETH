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

# usb_netif (inside components) and tinyusb (also inside components) have a
# circular reference at the static-archive level: libtinyusb.a's
# ecm_rndis_device.c calls rndis_class_set_handler(), which only exists in
# libusb_netif.a (lib/networking/rndis_reports.c, built as part of the
# usb_netif target - see components/CMakeLists.txt). A plain left-to-right
# link order places libusb_netif.a before libtinyusb.a and never resolves
# it, because ld only does a single left-to-right pass over static
# archives and does not revisit ones it already scanned (verified against
# a real failed build log).
#
# Fix: list usb_netif a second time, after components (which already
# pulls in tinyusb). By the time ld reaches this second usb_netif entry,
# it already knows it needs rndis_class_set_handler (from having just
# scanned tinyusb), so this second pass over libusb_netif.a actually pulls
# the object in. This avoids requiring CMake >= 3.24 (LINK_GROUP) and
# keeps the fix entirely inside USB_ETH_SDK, not the CubeMX-generated root
# CMakeLists.txt. Downside (documented on purpose): this is a targeted
# patch for this one circular reference - if a future module introduces
# another callback-from-tinyusb-into-a-component situation, it will need
# its own similar re-listing, this does not generalize automatically.
target_link_libraries(${CMAKE_LIBS_NAME} INTERFACE
    app
    board
    components
    services
    logger
    cqueue
    usb_netif
)