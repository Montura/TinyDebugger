cmake_minimum_required(VERSION 3.12)
project(debugger)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(linenoise)
#add_subdirectory(libelfin)

include_directories(${PROJECT_NAME} include linenoise/include)
set(SOURCE_FILES
    src/main.cpp
    src/debugger.cpp
    src/ptrace_impl.cpp
    src/breakpoint.cpp
    src/registers.cpp
)

add_executable(debugger ${SOURCE_FILES})
target_link_libraries(debugger PRIVATE linenoise libdwarf libelf)

if (UNIX)
  target_link_libraries(debugger PRIVATE "-lpthread")
endif ()