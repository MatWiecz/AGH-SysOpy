cmake_minimum_required(VERSION 2.8)

set(CMAKE_VERBOSE_MAKEFILE 1)
project(monitor C)
add_compile_options("-Wall")

add_executable(monitor main.c)
add_executable(tester test.c)
