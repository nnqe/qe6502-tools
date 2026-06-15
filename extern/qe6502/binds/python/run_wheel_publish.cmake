if(NOT DEFINED QE6502_PYTHON_EXECUTABLE OR QE6502_PYTHON_EXECUTABLE STREQUAL "")
    message(FATAL_ERROR "QE6502_PYTHON_EXECUTABLE is required")
endif()

if(NOT DEFINED QE6502_PYTHON_DIST_DIR OR QE6502_PYTHON_DIST_DIR STREQUAL "")
    message(FATAL_ERROR "QE6502_PYTHON_DIST_DIR is required")
endif()

if(NOT DEFINED QE6502_PYTHON_PUBLISH_DIST_DIR OR QE6502_PYTHON_PUBLISH_DIST_DIR STREQUAL "")
    message(FATAL_ERROR "QE6502_PYTHON_PUBLISH_DIST_DIR is required")
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

file(REMOVE_RECURSE "${QE6502_PYTHON_PUBLISH_DIST_DIR}")
file(MAKE_DIRECTORY "${QE6502_PYTHON_PUBLISH_DIST_DIR}")

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Repairing Linux wheel with auditwheel: ${wheel_file}")
    execute_process(
        COMMAND "${QE6502_PYTHON_EXECUTABLE}" -m auditwheel repair
            --wheel-dir "${QE6502_PYTHON_PUBLISH_DIST_DIR}"
            "${wheel_file}"
        RESULT_VARIABLE auditwheel_result
        OUTPUT_VARIABLE auditwheel_stdout
        ERROR_VARIABLE auditwheel_stderr
    )
    if(NOT auditwheel_result EQUAL 0)
        message(FATAL_ERROR "auditwheel repair failed (${auditwheel_result})\nwheel: ${wheel_file}\nstdout:\n${auditwheel_stdout}\nstderr:\n${auditwheel_stderr}")
    endif()
    message(STATUS "auditwheel repair passed")
    message(STATUS "${auditwheel_stdout}")
else()
    get_filename_component(wheel_name "${wheel_file}" NAME)
    message(STATUS "Using platform wheel without auditwheel repair: ${wheel_name}")
    file(COPY "${wheel_file}" DESTINATION "${QE6502_PYTHON_PUBLISH_DIST_DIR}")
endif()

file(GLOB publish_wheels "${QE6502_PYTHON_PUBLISH_DIST_DIR}/*.whl")
list(LENGTH publish_wheels publish_count)
if(NOT publish_count EQUAL 1)
    message(FATAL_ERROR "Expected exactly one publish wheel in ${QE6502_PYTHON_PUBLISH_DIST_DIR}, found ${publish_count}: ${publish_wheels}")
endif()
list(GET publish_wheels 0 publish_wheel)
message(STATUS "Python publish wheel candidate: ${publish_wheel}")
