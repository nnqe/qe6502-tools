if(NOT QE6502_BUILD_PYTHON)
    message(STATUS "QE6502_BUILD_PYTHON is OFF; skipping qe6502 Python smoke harness")
    return()
endif()

find_package(Python3 QUIET COMPONENTS Interpreter)

if(NOT Python3_Interpreter_FOUND)
    message(STATUS "Python3 interpreter not found; skipping qe6502 Python smoke harness")
    return()
endif()

if(NOT TARGET qe6502_python)
    message(STATUS "qe6502_python target is not available; skipping qe6502 Python smoke harness")
    return()
endif()

set(QE6502_PY_SMOKE_SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/py_smoke.py)

add_custom_command(
    OUTPUT ${QE6502_PY_SMOKE_SCRIPT}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/py_smoke.py
        ${QE6502_PY_SMOKE_SCRIPT}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/py_smoke.py
    VERBATIM
)

add_custom_target(qe6502_py_smoke_runtime ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/qe6502
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/binds/python/qe6502/__init__.py
        ${CMAKE_CURRENT_BINARY_DIR}/qe6502/__init__.py
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:_qe6502>
        ${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_NAME:_qe6502>
    DEPENDS
        qe6502_python
        ${QE6502_PY_SMOKE_SCRIPT}
        ${CMAKE_SOURCE_DIR}/binds/python/qe6502/__init__.py
)

add_custom_target(qe6502_py_smoke_run
    COMMAND ${Python3_EXECUTABLE} ${QE6502_PY_SMOKE_SCRIPT}
    DEPENDS qe6502_py_smoke_runtime
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running qe6502 Python smoke harness"
    VERBATIM
)

add_test(NAME qe6502_py_smoke
    COMMAND ${Python3_EXECUTABLE} ${QE6502_PY_SMOKE_SCRIPT}
)

set_tests_properties(qe6502_py_smoke PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    LABELS "python;smoke;functional"
    TIMEOUT 30
)
