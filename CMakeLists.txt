cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME USB_ETH_V1)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# ADD USER SOURCES HERE
add_subdirectory(USB_ETH_SDK)
# ADD USER SOURCES HERE

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    # rndis_reports.c: TinyUSB's own vendor glue for the RNDIS class driver
    # (implements rndis_class_set_handler(), called from ecm_rndis_device.c
    # inside libtinyusb.a). Compiled directly into the executable, NOT into
    # any static library archive: it needs symbols from both libtinyusb.a
    # (tusb.h) and libusb_cdc.a (tud_network_mac_address), which would form
    # a real circular dependency between static archives if it were placed
    # in either one - GNU ld resolves archive members lazily (only on
    # already-outstanding undefined references, scanned strictly left to
    # right, no second pass), so no single link order can satisfy a true
    # cycle between two archives. A plain object file has no such lazy
    # extraction: its definitions become available unconditionally, so as
    # long as this object is placed before libtinyusb.a in the link line
    # (true here - executable sources are emitted before USB_ETH_SDK_LIBS),
    # rndis_class_set_handler resolves correctly. See mục 8 of the v4/v5
    # handoff notes for the group-flag attempts that did not work.
    USB_ETH_SDK/libs/tinyusb/lib/networking/rndis_reports.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    USB_ETH_SDK/components/usb/config
    USB_ETH_SDK/libs/tinyusb/src
    USB_ETH_SDK/libs/tinyusb/lib/networking
    USB_ETH_SDK/libs/stm32-mw-lwip/src/include
    USB_ETH_SDK/libs/stm32-mw-lwip/system
    USB_ETH_SDK/components/lwip
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    -Wl,--start-group
    USB_ETH_SDK_LIBS
    -Wl,--end-group
    # Add user defined libraries
)
