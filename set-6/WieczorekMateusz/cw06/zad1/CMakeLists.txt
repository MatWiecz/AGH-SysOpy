cmake_minimum_required(VERSION 2.8)

set(CMAKE_VERBOSE_MAKEFILE 1)
project(zad1 C)
add_compile_options("-Wall")

add_executable(my-chat-sv-server server.c common.c)
add_executable(my-chat-sv-client client.c common.c)