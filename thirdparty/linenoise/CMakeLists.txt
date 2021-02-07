# https://github.com/antirez/linenoise
cmake_minimum_required(VERSION 3.12)
project(linenoise)

set(CMAKE_C_STANDARD 11)

include_directories(${PROJECT_NAME} include)
set(SOURCE_FILES src/linenoise.c)

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})
add_library(linenoise ALIAS ${PROJECT_NAME})
