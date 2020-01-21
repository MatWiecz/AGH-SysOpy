cmake_minimum_required(VERSION 2.8)

set(CMAKE_VERBOSE_MAKEFILE 0)
project(zad1 C)
add_compile_options("-Wall")

add_executable(stream-server server.c common.h)
add_executable(stream-client client.c common.h)
target_link_libraries(stream-server pthread)
target_link_libraries(stream-client pthread)