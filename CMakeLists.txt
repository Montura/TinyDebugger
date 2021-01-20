cmake_minimum_required(VERSION 3.12)
project(debugger)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(linenoise)

add_executable(debugger main.cpp debugger.cpp utils.cpp)
target_link_libraries(debugger PRIVATE linenoise)

if (UNIX)
  target_link_libraries(debugger PRIVATE "-lpthread")
endif()