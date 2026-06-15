cmake_minimum_required(VERSION 3.15)

function(qe6502_run_step step_name)
    execute_process(
        COMMAND ${ARGN}
        RESULT_VARIABLE QE6502_STEP_RESULT
        OUTPUT_VARIABLE QE6502_STEP_OUTPUT
        ERROR_VARIABLE QE6502_STEP_ERROR
        ECHO_OUTPUT_VARIABLE
        ECHO_ERROR_VARIABLE
    )

    if(NOT QE6502_STEP_RESULT EQUAL 0)
        message(FATAL_ERROR "${step_name} failed with exit code ${QE6502_STEP_RESULT}")
    endif()
endfunction()

foreach(required_var IN ITEMS
    QE6502_SOURCE_DIR
    QE6502_WORK_DIR
    QE6502_CONSUMER_SOURCE_DIR
    QE6502_INSTALL_CONSUME_CASE
    QE6502_GENERATOR
)
    if(NOT DEFINED ${required_var} OR "${${required_var}}" STREQUAL "")
        message(FATAL_ERROR "${required_var} is required")
    endif()
endforeach()

function(qe6502_find_first_existing out_var)
    set(found "")
    foreach(candidate IN LISTS ARGN)
        if(EXISTS "${candidate}")
            set(found "${candidate}")
            break()
        endif()
    endforeach()
    set(${out_var} "${found}" PARENT_SCOPE)
endfunction()

function(qe6502_symbol_regex_for name out_var)
    # Match the symbol as a whole token in typical nm/llvm-nm output. The optional
    # leading underscore covers platforms/toolchains that decorate C symbols.
    set(${out_var} "(^|[ \t])_?${name}($|[ \t\r\n])" PARENT_SCOPE)
endfunction()

function(qe6502_require_export library symbols_text symbol_name)
    qe6502_symbol_regex_for("${symbol_name}" symbol_regex)
    if(NOT "${symbols_text}" MATCHES "${symbol_regex}")
        message(FATAL_ERROR
            "Required symbol '${symbol_name}' was not exported by ${library}"
        )
    endif()
endfunction()

function(qe6502_forbid_export library symbols_text symbol_name)
    qe6502_symbol_regex_for("${symbol_name}" symbol_regex)
    if("${symbols_text}" MATCHES "${symbol_regex}")
        message(FATAL_ERROR
            "Forbidden symbol '${symbol_name}' was exported by ${library}"
        )
    endif()
endfunction()

function(qe6502_read_exports library out_var)
    get_filename_component(library_ext "${library}" EXT)

    if(library_ext STREQUAL ".dll")
        # MinGW nm reports global COFF symbols from a DLL, not just the PE
        # export table. Use objdump private headers for loader-visible exports.
        find_program(QE6502_PE_EXPORT_TOOL NAMES llvm-objdump objdump)
        if(NOT QE6502_PE_EXPORT_TOOL)
            message(STATUS
                "Skipping symbol visibility checks for ${library}: "
                "llvm-objdump/objdump not found"
            )
            set(${out_var} "" PARENT_SCOPE)
            set(QE6502_SYMBOL_CHECK_AVAILABLE OFF PARENT_SCOPE)
            return()
        endif()

        get_filename_component(symbol_tool_name "${QE6502_PE_EXPORT_TOOL}" NAME)
        execute_process(
            COMMAND "${QE6502_PE_EXPORT_TOOL}" -p "${library}"
            RESULT_VARIABLE symbol_result
            OUTPUT_VARIABLE symbol_output
            ERROR_VARIABLE symbol_error
        )

        if(NOT symbol_result EQUAL 0)
            message(STATUS
                "Skipping symbol visibility checks for ${library}: "
                "${symbol_tool_name} failed with exit code ${symbol_result}: ${symbol_error}"
            )
            set(${out_var} "" PARENT_SCOPE)
            set(QE6502_SYMBOL_CHECK_AVAILABLE OFF PARENT_SCOPE)
            return()
        endif()

        set(${out_var} "${symbol_output}" PARENT_SCOPE)
        set(QE6502_SYMBOL_CHECK_AVAILABLE ON PARENT_SCOPE)
        return()
    endif()

    find_program(QE6502_SYMBOL_TOOL NAMES llvm-nm nm)
    if(NOT QE6502_SYMBOL_TOOL)
        message(STATUS "Skipping symbol visibility checks for ${library}: llvm-nm/nm not found")
        set(${out_var} "" PARENT_SCOPE)
        set(QE6502_SYMBOL_CHECK_AVAILABLE OFF PARENT_SCOPE)
        return()
    endif()

    get_filename_component(symbol_tool_name "${QE6502_SYMBOL_TOOL}" NAME)

    set(symbol_tool_args "${QE6502_SYMBOL_TOOL}")
    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
        list(APPEND symbol_tool_args -gU "${library}")
    else()
        list(APPEND symbol_tool_args -D -g --defined-only "${library}")
    endif()

    execute_process(
        COMMAND ${symbol_tool_args}
        RESULT_VARIABLE symbol_result
        OUTPUT_VARIABLE symbol_output
        ERROR_VARIABLE symbol_error
    )

    if(NOT symbol_result EQUAL 0)
        message(STATUS
            "Skipping symbol visibility checks for ${library}: "
            "${symbol_tool_name} failed with exit code ${symbol_result}: ${symbol_error}"
        )
        set(${out_var} "" PARENT_SCOPE)
        set(QE6502_SYMBOL_CHECK_AVAILABLE OFF PARENT_SCOPE)
        return()
    endif()

    set(${out_var} "${symbol_output}" PARENT_SCOPE)
    set(QE6502_SYMBOL_CHECK_AVAILABLE ON PARENT_SCOPE)
