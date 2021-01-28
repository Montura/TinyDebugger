cmake_minimum_required(VERSION 3.12)
project(elf)

set(CMAKE_C_STANDARD 11)

set(SOURCE_FILES
    elf.cc
    mmap_loader.cc
    to_string.cc)

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})
add_library(libelf ALIAS ${PROJECT_NAME})
