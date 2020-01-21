cmake_minimum_required(VERSION 2.8)

set(CMAKE_VERBOSE_MAKEFILE 1)
project(memory-manager C)
add_compile_options("-Wall")

add_library(memory-manager-static STATIC memory-manager.c)
add_library(memory-manager-shared SHARED memory-manager.c)