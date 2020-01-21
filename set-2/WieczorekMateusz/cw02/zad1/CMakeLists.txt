cmake_minimum_required(VERSION 2.8)

set(CMAKE_VERBOSE_MAKEFILE 1)
project(record-sorter C)
add_compile_options("-Wall")

add_executable(record-sorter main.c)