endfunction()

function(qe6502_check_installed_shared_symbols)
    if(QE6502_INSTALL_CONSUME_CASE STREQUAL "static")
        message(STATUS "Skipping shared symbol checks for static-only install")
        return()
    endif()

    set(qe6502_abi_candidates
        "${QE6502_INSTALL_PREFIX}/bin/libqe6502.dll"
        "${QE6502_INSTALL_PREFIX}/bin/qe6502.dll"
        "${QE6502_INSTALL_PREFIX}/lib/libqe6502.so"
        "${QE6502_INSTALL_PREFIX}/lib/libqe6502.dylib"
    )
    set(qe6502_cpp_candidates
        "${QE6502_INSTALL_PREFIX}/bin/libqe6502_cpp.dll"
        "${QE6502_INSTALL_PREFIX}/bin/qe6502_cpp.dll"
        "${QE6502_INSTALL_PREFIX}/lib/libqe6502_cpp.so"
        "${QE6502_INSTALL_PREFIX}/lib/libqe6502_cpp.dylib"
    )

    qe6502_find_first_existing(qe6502_abi_library ${qe6502_abi_candidates})
    qe6502_find_first_existing(qe6502_cpp_library ${qe6502_cpp_candidates})

    if(qe6502_abi_library STREQUAL "")
        message(FATAL_ERROR "Installed qe6502 shared library was not found. Checked: ${qe6502_abi_candidates}")
    endif()
    if(qe6502_cpp_library STREQUAL "")
        message(FATAL_ERROR "Installed qe6502_cpp shared library was not found. Checked: ${qe6502_cpp_candidates}")
    endif()

    message(STATUS "Checking installed C ABI shared exports: ${qe6502_abi_library}")
    qe6502_read_exports("${qe6502_abi_library}" qe6502_abi_symbols)
    if(QE6502_SYMBOL_CHECK_AVAILABLE)
        foreach(required_symbol IN ITEMS
            qe6502abi_version
            qe6502abi_setup
            qe6502abi_tick
        )
            qe6502_require_export("${qe6502_abi_library}" "${qe6502_abi_symbols}" "${required_symbol}")
        endforeach()
        foreach(forbidden_symbol IN ITEMS
            qe6502_setup
            qe6502_restart
            qe6502_tick_exported
            qe6502_control_store
            qe6502_save
            qe6502_load
        )
            qe6502_forbid_export("${qe6502_abi_library}" "${qe6502_abi_symbols}" "${forbidden_symbol}")
        endforeach()
    endif()

    message(STATUS "Checking installed C++ shared exports: ${qe6502_cpp_library}")
    qe6502_read_exports("${qe6502_cpp_library}" qe6502_cpp_symbols)
    if(QE6502_SYMBOL_CHECK_AVAILABLE)
        foreach(forbidden_symbol IN ITEMS
            qe6502abi_version
            qe6502abi_setup
            qe6502abi_tick
            qe6502_setup
            qe6502_restart
            qe6502_tick_exported
            qe6502_control_store
            qe6502_save
            qe6502_load
        )
            qe6502_forbid_export("${qe6502_cpp_library}" "${qe6502_cpp_symbols}" "${forbidden_symbol}")
        endforeach()
    endif()
endfunction()

