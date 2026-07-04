cmake_minimum_required(VERSION 3.22)

enable_language(C)

# ============================================================================
# cli_shell (services/shell) - USB CDC command-line shell
# ============================================================================
add_library(cli_shell STATIC)
target_sources(cli_shell PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/shell/cli_shell.c
)
target_include_directories(cli_shell PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/shell
)
target_link_libraries(cli_shell PUBLIC
    usb_cdc
    logger
)

# ============================================================================
# mqtt_service (services/mqtt_service) - MQTT client over lwIP
# ============================================================================
add_library(mqtt_service STATIC)
target_sources(mqtt_service PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/mqtt_service/mqtt_service.c
)
target_include_directories(mqtt_service PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/mqtt_service
)
target_link_libraries(mqtt_service PUBLIC
    lwip
    lwip_config
    RTOS2
    logger
)

# ============================================================================
# socketclient / socketserver (services/socket_client, services/socket_server)
#
# socketclient.h includes socketserver.h (only for the SOCKET_MODE_t/enum
# MODE type, no function calls) and socketserver.c includes socketclient.h
# (only for the SocketClient_t type, no function calls). This is a type-only
# cross-reference, NOT a real call dependency in either direction, so it is
# resolved with cross target_include_directories, deliberately WITHOUT
# target_link_libraries between the two - linking them would create a real
# cycle in CMake's dependency graph.
# ============================================================================
add_library(socketclient STATIC)
target_sources(socketclient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/socket_client/socketclient.c
)
target_include_directories(socketclient PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/socket_client
)
target_include_directories(socketclient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/socket_server
)
target_link_libraries(socketclient PUBLIC
    lwip
    lwip_config
    RTOS2
    logger
)

add_library(socketserver STATIC)
target_sources(socketserver PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/socket_server/socketserver.c
)
target_include_directories(socketserver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/socket_server
)
target_include_directories(socketserver PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/socket_client
)
target_link_libraries(socketserver PUBLIC
    lwip
    lwip_config
    RTOS2
    logger
)

# ============================================================================
# tcp_server (services/tcp_server) - real caller of socket_server API
# (socket_server_init()/socket_server_start()), one-directional, no cycle.
# ============================================================================
add_library(tcp_server STATIC)
target_sources(tcp_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tcp_server/tcp_server.c
)
target_include_directories(tcp_server PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/tcp_server
)
target_link_libraries(tcp_server PUBLIC
    socketserver
    logger
)

# services/http_service: NOT built on purpose - http_service.c/.h are 0
# bytes (not implemented yet). Add here when there is real code.

# ============================================================================
# services - INTERFACE umbrella target grouping all service sub-targets.
# Used by USB_ETH_SDK/CMakeLists.txt so the top-level file does not need to
# know the individual sub-target names. Add new service targets here.
# Note: socketclient/socketserver are cross-referenced via include dirs only
# (no target_link_libraries between them) to avoid a real link cycle; both
# are still listed here since both are real buildable static libs.
# ============================================================================
add_library(services INTERFACE)
target_link_libraries(services INTERFACE
    cli_shell
    mqtt_service
    socketclient
    socketserver
    tcp_server
)