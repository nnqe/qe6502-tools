cmake_minimum_required(VERSION 3.15)
project(qe6502-tools VERSION 0.1.0 LANGUAGES C)

include(CTest)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(QE6502_TOOLS_DEFAULT_BUILD_TESTS ${BUILD_TESTING})
else()
    set(QE6502_TOOLS_DEFAULT_BUILD_TESTS OFF)
endif()

option(QE6502_TOOLS_BUILD_TESTS "Build qe6502-tools smoke tests" ${QE6502_TOOLS_DEFAULT_BUILD_TESTS})
option(QE6502_TOOLS_BUILD_STATIC_SMOKE "Build qe6502 static-link smoke test" ${QE6502_TOOLS_BUILD_TESTS})
option(QE6502_TOOLS_BUILD_SOKOL_SMOKE "Build Sokol smoke test" ${QE6502_TOOLS_BUILD_TESTS})
option(QE6502_TOOLS_ENABLE_WERROR "Treat qe6502-tools warnings as errors" OFF)

function(qe6502_tools_configure_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            $<$<BOOL:${QE6502_TOOLS_ENABLE_WERROR}>:/WX>
        )
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic
            -Wconversion -Wsign-conversion -Wshadow
            -Wformat=2 -Wundef
            $<$<BOOL:${QE6502_TOOLS_ENABLE_WERROR}>:-Werror>
        )
    endif()
endfunction()

add_library(qe6502_tools_sokol INTERFACE)
target_include_directories(qe6502_tools_sokol SYSTEM INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/extern/sokol"
)

if(QE6502_TOOLS_BUILD_STATIC_SMOKE)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/qe6502/CMakeLists.txt")
        message(FATAL_ERROR
            "The qe6502 submodule is not initialized. Run: git submodule update --init --recursive")
    endif()

    set(QE6502_BUILD_STATIC ON CACHE BOOL "" FORCE)
    set(QE6502_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(QE6502_BUILD_CPP OFF CACHE BOOL "" FORCE)
    set(QE6502_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(QE6502_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(QE6502_INSTALL OFF CACHE BOOL "" FORCE)

    add_subdirectory(qe6502)

    add_executable(qe6502_tools_static_smoke
        tests/qe6502_tools_static_smoke.c
    )
    target_link_libraries(qe6502_tools_static_smoke PRIVATE qe6502::static)
    qe6502_tools_configure_warnings(qe6502_tools_static_smoke)

    add_test(NAME qe6502_tools.static_smoke COMMAND qe6502_tools_static_smoke)
endif()

if(QE6502_TOOLS_BUILD_SOKOL_SMOKE)
    add_executable(qe6502_tools_sokol_smoke
        tests/qe6502_tools_sokol_smoke.c
    )
    target_link_libraries(qe6502_tools_sokol_smoke PRIVATE qe6502_tools_sokol)
    qe6502_tools_configure_warnings(qe6502_tools_sokol_smoke)

    add_test(NAME qe6502_tools.sokol_smoke COMMAND qe6502_tools_sokol_smoke)
endif()
