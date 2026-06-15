if(NOT DEFINED QE6502_NPM_EXECUTABLE)
    message(FATAL_ERROR "QE6502_NPM_EXECUTABLE is required")
endif()

if(NOT DEFINED QE6502_PACKAGE_DIR)
    message(FATAL_ERROR "QE6502_PACKAGE_DIR is required")
endif()

message(STATUS "Running npm publish --dry-run in: ${QE6502_PACKAGE_DIR}")

execute_process(
    COMMAND "${QE6502_NPM_EXECUTABLE}" publish --dry-run
    WORKING_DIRECTORY "${QE6502_PACKAGE_DIR}"
    RESULT_VARIABLE publish_result
    OUTPUT_VARIABLE publish_output
    ERROR_VARIABLE publish_error
)

if(NOT publish_result EQUAL 0)
    message(FATAL_ERROR "npm publish --dry-run failed\nCommand: ${QE6502_NPM_EXECUTABLE} publish --dry-run\nWorking directory: ${QE6502_PACKAGE_DIR}\nResult: ${publish_result}\nstdout:\n${publish_output}\nstderr:\n${publish_error}")
endif()

message(STATUS "${publish_output}")
if(NOT publish_error STREQUAL "")
    message(STATUS "${publish_error}")
endif()
