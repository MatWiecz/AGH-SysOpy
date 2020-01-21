cmake_minimum_required(VERSION 2.8)

set(CMAKE_VERBOSE_MAKEFILE 1)
project(test C)
add_compile_options("-Wall")

add_executable(main main.c)
add_library(memory-manager memory-manager.c)

target_link_libraries(main memory-manager)