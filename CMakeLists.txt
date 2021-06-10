cmake_minimum_required(VERSION 3.17)
project(mytar C)

set(CMAKE_C_STANDARD 99)
# set gcc flags
set(GCC_COVERAGE_COMPILE_FLAGS "-Wall -Wextra -Wconversion -Wshadow -Wpedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )

add_executable(mytar main.c)