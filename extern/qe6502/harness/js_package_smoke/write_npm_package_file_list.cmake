if(NOT DEFINED QE6502_TARBALL)
    message(FATAL_ERROR "QE6502_TARBALL is required")
endif()

if(NOT DEFINED QE6502_FILE_LIST)
    message(FATAL_ERROR "QE6502_FILE_LIST is required")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar tf "${QE6502_TARBALL}"
    RESULT_VARIABLE list_result
    OUTPUT_VARIABLE list_output
    ERROR_VARIABLE list_error
)

if(NOT list_result EQUAL 0)
    message(FATAL_ERROR "Failed to list npm tarball contents\nTarball: ${QE6502_TARBALL}\nResult: ${list_result}\nstdout:\n${list_output}\nstderr:\n${list_error}")
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

string(REPLACE ";" "\n" package_file_text "${package_files}")
file(WRITE "${QE6502_FILE_LIST}" "${package_file_text}\n")