if(NOT DEFINED QE6502_TEST_CONFIG OR "${QE6502_TEST_CONFIG}" STREQUAL "")
    set(QE6502_TEST_CONFIG Debug)
endif()

if(QE6502_INSTALL_CONSUME_CASE STREQUAL "dynamic")
    set(QE6502_CASE_BUILD_STATIC OFF)
    set(QE6502_CASE_BUILD_SHARED ON)
elseif(QE6502_INSTALL_CONSUME_CASE STREQUAL "static")
    set(QE6502_CASE_BUILD_STATIC ON)
    set(QE6502_CASE_BUILD_SHARED OFF)
elseif(QE6502_INSTALL_CONSUME_CASE STREQUAL "both")
    set(QE6502_CASE_BUILD_STATIC ON)
    set(QE6502_CASE_BUILD_SHARED ON)
else()
    message(FATAL_ERROR "Unknown QE6502_INSTALL_CONSUME_CASE: ${QE6502_INSTALL_CONSUME_CASE}")
endif()

set(QE6502_PACKAGE_BUILD_DIR "${QE6502_WORK_DIR}/qe6502-build")
set(QE6502_INSTALL_PREFIX "${QE6502_WORK_DIR}/qe6502-install")
set(QE6502_CONSUMER_BUILD_DIR "${QE6502_WORK_DIR}/consumer-build")

file(REMOVE_RECURSE
    "${QE6502_PACKAGE_BUILD_DIR}"
    "${QE6502_INSTALL_PREFIX}"
    "${QE6502_CONSUMER_BUILD_DIR}"
)
file(MAKE_DIRECTORY "${QE6502_WORK_DIR}")

set(QE6502_GENERATOR_ARGS -G "${QE6502_GENERATOR}")
if(DEFINED QE6502_GENERATOR_PLATFORM AND NOT "${QE6502_GENERATOR_PLATFORM}" STREQUAL "")
    list(APPEND QE6502_GENERATOR_ARGS -A "${QE6502_GENERATOR_PLATFORM}")
endif()
if(DEFINED QE6502_GENERATOR_TOOLSET AND NOT "${QE6502_GENERATOR_TOOLSET}" STREQUAL "")
    list(APPEND QE6502_GENERATOR_ARGS -T "${QE6502_GENERATOR_TOOLSET}")
endif()

message(STATUS "Configuring qe6502 ${QE6502_INSTALL_CONSUME_CASE} package build")
qe6502_run_step("configure qe6502 ${QE6502_INSTALL_CONSUME_CASE} package"
    "${CMAKE_COMMAND}"
    -S "${QE6502_SOURCE_DIR}"
    -B "${QE6502_PACKAGE_BUILD_DIR}"
    ${QE6502_GENERATOR_ARGS}
    -DCMAKE_BUILD_TYPE=${QE6502_TEST_CONFIG}
    -DCMAKE_INSTALL_PREFIX=${QE6502_INSTALL_PREFIX}
    -DBUILD_TESTING=OFF
    -DQE6502_BUILD_STATIC=${QE6502_CASE_BUILD_STATIC}
    -DQE6502_BUILD_SHARED=${QE6502_CASE_BUILD_SHARED}
    -DQE6502_BUILD_CPP=ON
    -DQE6502_BUILD_TOOLS=OFF
    -DQE6502_BUILD_TESTS=OFF
    -DQE6502_BUILD_CSHARP=OFF
    -DQE6502_BUILD_RUST=OFF
    -DQE6502_BUILD_JAVA=OFF
    -DQE6502_BUILD_PYTHON=OFF
    -DQE6502_INSTALL=ON
)

message(STATUS "Building qe6502 ${QE6502_INSTALL_CONSUME_CASE} package build")
qe6502_run_step("build qe6502 ${QE6502_INSTALL_CONSUME_CASE} package"
    "${CMAKE_COMMAND}" --build "${QE6502_PACKAGE_BUILD_DIR}" --config "${QE6502_TEST_CONFIG}"
)

message(STATUS "Installing qe6502 ${QE6502_INSTALL_CONSUME_CASE} package build")
qe6502_run_step("install qe6502 ${QE6502_INSTALL_CONSUME_CASE} package"
    "${CMAKE_COMMAND}" --install "${QE6502_PACKAGE_BUILD_DIR}" --config "${QE6502_TEST_CONFIG}"
)

qe6502_check_installed_shared_symbols()

