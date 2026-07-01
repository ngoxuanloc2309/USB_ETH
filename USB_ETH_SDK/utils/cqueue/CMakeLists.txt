cmake_minimum_required(VERSION 3.22)

enable_language(C)

add_library(cqueue STATIC)

target_sources(cqueue PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cqueue.c)

target_include_directories(cqueue PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(cqueue PRIVATE)