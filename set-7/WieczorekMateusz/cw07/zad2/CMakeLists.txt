cmake_minimum_required(VERSION 2.8)

set(CMAKE_VERBOSE_MAKEFILE 1)
project(zad2 C)
add_compile_options("-Wall")

add_executable(posix-trucker trucker.c common.c)
target_link_libraries(posix-trucker pthread rt)
add_executable(posix-loader loader.c common.c)
target_link_libraries(posix-loader pthread rt)
add_executable(loader-spawner loaders-spawner.c)