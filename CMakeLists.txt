cmake_minimum_required(VERSION 3.15)
project(qe6502-tools VERSION 0.1.0 LANGUAGES C)

include(CTest)

set(QE6502_BUILD_STATIC ON)
set(QE6502_BUILD_SHARED OFF)
set(QE6502_BUILD_CPP OFF)
set(QE6502_BUILD_TESTS OFF)
set(QE6502_BUILD_TOOLS OFF)
set(QE6502_BUILD_CSHARP OFF)
set(QE6502_BUILD_RUST OFF)
set(QE6502_BUILD_JAVA OFF)
set(QE6502_BUILD_PYTHON OFF)
set(QE6502_INSTALL OFF)

add_subdirectory(extern/qe6502)

add_library(qe6502_tools_sokol INTERFACE)
target_include_directories(qe6502_tools_sokol INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/extern/sokol"
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
add_subdirectory(appleii)