message(STATUS "Configuring installed-package C++ consumer")
qe6502_run_step("configure installed-package C++ consumer"
    "${CMAKE_COMMAND}"
    -S "${QE6502_CONSUMER_SOURCE_DIR}"
    -B "${QE6502_CONSUMER_BUILD_DIR}"
    ${QE6502_GENERATOR_ARGS}
    -DCMAKE_BUILD_TYPE=${QE6502_TEST_CONFIG}
    -DCMAKE_PREFIX_PATH=${QE6502_INSTALL_PREFIX}
    -DQE6502_INSTALL_PREFIX=${QE6502_INSTALL_PREFIX}
    -DQE6502_INSTALL_CONSUME_CASE=${QE6502_INSTALL_CONSUME_CASE}
)

message(STATUS "Building installed-package C++ consumer")
qe6502_run_step("build installed-package C++ consumer"
    "${CMAKE_COMMAND}" --build "${QE6502_CONSUMER_BUILD_DIR}" --config "${QE6502_TEST_CONFIG}"
)

set(QE6502_CONSUMER_EXES qe6502_install_consume_cpp)
if(QE6502_INSTALL_CONSUME_CASE STREQUAL "both")
    list(APPEND QE6502_CONSUMER_EXES qe6502_install_consume_cpp_shared)
endif()

set(QE6502_RUNTIME_PATH "${QE6502_INSTALL_PREFIX}/bin")
if(WIN32)
    set(QE6502_PATH_SEP ";")
else()
    set(QE6502_PATH_SEP ":")
endif()

foreach(QE6502_CONSUMER_TARGET IN LISTS QE6502_CONSUMER_EXES)
    set(QE6502_CONSUMER_EXE "")
    set(QE6502_CONSUMER_EXE_CANDIDATES
        "${QE6502_CONSUMER_BUILD_DIR}/${QE6502_CONSUMER_TARGET}${CMAKE_EXECUTABLE_SUFFIX}"
        "${QE6502_CONSUMER_BUILD_DIR}/${QE6502_CONSUMER_TARGET}.exe"
        "${QE6502_CONSUMER_BUILD_DIR}/${QE6502_TEST_CONFIG}/${QE6502_CONSUMER_TARGET}${CMAKE_EXECUTABLE_SUFFIX}"
        "${QE6502_CONSUMER_BUILD_DIR}/${QE6502_TEST_CONFIG}/${QE6502_CONSUMER_TARGET}.exe"
    )

    foreach(QE6502_CONSUMER_EXE_CANDIDATE IN LISTS QE6502_CONSUMER_EXE_CANDIDATES)
        if(EXISTS "${QE6502_CONSUMER_EXE_CANDIDATE}")
            set(QE6502_CONSUMER_EXE "${QE6502_CONSUMER_EXE_CANDIDATE}")
            break()
        endif()
    endforeach()

    if(QE6502_CONSUMER_EXE STREQUAL "")
        message(FATAL_ERROR
            "Consumer executable was not produced for ${QE6502_CONSUMER_TARGET}. "
            "Checked: ${QE6502_CONSUMER_EXE_CANDIDATES}"
        )
    endif()

    if(WIN32 AND IS_DIRECTORY "${QE6502_RUNTIME_PATH}")
        get_filename_component(QE6502_CONSUMER_EXE_DIR "${QE6502_CONSUMER_EXE}" DIRECTORY)
        file(GLOB QE6502_RUNTIME_DLLS "${QE6502_RUNTIME_PATH}/*.dll")
        if(QE6502_RUNTIME_DLLS)
            file(COPY ${QE6502_RUNTIME_DLLS} DESTINATION "${QE6502_CONSUMER_EXE_DIR}")
        endif()
    endif()

    message(STATUS "Running installed-package C++ consumer: ${QE6502_CONSUMER_EXE}")
    if(WIN32)
        qe6502_run_step("run installed-package C++ consumer ${QE6502_CONSUMER_TARGET}"
            "${QE6502_CONSUMER_EXE}"
        )
    else()
        qe6502_run_step("run installed-package C++ consumer ${QE6502_CONSUMER_TARGET}"
            "${CMAKE_COMMAND}" -E env
                "LD_LIBRARY_PATH=${QE6502_INSTALL_PREFIX}/lib${QE6502_PATH_SEP}$ENV{LD_LIBRARY_PATH}"
                "DYLD_LIBRARY_PATH=${QE6502_INSTALL_PREFIX}/lib${QE6502_PATH_SEP}$ENV{DYLD_LIBRARY_PATH}"
                "${QE6502_CONSUMER_EXE}"
        )
    endif()
endforeach()
