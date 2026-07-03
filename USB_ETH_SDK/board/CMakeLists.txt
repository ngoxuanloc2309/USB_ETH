cmake_minimum_required(VERSION 3.22)

enable_language(C)

add_library(board STATIC)

target_sources(board PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/board.c
)

target_include_directories(board PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# PUBLIC on purpose: board.h itself pulls in gpio.h/usart.h/stm32h5xx_hal.h
# (stm32cubemx), tusb.h (tinyusb + tusb_config), so any consumer of board.h
# needs these include paths too, not just board.c.
target_link_libraries(board PUBLIC
    stm32cubemx
    RTOS2
    tinyusb
    tusb_config
    app_config
    logger
)