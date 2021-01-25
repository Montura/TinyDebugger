cmake_minimum_required(VERSION 3.12)
project(debugger)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(linenoise)

include_directories(${PROJECT_NAME} include linenoise/include)
set(SOURCE_FILES
    src/main.cpp
    src/debugger.cpp
    src/utils.cpp
    src/breakpoint.cpp
    src/registers.cpp
)

add_executable(debugger ${SOURCE_FILES})
target_link_libraries(debugger PRIVATE linenoise)

if (UNIX)
  target_link_libraries(debugger PRIVATE "-lpthread")
endif ()