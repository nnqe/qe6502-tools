if(NOT DEFINED QE6502_PYTHON_EXECUTABLE OR QE6502_PYTHON_EXECUTABLE STREQUAL "")
    message(FATAL_ERROR "QE6502_PYTHON_EXECUTABLE is required")
endif()

if(NOT DEFINED QE6502_PYTHON_DIST_DIR OR QE6502_PYTHON_DIST_DIR STREQUAL "")
    message(FATAL_ERROR "QE6502_PYTHON_DIST_DIR is required")
endif()

if(NOT DEFINED QE6502_PYTHON_WHEEL_GLOB OR QE6502_PYTHON_WHEEL_GLOB STREQUAL "")
    message(FATAL_ERROR "QE6502_PYTHON_WHEEL_GLOB is required")
endif()

file(GLOB wheel_candidates "${QE6502_PYTHON_WHEEL_GLOB}")
list(LENGTH wheel_candidates wheel_count)
if(NOT wheel_count EQUAL 1)
    message(FATAL_ERROR "Expected exactly one qe6502 wheel in ${QE6502_PYTHON_DIST_DIR}, found ${wheel_count}: ${wheel_candidates}")
endif()
list(GET wheel_candidates 0 wheel_file)

message(STATUS "Running twine check for ${wheel_file}")
execute_process(
    COMMAND "${QE6502_PYTHON_EXECUTABLE}" -m twine check "${wheel_file}"
    RESULT_VARIABLE twine_result
    OUTPUT_VARIABLE twine_stdout
    ERROR_VARIABLE twine_stderr
)
if(NOT twine_result EQUAL 0)
    message(FATAL_ERROR "twine check failed (${twine_result})\nstdout:\n${twine_stdout}\nstderr:\n${twine_stderr}")
endif()

message(STATUS "twine check passed")
message(STATUS "${twine_stdout}")
