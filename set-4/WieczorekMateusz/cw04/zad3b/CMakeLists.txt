cmake_minimum_required(VERSION 2.8)

set(CMAKE_VERBOSE_MAKEFILE 1)
project(zad3b C)
add_compile_options("-Wall")

add_executable(sender sender.c)
add_executable(catcher catcher.c)
