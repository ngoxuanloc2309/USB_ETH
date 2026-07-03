add_library(tcp_server STATIC
    tcp_server/tcp_server.c
)

target_include_directories(tcp_server
    PUBLIC
        tcp_server
)

target_link_libraries(tcp_server
    PUBLIC
        stm32cubemx
        lwipcore
        logger
        synaptix_cqueue
)

add_library(mqtt_service STATIC
    mqtt_service/mqtt_service.c
)

target_include_directories(mqtt_service
    PUBLIC
        mqtt_service
)

target_link_libraries(mqtt_service
    PUBLIC
        tcp_server
)

add_library(http_service STATIC
    http_service/http_service.c
)

target_include_directories(http_service
    PUBLIC
        http_service
)

target_link_libraries(http_service
    PUBLIC
        tcp_server
)