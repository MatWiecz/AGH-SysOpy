cmake_minimum_required(VERSION 2.8)

set(CMAKE_VERBOSE_MAKEFILE 1)
project(my-find-cmd C)
add_compile_options("-Wall")

add_executable(my-find-cmd main.c)