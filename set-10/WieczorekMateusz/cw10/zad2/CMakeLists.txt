cmake_minimum_required(VERSION 2.8)

set(CMAKE_VERBOSE_MAKEFILE 0)
project(zad2 C)
add_compile_options("-Wall")

add_executable(datagram-server server.c common.h)
add_executable(datagram-client client.c common.h)
target_link_libraries(datagram-server pthread)
target_link_libraries(datagram-client pthread)