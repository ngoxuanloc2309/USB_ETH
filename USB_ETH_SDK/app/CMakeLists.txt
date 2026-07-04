cmake_minimum_required(VERSION 3.22)

enable_language(C)

# ============================================================================
# app_config (INTERFACE) - exposes app/ (app.h, app_config.h)
# Consumed by: board and mqtt_app. NOT linked by shell_app or usb_cdc/
# usb_netif (they don't include app_config.h).
# app_config.h itself pulls in stm32h5xx_hal.h (stm32cubemx) and FreeRTOS.h/
# task.h (RTOS2), so those are linked here too and propagate to consumers.
# ============================================================================
add_library(app_config INTERFACE)
target_include_directories(app_config INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(app_config INTERFACE
    stm32cubemx
    RTOS2
)

# ============================================================================
# shell_app (app/user/shell_app) - shell command registration + task
# ============================================================================
add_library(shell_app STATIC)
target_sources(shell_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/user/shell_app/shell_app.c
    ${CMAKE_CURRENT_SOURCE_DIR}/user/shell_app/shell_command_lists.c
)
target_include_directories(shell_app PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/user/shell_app
)
target_link_libraries(shell_app PUBLIC
    cli_shell
    board
    RTOS2
    logger
)

# ============================================================================
# mqtt_app (app/user/mqtt_app) - MQTT application logic + task
# ============================================================================
add_library(mqtt_app STATIC)
target_sources(mqtt_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/user/mqtt_app/mqtt_app.c
)
target_include_directories(mqtt_app PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/user/mqtt_app
)
target_link_libraries(mqtt_app PUBLIC
    mqtt_service
    usb_netif
    app_config
    RTOS2
    logger
)

# app/user/http_app: NOT built on purpose - http_app.c/.h not implemented
# yet. Add here when there is real code.

# ============================================================================
# app (composition root) - board_init() -> usb_cdc_transport_init() ->
# usb_netif_init() -> shell_app_init()/start() -> mqtt_app_init()/start()
# -> board_usb_task_start()
# ============================================================================
add_library(app STATIC)
target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/app.c
)
target_include_directories(app PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(app PUBLIC
    board
    usb_cdc
    usb_netif
    shell_app
    mqtt_app
    logger
)