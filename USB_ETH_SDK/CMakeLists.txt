cmake_minimum_required(VERSION 3.22)

enable_language(C CXX ASM)

set(CMAKE_LIBS_NAME synaptix)

add_library(${CMAKE_LIBS_NAME} INTERFACE)

add_subdirectory(services)
add_subdirectory(board)
add_subdirectory(components)
add_subdirectory(app)
add_subdirectory(utils/cqueue)
add_subdirectory(utils/logger)


target_include_directories(${CMAKE_LIBS_NAME} INTERFACE)

target_link_libraries(${CMAKE_LIBS_NAME} INTERFACE
    app
    board
    components
    services
    logger
    cqueue
)