# /CMakeLists.txt
cmake_minimum_required(VERSION 3.15)

set(QE6502_VERSION_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/cpu/include/qe6502/qe6502_version.h")
file(READ "${QE6502_VERSION_HEADER}" QE6502_VERSION_HEADER_TEXT)

function(qe6502_read_uint_define out_var define_name)
    string(REGEX MATCH
        "#[ \t]*define[ \t]+${define_name}[ \t]+([0-9]+)[uU]?"
        _qe6502_define_match
        "${QE6502_VERSION_HEADER_TEXT}"
    )

    if(NOT _qe6502_define_match)
        message(FATAL_ERROR "Failed to parse ${define_name} from ${QE6502_VERSION_HEADER}")
    endif()

    set(${out_var} "${CMAKE_MATCH_1}" PARENT_SCOPE)
endfunction()

qe6502_read_uint_define(QE6502_VERSION_MAJOR QE6502_VERSION_MAJOR)
qe6502_read_uint_define(QE6502_VERSION_MINOR QE6502_VERSION_MINOR)
set(QE6502_PACKAGE_VERSION "${QE6502_VERSION_MAJOR}.${QE6502_VERSION_MINOR}.0")
project(qe6502 VERSION "${QE6502_PACKAGE_VERSION}" LANGUAGES C)

include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(QE6502_DEFAULT_BUILD_TESTS ${BUILD_TESTING})
    set(QE6502_DEFAULT_BUILD_TOOLS ${BUILD_TESTING})
    set(QE6502_DEFAULT_INSTALL ON)
else()
    set(QE6502_DEFAULT_BUILD_TESTS OFF)
    set(QE6502_DEFAULT_BUILD_TOOLS OFF)
    set(QE6502_DEFAULT_INSTALL OFF)
endif()

option(QE6502_BUILD_WASM "Enable WebAssembly target mode" OFF)
option(QE6502_BUILD_STATIC "Build the fast static qe6502 C library" ON)
option(QE6502_BUILD_SHARED "Build the stable shared qe6502 ABI library" ON)
option(QE6502_BUILD_CPP "Build the qe6502 C++ wrapper" ON)
option(QE6502_BUILD_TOOLS "Build qe6502 developer tools and tool support libraries" ${QE6502_DEFAULT_BUILD_TOOLS})
option(QE6502_BUILD_CSHARP "Build the qe6502 C# binding when dotnet is available" ON)
option(QE6502_BUILD_RUST "Build the qe6502 Rust binding when Cargo is available" ON)
option(QE6502_BUILD_JAVA "Build the qe6502 Java binding when Java 25+ is available" ON)
option(QE6502_BUILD_PYTHON "Build the qe6502 Python binding when Python development headers are available" ON)
option(QE6502_BUILD_TESTS "Build qe6502 tests and harnesses" ${QE6502_DEFAULT_BUILD_TESTS})
option(QE6502_ENABLE_WERROR "Treat qe6502 warnings as errors" OFF)
option(QE6502_GENERATE_ASSEMBLY "Generate compiler assembly/intermediate files for inspection" OFF)
option(QE6502_INSTALL "Install qe6502 headers, libraries and package metadata" ${QE6502_DEFAULT_INSTALL})

if(QE6502_BUILD_WASM AND NOT QE6502_BUILD_STATIC)
    message(FATAL_ERROR "QE6502_BUILD_WASM requires QE6502_BUILD_STATIC because the JS/WASM bindings link the static core")
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
    if(NOT QE6502_BUILD_TOOLS)
        message(FATAL_ERROR "Native QE6502_BUILD_TESTS requires QE6502_BUILD_TOOLS because lockstep and tool smoke harnesses link tools6502")
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

message(STATUS "  QE6502_PACKAGE_VERSION   = ${QE6502_PACKAGE_VERSION}")
message(STATUS "  QE6502_BUILD_WASM        = ${QE6502_BUILD_WASM}")
message(STATUS "  QE6502_BUILD_STATIC      = ${QE6502_BUILD_STATIC}")
message(STATUS "  QE6502_BUILD_SHARED      = ${QE6502_BUILD_SHARED}")
message(STATUS "  QE6502_BUILD_CPP         = ${QE6502_BUILD_CPP}")
message(STATUS "  QE6502_BUILD_TOOLS       = ${QE6502_BUILD_TOOLS}")
message(STATUS "  QE6502_BUILD_CSHARP      = ${QE6502_BUILD_CSHARP}")
message(STATUS "  QE6502_BUILD_RUST        = ${QE6502_BUILD_RUST}")
message(STATUS "  QE6502_BUILD_JAVA        = ${QE6502_BUILD_JAVA}")
message(STATUS "  QE6502_BUILD_PYTHON      = ${QE6502_BUILD_PYTHON}")
message(STATUS "  QE6502_BUILD_TESTS       = ${QE6502_BUILD_TESTS}")
message(STATUS "  QE6502_ENABLE_WERROR     = ${QE6502_ENABLE_WERROR}")
message(STATUS "  QE6502_GENERATE_ASSEMBLY = ${QE6502_GENERATE_ASSEMBLY}")
message(STATUS "  QE6502_INSTALL           = ${QE6502_INSTALL}")

set(QE6502_WASM_EXPORT_MEMORY_LINK_OPTION)
if(QE6502_BUILD_WASM)
    execute_process(
        COMMAND "${CMAKE_LINKER}" --help
        OUTPUT_VARIABLE QE6502_WASM_LD_HELP
        ERROR_VARIABLE QE6502_WASM_LD_HELP_ERROR
        RESULT_VARIABLE QE6502_WASM_LD_HELP_RESULT
    )

    if(QE6502_WASM_LD_HELP MATCHES "--export-memory" OR QE6502_WASM_LD_HELP_ERROR MATCHES "--export-memory")
        list(APPEND QE6502_WASM_EXPORT_MEMORY_LINK_OPTION LINKER:--export-memory)
        message(STATUS "  QE6502_WASM_EXPORT_MEMORY = ON")
    else()
        message(STATUS "  QE6502_WASM_EXPORT_MEMORY = OFF (linker does not support --export-memory)")
    endif()
endif()

add_subdirectory(cpu)
add_subdirectory(binds)

if(NOT QE6502_BUILD_WASM AND QE6502_BUILD_TOOLS)
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

    install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
        DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/qe6502
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
        if(QE6502_BUILD_SHARED AND NOT QE6502_BUILD_STATIC)
            set(QE6502_CPP_PC_LIBS "-L\${libdir} -lqe6502_cpp")
        else()
            set(QE6502_CPP_PC_LIBS "-L\${libdir} -lqe6502_cpp_static -lqe6502_static")
        endif()

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
