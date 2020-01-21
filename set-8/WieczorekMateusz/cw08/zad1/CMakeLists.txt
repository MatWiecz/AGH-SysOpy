cmake_minimum_required(VERSION 2.8)

set(CMAKE_VERBOSE_MAKEFILE 0)
project(zad1 C)
add_compile_options("-Wall")

add_executable(image-filterer main.c)
target_link_libraries(image-filterer pthread)