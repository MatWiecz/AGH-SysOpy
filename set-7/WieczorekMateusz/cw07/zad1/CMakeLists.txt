cmake_minimum_required(VERSION 2.8)

set(CMAKE_VERBOSE_MAKEFILE 1)
project(zad1 C)
add_compile_options("-Wall")

add_executable(sv-trucker trucker.c common.c)
add_executable(sv-loader loader.c common.c)
add_executable(loader-spawner loaders-spawner.c)