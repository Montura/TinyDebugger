# https://github.com/antirez/linenoise
cmake_minimum_required(VERSION 3.12)
project(linenoise)

set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC linenoise.c)
add_library(linenoise ALIAS ${PROJECT_NAME})
