cmake_minimum_required(VERSION 3.12)
project(dwarf)

set(CMAKE_C_STANDARD 11)

set(SOURCE_FILES
    abbrev.cc
    attrs.cc
    cursor.cc
    die.cc
    die_str_map.cc
    dwarf.cc
    elf.cc
    expr.cc
    line.cc
    loclist.cc
    rangelist.cc
    to_string.cc
    value.cc)

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})
add_library(libdwarf ALIAS ${PROJECT_NAME})
