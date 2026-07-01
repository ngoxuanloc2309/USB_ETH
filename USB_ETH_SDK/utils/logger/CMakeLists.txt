cmake_minimum_required(VERSION 3.22)

enable_language(C)

add_library(logger STATIC)

target_sources(logger PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/logger.c)

target_include_directories(logger PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(logger PRIVATE)