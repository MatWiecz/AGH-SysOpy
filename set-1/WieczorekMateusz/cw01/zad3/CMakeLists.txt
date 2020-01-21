cmake_minimum_required(VERSION 2.8)

#set(CMAKE_VERBOSE_MAKEFILE 1)
project(main C)
enable_testing()
add_compile_options("-Wall")
set(CMAKE_C_STANDARD 99)

option(O1 "O1" OFF)
option(O2 "O2" OFF)
option(O3 "O3" OFF)
option(Os "Os" OFF)

if(O1)
    add_compile_options("-O1")
endif(O1)

if(O2)
    add_compile_options("-O2")
endif(O2)

if(O3)
    add_compile_options("-O3")
endif(O3)

if(Os)
    add_compile_options("-Os")
endif(Os)

SET(CMAKE_STATIC_LIBRARY_PREFIX "")
SET(CMAKE_SHARED_LIBRARY_PREFIX "")
SET(CTEST_OUTPUT_ON_FAILURE TRUE)

add_library(memory-manager-static STATIC memory-manager.c)
add_executable(main-static main.c)
target_link_libraries(main-static memory-manager-static)

add_library(memory-manager-shared SHARED memory-manager.c)
add_executable(main-shared main.c)
target_link_libraries(main-shared memory-manager-shared)

SET(EASY_TEST create_table 10 run_find . * temp load_file temp remove_block 0 load_file temp remove_block 0 load_file temp load_file temp remove_block 0 remove_block 1)
SET(MEDIUM_TEST create_table 10 run_find home * temp load_file temp remove_block 0 load_file temp remove_block 0 load_file temp load_file temp remove_block 0 remove_block 1)
SET(HARD_TEST create_table 10 run_find / * temp load_file temp remove_block 0 load_file temp remove_block 0 load_file temp load_file temp remove_block 0 remove_block 1)

add_test(StaticHard main-static ${HARD_TEST})
add_test(StaticMedium main-static ${MEDIUM_TEST})
add_test(StaticEasy main-static ${EASY_TEST})
add_test(SharedHard main-shared ${HARD_TEST})
add_test(SharedMedium main-shared ${MEDIUM_TEST})
add_test(SharedEasy main-shared ${EASY_TEST})
