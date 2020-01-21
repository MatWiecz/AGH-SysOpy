cmake_minimum_required(VERSION 2.8)

set(CMAKE_VERBOSE_MAKEFILE 1)
project(zad2 C)
add_compile_options("-Wall")

add_executable(my-chat-posix-server server.c common.c)
add_executable(my-chat-posix-client client.c common.c)
target_link_libraries(my-chat-posix-server rt)
target_link_libraries(my-chat-posix-client rt)