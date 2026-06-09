cmake_minimum_required(VERSION 3.16)

project(tools6502 LANGUAGES CXX)

if(NOT TARGET perfect6502)
    add_subdirectory(
        ${CMAKE_CURRENT_LIST_DIR}/../harness/third_party/perfect6502
        ${CMAKE_CURRENT_BINARY_DIR}/perfect6502
    )
endif()

file(GLOB_RECURSE tools6502_sources CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cc
)
list(FILTER tools6502_sources EXCLUDE REGEX [[/tests/]])
list(FILTER tools6502_sources EXCLUDE REGEX [[/cpu6502_bridge/tools/]])
list(FILTER tools6502_sources EXCLUDE REGEX [[/hello_world/]])

file(GLOB_RECURSE tools6502_headers CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)
list(FILTER tools6502_headers EXCLUDE REGEX [[/tests/]])
list(FILTER tools6502_headers EXCLUDE REGEX [[/cpu6502_bridge/tools/]])
list(FILTER tools6502_headers EXCLUDE REGEX [[/hello_world/]])

add_library(tools6502 STATIC
    ${tools6502_sources}
    ${tools6502_headers}
)
add_library(tools6502::tools6502 ALIAS tools6502)

file(GLOB_RECURSE tools6502_include_markers LIST_DIRECTORIES true CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*/include
)
set(tools6502_include_dirs)
foreach(tools6502_include_dir IN LISTS tools6502_include_markers)
    if(IS_DIRECTORY ${tools6502_include_dir})
        list(APPEND tools6502_include_dirs ${tools6502_include_dir})
    endif()
endforeach()
list(REMOVE_DUPLICATES tools6502_include_dirs)

set(tools6502_build_include_dirs)
foreach(tools6502_include_dir IN LISTS tools6502_include_dirs)
    list(APPEND tools6502_build_include_dirs
        $<BUILD_INTERFACE:${tools6502_include_dir}>
    )
endforeach()

target_include_directories(tools6502
    PUBLIC
        ${tools6502_build_include_dirs}
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(tools6502
    PRIVATE
        qe6502::static
        perfect6502
)

qe6502_configure_warnings(tools6502)
target_compile_features(tools6502 PUBLIC cxx_std_17 PRIVATE cxx_std_20)

if(QE6502_INSTALL)
    install(TARGETS tools6502
        EXPORT qe6502ToolsTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    foreach(tools6502_include_dir IN LISTS tools6502_include_dirs)
        install(DIRECTORY ${tools6502_include_dir}/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    endforeach()
endif()

if(QE6502_BUILD_TESTS)
    add_executable(asm6502_smoke
        asm6502/tests/smoke.cpp
    )
    target_link_libraries(asm6502_smoke PRIVATE tools6502::tools6502)
    qe6502_configure_warnings(asm6502_smoke)
    target_compile_features(asm6502_smoke PRIVATE cxx_std_17)
    add_test(NAME asm6502.smoke COMMAND asm6502_smoke)

    add_executable(tools6502_common_smoke
        common/tests/smoke.cpp
    )
    target_link_libraries(tools6502_common_smoke PRIVATE tools6502::tools6502)
    qe6502_configure_warnings(tools6502_common_smoke)
    target_compile_features(tools6502_common_smoke PRIVATE cxx_std_17)
    add_test(NAME tools6502.common.smoke COMMAND tools6502_common_smoke)

    add_executable(tools6502_lockstep_smoke
        lockstep/tests/smoke.cpp
    )
    target_link_libraries(tools6502_lockstep_smoke PRIVATE tools6502::tools6502)
    qe6502_configure_warnings(tools6502_lockstep_smoke)
    target_compile_features(tools6502_lockstep_smoke PRIVATE cxx_std_17)
    add_test(NAME tools6502.lockstep.smoke COMMAND tools6502_lockstep_smoke)
endif()

add_executable(cpu6502_bridge_fetch_compare
    cpu6502_bridge/tools/compare_fetch_log.cpp
)
target_link_libraries(cpu6502_bridge_fetch_compare PRIVATE tools6502::tools6502)
qe6502_configure_warnings(cpu6502_bridge_fetch_compare)
target_compile_features(cpu6502_bridge_fetch_compare PRIVATE cxx_std_17)

add_executable(tools6502_hello_world
    hello_world/hello_world.cpp
)
target_link_libraries(tools6502_hello_world PRIVATE
    tools6502::tools6502
    qe6502::static
)
qe6502_configure_warnings(tools6502_hello_world)
target_compile_features(tools6502_hello_world PRIVATE cxx_std_17)
