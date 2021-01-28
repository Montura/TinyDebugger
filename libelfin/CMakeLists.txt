# https://github.com/TartanLlama/libelfin/tree/fbreg
cmake_minimum_required(VERSION 3.12)
project(libelfin)

set(CMAKE_C_STANDARD 11)

add_subdirectory(dwarf)
add_subdirectory(elf)
