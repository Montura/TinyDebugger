cmake_minimum_required(VERSION 3.12)
project(debugger)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(thirdparty/linenoise)
add_subdirectory(thirdparty/libelfin)

include_directories(${PROJECT_NAME} include thirdparty/linenoise/include thirdparty/libelfin/dwarf thirdparty/libelfin/elf)
set(SOURCE_FILES
    src/main.cpp
    src/debugger.cpp
    src/ptrace_impl.cpp
    src/breakpoint.cpp
    src/registers.cpp
    src/symbol.cpp
    src/expression_context.cpp)

add_executable(debugger ${SOURCE_FILES})
target_link_libraries(debugger PRIVATE thirdparty/linenoise libdwarf libelf)

if (UNIX)
  target_link_libraries(debugger PRIVATE "-lpthread")
endif ()