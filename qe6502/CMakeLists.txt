# /CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(qe6502 VERSION 0.1.0 LANGUAGES C)

include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(QE6502_DEFAULT_BUILD_TESTS ${BUILD_TESTING})
    set(QE6502_DEFAULT_INSTALL ON)
else()
    set(QE6502_DEFAULT_BUILD_TESTS OFF)
    set(QE6502_DEFAULT_INSTALL OFF)
endif()

option(QE6502_BUILD_WASM "Enable WebAssembly target mode" OFF)
option(QE6502_BUILD_STATIC "Build the fast static qe6502 C library" ON)
option(QE6502_BUILD_SHARED "Build the stable shared qe6502 ABI library" ON)
option(QE6502_BUILD_CPP "Build the qe6502 C++ wrapper" ON)
option(QE6502_BUILD_TESTS "Build qe6502 tests and harnesses" ${QE6502_DEFAULT_BUILD_TESTS})
option(QE6502_ENABLE_WERROR "Treat qe6502 warnings as errors" OFF)
option(QE6502_GENERATE_ASSEMBLY "Generate compiler assembly/intermediate files for inspection" OFF)
option(QE6502_INSTALL "Install qe6502 headers, libraries and package metadata" ${QE6502_DEFAULT_INSTALL})

if(QE6502_BUILD_WASM AND NOT QE6502_BUILD_STATIC)
    message(FATAL_ERROR "QE6502_BUILD_WASM requires QE6502_BUILD_STATIC because the JS/WASM bindings link the static core")
endif()

if(QE6502_BUILD_CPP AND NOT QE6502_BUILD_STATIC)
    message(FATAL_ERROR "QE6502_BUILD_CPP requires QE6502_BUILD_STATIC")
endif()

if(NOT QE6502_BUILD_STATIC AND NOT QE6502_BUILD_SHARED)
    message(FATAL_ERROR "At least one of QE6502_BUILD_STATIC or QE6502_BUILD_SHARED must be enabled")
endif()

if(QE6502_BUILD_WASM AND QE6502_BUILD_CPP)
    message(STATUS "Disabling QE6502_BUILD_CPP because QE6502_BUILD_WASM is enabled")
    set(QE6502_BUILD_CPP OFF CACHE BOOL "Build the qe6502 C++ wrapper" FORCE)
endif()

if(QE6502_BUILD_TESTS AND NOT QE6502_BUILD_WASM)
    if(NOT QE6502_BUILD_STATIC OR NOT QE6502_BUILD_SHARED OR NOT QE6502_BUILD_CPP)
        message(FATAL_ERROR "Native QE6502_BUILD_TESTS requires QE6502_BUILD_STATIC, QE6502_BUILD_SHARED and QE6502_BUILD_CPP")
    endif()
endif()

if(QE6502_BUILD_TESTS AND QE6502_BUILD_WASM AND NOT QE6502_BUILD_STATIC)
    message(FATAL_ERROR "WASM QE6502_BUILD_TESTS requires QE6502_BUILD_STATIC")
endif()

if(QE6502_GENERATE_ASSEMBLY)
    add_compile_options(
        $<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang>:-save-temps=obj>
        $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-save-temps=obj>
        $<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang>:-fverbose-asm>
        $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-fverbose-asm>
    )
endif()

function(qe6502_configure_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            $<$<BOOL:${QE6502_ENABLE_WERROR}>:/WX>
            $<$<BOOL:${QE6502_ENABLE_WERROR}>:/we4505>
        )
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic
            -Wconversion -Wsign-conversion -Wshadow
            -Wdouble-promotion -Wformat=2 -Wundef
            -Wunused-function
            $<$<BOOL:${QE6502_ENABLE_WERROR}>:-Werror>
            $<$<BOOL:${QE6502_ENABLE_WERROR}>:-Werror=unused-function>
        )
    endif()
endfunction()

message(STATUS "  QE6502_BUILD_WASM        = ${QE6502_BUILD_WASM}")
message(STATUS "  QE6502_BUILD_STATIC      = ${QE6502_BUILD_STATIC}")
message(STATUS "  QE6502_BUILD_SHARED      = ${QE6502_BUILD_SHARED}")
message(STATUS "  QE6502_BUILD_CPP         = ${QE6502_BUILD_CPP}")
message(STATUS "  QE6502_BUILD_TESTS       = ${QE6502_BUILD_TESTS}")
message(STATUS "  QE6502_ENABLE_WERROR     = ${QE6502_ENABLE_WERROR}")
message(STATUS "  QE6502_GENERATE_ASSEMBLY = ${QE6502_GENERATE_ASSEMBLY}")
message(STATUS "  QE6502_INSTALL           = ${QE6502_INSTALL}")

add_subdirectory(cpu)
add_subdirectory(binds)

if(NOT QE6502_BUILD_WASM)
    add_subdirectory(tools)
endif()

if(QE6502_BUILD_TESTS)
    enable_testing()
    add_subdirectory(harness)
endif()

if(QE6502_INSTALL)
    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/qe6502Config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/qe6502Config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/qe6502
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/qe6502ConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(EXPORT qe6502Targets
        NAMESPACE qe6502::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/qe6502
    )

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/qe6502Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/qe6502ConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/qe6502
    )

    file(RELATIVE_PATH QE6502_PC_PREFIX_FROM_PKGCONFIG_DIR
        "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig"
        "${CMAKE_INSTALL_PREFIX}"
    )

    if(QE6502_PC_PREFIX_FROM_PKGCONFIG_DIR STREQUAL "")
        set(QE6502_PC_PREFIX_FROM_PKGCONFIG_DIR ".")
    endif()

    if(IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}")
        set(QE6502_PC_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}")
    else()
        set(QE6502_PC_INCLUDEDIR "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
    endif()

    if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
        set(QE6502_PC_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
    else()
        set(QE6502_PC_LIBDIR "\${prefix}/${CMAKE_INSTALL_LIBDIR}")
    endif()

    if(QE6502_BUILD_STATIC)
        configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/qe6502.pc.in
            ${CMAKE_CURRENT_BINARY_DIR}/qe6502.pc
            @ONLY
        )
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/qe6502.pc
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
        )
    endif()

    if(QE6502_BUILD_SHARED)
        configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/qe6502-abi.pc.in
            ${CMAKE_CURRENT_BINARY_DIR}/qe6502-abi.pc
            @ONLY
        )
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/qe6502-abi.pc
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
        )
    endif()

    if(QE6502_BUILD_CPP)
        configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/qe6502-cpp.pc.in
            ${CMAKE_CURRENT_BINARY_DIR}/qe6502-cpp.pc
            @ONLY
        )
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/qe6502-cpp.pc
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
        )
    endif()
endif()
