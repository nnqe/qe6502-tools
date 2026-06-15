if(NOT QE6502_BUILD_PYTHON)
    message(STATUS "QE6502_BUILD_PYTHON is OFF; skipping qe6502 Python Klaus2m5 harness")
    return()
endif()

find_package(Python3 QUIET COMPONENTS Interpreter)

if(NOT Python3_Interpreter_FOUND)
    message(STATUS "Python3 interpreter not found; skipping qe6502 Python Klaus2m5 harness")
    return()
endif()

if(NOT TARGET qe6502_python)
    message(STATUS "qe6502_python target is not available; skipping qe6502 Python Klaus2m5 harness")
    return()
endif()

set(QE6502_PY_KLAUS_SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/py_klaus2m5.py)
set(QE6502_PY_KLAUS_ROM_DIR ${CMAKE_CURRENT_BINARY_DIR}/roms)
set(QE6502_PY_KLAUS_STANDARD_ROM ${QE6502_PY_KLAUS_ROM_DIR}/6502_functional_test.hex)
set(QE6502_PY_KLAUS_EXTENDED_ROM ${QE6502_PY_KLAUS_ROM_DIR}/65C02_extended_opcodes_test.hex)

add_custom_command(
    OUTPUT ${QE6502_PY_KLAUS_SCRIPT}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/py_klaus2m5.py
        ${QE6502_PY_KLAUS_SCRIPT}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/py_klaus2m5.py
    VERBATIM
)

add_custom_command(
    OUTPUT
        ${QE6502_PY_KLAUS_STANDARD_ROM}
        ${QE6502_PY_KLAUS_EXTENDED_ROM}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QE6502_PY_KLAUS_ROM_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/harness/klaus2m5/6502_functional_test.hex
        ${QE6502_PY_KLAUS_STANDARD_ROM}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/harness/klaus2m5/65C02_extended_opcodes_test.hex
        ${QE6502_PY_KLAUS_EXTENDED_ROM}
    DEPENDS
        ${CMAKE_SOURCE_DIR}/harness/klaus2m5/6502_functional_test.hex
        ${CMAKE_SOURCE_DIR}/harness/klaus2m5/65C02_extended_opcodes_test.hex
    VERBATIM
)

add_custom_target(qe6502_py_klaus2m5_runtime ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/qe6502
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/binds/python/qe6502/__init__.py
        ${CMAKE_CURRENT_BINARY_DIR}/qe6502/__init__.py
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:_qe6502>
        ${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_NAME:_qe6502>
    DEPENDS
        qe6502_python
        ${QE6502_PY_KLAUS_SCRIPT}
        ${QE6502_PY_KLAUS_STANDARD_ROM}
        ${QE6502_PY_KLAUS_EXTENDED_ROM}
        ${CMAKE_SOURCE_DIR}/binds/python/qe6502/__init__.py
)

add_custom_target(qe6502_py_klaus2m5_run
    COMMAND ${Python3_EXECUTABLE} ${QE6502_PY_KLAUS_SCRIPT}
    DEPENDS qe6502_py_klaus2m5_runtime
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running qe6502 Python Klaus2m5 harness"
    VERBATIM
)

set(QE6502_PY_KLAUS2M5_TESTS
    nmos standard
    rw extended
)

list(LENGTH QE6502_PY_KLAUS2M5_TESTS QE6502_PY_KLAUS2M5_TEST_COUNT)
math(EXPR QE6502_PY_KLAUS2M5_LAST_INDEX "${QE6502_PY_KLAUS2M5_TEST_COUNT} - 1")

foreach(index RANGE 0 ${QE6502_PY_KLAUS2M5_LAST_INDEX} 2)
    list(GET QE6502_PY_KLAUS2M5_TESTS ${index} model)
    math(EXPR test_index "${index} + 1")
    list(GET QE6502_PY_KLAUS2M5_TESTS ${test_index} test_name)

    add_test(
        NAME qe6502.py.klaus2m5.${model}.${test_name}
        COMMAND ${Python3_EXECUTABLE} ${QE6502_PY_KLAUS_SCRIPT} ${QE6502_PY_KLAUS_ROM_DIR} ${model} ${test_name}
    )

    set_tests_properties(qe6502.py.klaus2m5.${model}.${test_name} PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        LABELS "python;v2;klaus;functional;performance"
        TIMEOUT 180
    )
endforeach()
