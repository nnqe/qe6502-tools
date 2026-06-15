if(NOT DEFINED QE6502_NPM_EXECUTABLE)
    message(FATAL_ERROR "QE6502_NPM_EXECUTABLE is required")
endif()

if(NOT DEFINED QE6502_NODE_EXECUTABLE)
    message(FATAL_ERROR "QE6502_NODE_EXECUTABLE is required")
endif()

if(NOT DEFINED QE6502_PACKAGE_DIR)
    message(FATAL_ERROR "QE6502_PACKAGE_DIR is required")
endif()

if(NOT DEFINED QE6502_PACK_DIR)
    message(FATAL_ERROR "QE6502_PACK_DIR is required")
endif()

if(NOT DEFINED QE6502_TARBALL)
    message(FATAL_ERROR "QE6502_TARBALL is required")
endif()

if(NOT DEFINED QE6502_CONSUMER_DIR)
    message(FATAL_ERROR "QE6502_CONSUMER_DIR is required")
endif()

if(NOT DEFINED QE6502_CONSUMER_SCRIPT)
    message(FATAL_ERROR "QE6502_CONSUMER_SCRIPT is required")
endif()

file(REMOVE_RECURSE "${QE6502_PACK_DIR}" "${QE6502_CONSUMER_DIR}")
file(MAKE_DIRECTORY "${QE6502_PACK_DIR}" "${QE6502_CONSUMER_DIR}")

execute_process(
    COMMAND "${QE6502_NPM_EXECUTABLE}" pack --pack-destination "${QE6502_PACK_DIR}"
    WORKING_DIRECTORY "${QE6502_PACKAGE_DIR}"
    RESULT_VARIABLE pack_result
    OUTPUT_VARIABLE pack_output
    ERROR_VARIABLE pack_error
)

if(NOT pack_result EQUAL 0)
    message(FATAL_ERROR "npm pack failed\nCommand: ${QE6502_NPM_EXECUTABLE} pack --pack-destination ${QE6502_PACK_DIR}\nWorking directory: ${QE6502_PACKAGE_DIR}\nResult: ${pack_result}\nstdout:\n${pack_output}\nstderr:\n${pack_error}")
endif()

if(NOT EXISTS "${QE6502_TARBALL}")
    message(FATAL_ERROR "Expected npm tarball was not created: ${QE6502_TARBALL}\nnpm output:\n${pack_output}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar tf "${QE6502_TARBALL}"
    RESULT_VARIABLE list_result
    OUTPUT_VARIABLE list_output
    ERROR_VARIABLE list_error
)

if(NOT list_result EQUAL 0)
    message(FATAL_ERROR "Failed to list npm tarball contents\n${list_output}\n${list_error}")
endif()

string(REPLACE "\r\n" "\n" list_output "${list_output}")
string(REPLACE "\r" "\n" list_output "${list_output}")
string(REGEX REPLACE "\n+$" "" list_output "${list_output}")
if(list_output STREQUAL "")
    set(package_files)
else()
    string(REPLACE "\n" ";" package_files "${list_output}")
endif()
list(SORT package_files)

set(expected_files
    "package/LICENSE"
    "package/README.md"
    "package/package.json"
    "package/qe6502.js"
    "package/qe6502_js.wasm"
)
list(SORT expected_files)

foreach(expected_file IN LISTS expected_files)
    list(FIND package_files "${expected_file}" found_index)
    if(found_index EQUAL -1)
        message(FATAL_ERROR "Missing expected npm package file: ${expected_file}\nPackage files:\n${list_output}")
    endif()
endforeach()

list(LENGTH package_files package_file_count)
list(LENGTH expected_files expected_file_count)
if(NOT package_file_count EQUAL expected_file_count)
    message(FATAL_ERROR "Unexpected npm package contents\nExpected: ${expected_files}\nActual: ${package_files}")
endif()

file(WRITE "${QE6502_CONSUMER_DIR}/package.json" "{\n  \"type\": \"module\",\n  \"private\": true\n}\n")
file(COPY "${QE6502_CONSUMER_SCRIPT}" DESTINATION "${QE6502_CONSUMER_DIR}")

execute_process(
    COMMAND "${QE6502_NPM_EXECUTABLE}" install --no-audit --no-fund "${QE6502_TARBALL}"
    WORKING_DIRECTORY "${QE6502_CONSUMER_DIR}"
    RESULT_VARIABLE install_result
    OUTPUT_VARIABLE install_output
    ERROR_VARIABLE install_error
)

if(NOT install_result EQUAL 0)
    message(FATAL_ERROR "npm install from tarball failed\nCommand: ${QE6502_NPM_EXECUTABLE} install --no-audit --no-fund ${QE6502_TARBALL}\nWorking directory: ${QE6502_CONSUMER_DIR}\nResult: ${install_result}\nstdout:\n${install_output}\nstderr:\n${install_error}")
endif()

execute_process(
    COMMAND "${QE6502_NODE_EXECUTABLE}" package_consumer_smoke.mjs
    WORKING_DIRECTORY "${QE6502_CONSUMER_DIR}"
    RESULT_VARIABLE smoke_result
    OUTPUT_VARIABLE smoke_output
    ERROR_VARIABLE smoke_error
)

if(NOT smoke_result EQUAL 0)
    message(FATAL_ERROR "qe6502 npm package consumer smoke failed\nCommand: ${QE6502_NODE_EXECUTABLE} package_consumer_smoke.mjs\nWorking directory: ${QE6502_CONSUMER_DIR}\nResult: ${smoke_result}\nstdout:\n${smoke_output}\nstderr:\n${smoke_error}")
endif()

message(STATUS "${smoke_output}")
