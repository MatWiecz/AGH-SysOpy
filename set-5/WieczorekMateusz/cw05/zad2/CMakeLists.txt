cmake_minimum_required(VERSION 2.8)

set(CMAKE_VERBOSE_MAKEFILE 1)
project(zad2 C)
add_compile_options("-Wall")

add_executable(master master.c)
add_executable(slave slave.c)