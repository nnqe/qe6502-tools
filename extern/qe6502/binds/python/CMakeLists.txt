set(QE6502_PYTHON_LIMITED_API_HEX 0x030A0000)
set(QE6502_PYTHON_STABLE_ABI_MINIMUM "3.10")

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.26)
    find_package(Python3 QUIET COMPONENTS Development.SABIModule)

    if(Python3_Development.SABIModule_FOUND)
        set(QE6502_PYTHON_MODULE_TARGET Python3::SABIModule)
        set(QE6502_PYTHON_FOUND TRUE)
        set(QE6502_PYTHON_USING_STABLE_ABI_TARGET TRUE)
    endif()
endif()

if(NOT QE6502_PYTHON_FOUND)
    find_package(Python3 QUIET COMPONENTS Development.Module)

    if(Python3_Development.Module_FOUND)
        set(QE6502_PYTHON_MODULE_TARGET Python3::Module)
        set(QE6502_PYTHON_FOUND TRUE)
        set(QE6502_PYTHON_USING_STABLE_ABI_TARGET FALSE)
    endif()
endif()

if(NOT QE6502_PYTHON_FOUND)
    message(STATUS "Python3 development module not found; skipping qe6502 Python binding")
    return()
endif()

if(NOT TARGET qe6502_static)
    message(STATUS "qe6502_static target is not available; skipping qe6502 Python binding")
    return()
endif()

add_library(_qe6502 MODULE
    _qe6502.c
)

target_compile_features(_qe6502 PRIVATE c_std_11)
qe6502_configure_warnings(_qe6502)

target_compile_definitions(_qe6502
    PRIVATE
        Py_LIMITED_API=${QE6502_PYTHON_LIMITED_API_HEX}
)

target_link_libraries(_qe6502
    PRIVATE
        ${QE6502_PYTHON_MODULE_TARGET}
        qe6502_static
)

set_target_properties(_qe6502 PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

if(WIN32)
    set_target_properties(_qe6502 PROPERTIES SUFFIX ".pyd")
elseif(QE6502_PYTHON_USING_STABLE_ABI_TARGET AND DEFINED Python3_SOSABI AND NOT Python3_SOSABI STREQUAL "")
    if(Python3_SOSABI MATCHES "^\\.")
        set(QE6502_PYTHON_EXTENSION_SUFFIX "${Python3_SOSABI}")
    else()
        set(QE6502_PYTHON_EXTENSION_SUFFIX ".${Python3_SOSABI}${CMAKE_SHARED_MODULE_SUFFIX}")
    endif()
    set_target_properties(_qe6502 PROPERTIES SUFFIX "${QE6502_PYTHON_EXTENSION_SUFFIX}")
elseif(QE6502_PYTHON_USING_STABLE_ABI_TARGET)
    set_target_properties(_qe6502 PROPERTIES SUFFIX ".abi3${CMAKE_SHARED_MODULE_SUFFIX}")
elseif(DEFINED Python3_EXTENSION_SUFFIX AND NOT Python3_EXTENSION_SUFFIX STREQUAL "")
    set_target_properties(_qe6502 PROPERTIES SUFFIX "${Python3_EXTENSION_SUFFIX}")
endif()

message(STATUS "qe6502 Python binding uses CPython Limited API ${QE6502_PYTHON_STABLE_ABI_MINIMUM}+; stable ABI target: ${QE6502_PYTHON_USING_STABLE_ABI_TARGET}")

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/qe6502/__init__.py
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/qe6502
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/qe6502/__init__.py
        ${CMAKE_CURRENT_BINARY_DIR}/qe6502/__init__.py
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/qe6502/__init__.py
    VERBATIM
)

add_custom_target(qe6502_python ALL
    DEPENDS
        _qe6502
        ${CMAKE_CURRENT_BINARY_DIR}/qe6502/__init__.py
)


set(QE6502_PYTHON_PACKAGE_NAME "qe6502")
set(QE6502_PYTHON_PACKAGE_DESCRIPTION "A lightweight bus-cycle 6502 CPU emulator for Python.")
set(QE6502_PYTHON_PACKAGE_AUTHOR "Nikolay Nedelchev")
set(QE6502_PYTHON_PACKAGE_LICENSE "MIT")
set(QE6502_PYTHON_PACKAGE_REQUIRES_PYTHON ">=${QE6502_PYTHON_STABLE_ABI_MINIMUM}")
set(QE6502_PYTHON_WHEEL_PYTHON_TAG "cp310")
set(QE6502_PYTHON_WHEEL_ABI_TAG "abi3")

set(QE6502_PYTHON_PACKAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/package/qe6502")
set(QE6502_PYTHON_PACKAGE_DIST_DIR "${CMAKE_CURRENT_BINARY_DIR}/dist")
set(QE6502_PYTHON_PACKAGE_PUBLISH_DIST_DIR "${CMAKE_CURRENT_BINARY_DIR}/publish-dist")
set(QE6502_PYTHON_PACKAGE_SDIST_DIR "${CMAKE_CURRENT_BINARY_DIR}/sdist")
set(QE6502_PYTHON_PACKAGE_PYPROJECT "${QE6502_PYTHON_PACKAGE_DIR}/pyproject.toml")
set(QE6502_PYTHON_PACKAGE_README "${QE6502_PYTHON_PACKAGE_DIR}/README.md")
set(QE6502_PYTHON_PACKAGE_LICENSE_FILE "${QE6502_PYTHON_PACKAGE_DIR}/LICENSE")
set(QE6502_PYTHON_PACKAGE_INIT "${QE6502_PYTHON_PACKAGE_DIR}/qe6502/__init__.py")
set(QE6502_PYTHON_PACKAGE_WHEEL_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/package_wheel.py")
set(QE6502_PYTHON_PACKAGE_SDIST_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/package_sdist.py")
set(QE6502_PYTHON_PACKAGE_SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
set(QE6502_PYTHON_PACKAGE_SETUP_CFG_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.cfg.in")
set(QE6502_PYTHON_PACKAGE_SETUP_PY "${QE6502_PYTHON_PACKAGE_DIR}/setup.py")
set(QE6502_PYTHON_PACKAGE_SETUP_CFG "${QE6502_PYTHON_PACKAGE_DIR}/setup.cfg")
set(QE6502_PYTHON_WHEEL_GLOB "${QE6502_PYTHON_PACKAGE_DIST_DIR}/${QE6502_PYTHON_PACKAGE_NAME}-${QE6502_PACKAGE_VERSION}-${QE6502_PYTHON_WHEEL_PYTHON_TAG}-${QE6502_PYTHON_WHEEL_ABI_TAG}-*.whl")
set(QE6502_PYTHON_PUBLISH_WHEEL_GLOB "${QE6502_PYTHON_PACKAGE_PUBLISH_DIST_DIR}/${QE6502_PYTHON_PACKAGE_NAME}-${QE6502_PACKAGE_VERSION}-${QE6502_PYTHON_WHEEL_PYTHON_TAG}-${QE6502_PYTHON_WHEEL_ABI_TAG}-*.whl")
set(QE6502_PYTHON_SDIST_GLOB "${QE6502_PYTHON_PACKAGE_SDIST_DIR}/${QE6502_PYTHON_PACKAGE_NAME}-${QE6502_PACKAGE_VERSION}.tar.gz")
set(QE6502_PYTHON_WHEEL_SMOKE_DIR "${CMAKE_CURRENT_BINARY_DIR}/wheel-smoke")

file(MAKE_DIRECTORY "${QE6502_PYTHON_PACKAGE_DIR}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/pyproject.toml.in"
    "${QE6502_PYTHON_PACKAGE_PYPROJECT}"
    @ONLY
)
configure_file(
    "${QE6502_PYTHON_PACKAGE_SETUP_PY_IN}"
    "${QE6502_PYTHON_PACKAGE_SETUP_PY}"
    @ONLY
)
configure_file(
    "${QE6502_PYTHON_PACKAGE_SETUP_CFG_IN}"
    "${QE6502_PYTHON_PACKAGE_SETUP_CFG}"
    @ONLY
)

find_program(QE6502_PYTHON_PACKAGE_EXECUTABLE NAMES python3 python)

if(NOT QE6502_PYTHON_PACKAGE_EXECUTABLE)
    message(STATUS "python3/python executable not found; skipping qe6502 Python wheel target")
else()
    add_custom_target(qe6502_python_package_stage
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${QE6502_PYTHON_PACKAGE_DIR}"
            "${QE6502_PYTHON_PACKAGE_DIR}/qe6502"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
            "${QE6502_PYTHON_PACKAGE_README}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_SOURCE_DIR}/LICENSE"
            "${QE6502_PYTHON_PACKAGE_LICENSE_FILE}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/qe6502/__init__.py"
            "${QE6502_PYTHON_PACKAGE_INIT}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:_qe6502>"
            "${QE6502_PYTHON_PACKAGE_DIR}/$<TARGET_FILE_NAME:_qe6502>"
        DEPENDS
            qe6502_python
            "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
            "${CMAKE_CURRENT_SOURCE_DIR}/qe6502/__init__.py"
            "${CMAKE_SOURCE_DIR}/LICENSE"
            "${QE6502_PYTHON_PACKAGE_PYPROJECT}"
        COMMENT "Staging qe6502 Python package files"
        VERBATIM
    )

    add_custom_target(qe6502_python_wheel
        COMMAND "${CMAKE_COMMAND}" -E remove_directory "${QE6502_PYTHON_PACKAGE_DIST_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${QE6502_PYTHON_PACKAGE_DIST_DIR}"
        COMMAND "${QE6502_PYTHON_PACKAGE_EXECUTABLE}"
            "${QE6502_PYTHON_PACKAGE_WHEEL_SCRIPT}"
            --stage-dir "${QE6502_PYTHON_PACKAGE_DIR}"
            --dist-dir "${QE6502_PYTHON_PACKAGE_DIST_DIR}"
            --name "${QE6502_PYTHON_PACKAGE_NAME}"
            --version "${QE6502_PACKAGE_VERSION}"
            --description "${QE6502_PYTHON_PACKAGE_DESCRIPTION}"
            --author "${QE6502_PYTHON_PACKAGE_AUTHOR}"
            --license "${QE6502_PYTHON_PACKAGE_LICENSE}"
            --license-file "${QE6502_PYTHON_PACKAGE_LICENSE_FILE}"
            --readme "${QE6502_PYTHON_PACKAGE_README}"
            --requires-python "${QE6502_PYTHON_PACKAGE_REQUIRES_PYTHON}"
            --python-tag "${QE6502_PYTHON_WHEEL_PYTHON_TAG}"
            --abi-tag "${QE6502_PYTHON_WHEEL_ABI_TAG}"
            --keyword 6502
            --keyword 65c02
            --keyword emulator
            --keyword cpu
            --keyword cycle-accurate
            --keyword bus-cycle
            --keyword python
            --keyword cpython
            --keyword retrocomputing
            --classifier "Development Status :: 5 - Production/Stable"
            --classifier "License :: OSI Approved :: MIT License"
            --classifier "Programming Language :: C"
            --classifier "Programming Language :: Python :: 3"
            --classifier "Programming Language :: Python :: 3 :: Only"
            --classifier "Programming Language :: Python :: 3.10"
            --classifier "Programming Language :: Python :: 3.11"
            --classifier "Programming Language :: Python :: 3.12"
            --classifier "Programming Language :: Python :: 3.13"
            --classifier "Programming Language :: Python :: Implementation :: CPython"
            --classifier "Topic :: System :: Emulators"
            --project-url Homepage "https://github.com/nnqe/qe6502"
            --project-url Repository "https://github.com/nnqe/qe6502"
            --project-url Issues "https://github.com/nnqe/qe6502/issues"
        DEPENDS
            qe6502_python_package_stage
            "${QE6502_PYTHON_PACKAGE_WHEEL_SCRIPT}"
        COMMENT "Building qe6502 Python wheel"
        VERBATIM
    )

    add_custom_target(qe6502_python_sdist
        COMMAND "${CMAKE_COMMAND}" -E remove_directory "${QE6502_PYTHON_PACKAGE_SDIST_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${QE6502_PYTHON_PACKAGE_SDIST_DIR}"
        COMMAND "${QE6502_PYTHON_PACKAGE_EXECUTABLE}"
            "${QE6502_PYTHON_PACKAGE_SDIST_SCRIPT}"
            --repo-root "${CMAKE_SOURCE_DIR}"
            --stage-dir "${QE6502_PYTHON_PACKAGE_DIR}"
            --dist-dir "${QE6502_PYTHON_PACKAGE_SDIST_DIR}"
            --setup-py "${QE6502_PYTHON_PACKAGE_SETUP_PY}"
            --setup-cfg "${QE6502_PYTHON_PACKAGE_SETUP_CFG}"
            --name "${QE6502_PYTHON_PACKAGE_NAME}"
            --version "${QE6502_PACKAGE_VERSION}"
            --description "${QE6502_PYTHON_PACKAGE_DESCRIPTION}"
            --author "${QE6502_PYTHON_PACKAGE_AUTHOR}"
            --license "${QE6502_PYTHON_PACKAGE_LICENSE}"
            --license-file "${QE6502_PYTHON_PACKAGE_LICENSE_FILE}"
            --readme "${QE6502_PYTHON_PACKAGE_README}"
            --requires-python "${QE6502_PYTHON_PACKAGE_REQUIRES_PYTHON}"
            --keyword 6502
            --keyword 65c02
            --keyword emulator
            --keyword cpu
            --keyword cycle-accurate
            --keyword bus-cycle
            --keyword python
            --keyword cpython
            --keyword retrocomputing
            --classifier "Development Status :: 5 - Production/Stable"
            --classifier "License :: OSI Approved :: MIT License"
            --classifier "Programming Language :: C"
            --classifier "Programming Language :: Python :: 3"
            --classifier "Programming Language :: Python :: 3 :: Only"
            --classifier "Programming Language :: Python :: 3.10"
            --classifier "Programming Language :: Python :: 3.11"
            --classifier "Programming Language :: Python :: 3.12"
            --classifier "Programming Language :: Python :: 3.13"
            --classifier "Programming Language :: Python :: Implementation :: CPython"
            --classifier "Topic :: System :: Emulators"
            --project-url Homepage "https://github.com/nnqe/qe6502"
            --project-url Repository "https://github.com/nnqe/qe6502"
            --project-url Issues "https://github.com/nnqe/qe6502/issues"
        DEPENDS
            qe6502_python_package_stage
            "${QE6502_PYTHON_PACKAGE_SDIST_SCRIPT}"
            "${QE6502_PYTHON_PACKAGE_SETUP_PY}"
            "${QE6502_PYTHON_PACKAGE_SETUP_CFG}"
        COMMENT "Building qe6502 Python source distribution"
        VERBATIM
    )

    add_custom_target(qe6502_python_sdist_twine_check
        COMMAND "${QE6502_PYTHON_PACKAGE_EXECUTABLE}" -m twine check "${QE6502_PYTHON_SDIST_GLOB}"
        DEPENDS qe6502_python_sdist
        COMMENT "Running twine check for qe6502 Python source distribution"
        VERBATIM
    )

    add_custom_target(qe6502_python_publish_wheel
        COMMAND "${CMAKE_COMMAND}"
            -DQE6502_PYTHON_EXECUTABLE=${QE6502_PYTHON_PACKAGE_EXECUTABLE}
            -DQE6502_PYTHON_DIST_DIR=${QE6502_PYTHON_PACKAGE_DIST_DIR}
            -DQE6502_PYTHON_PUBLISH_DIST_DIR=${QE6502_PYTHON_PACKAGE_PUBLISH_DIST_DIR}
            -DQE6502_PYTHON_WHEEL_GLOB=${QE6502_PYTHON_WHEEL_GLOB}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/run_wheel_publish.cmake"
        DEPENDS
            qe6502_python_wheel
            "${CMAKE_CURRENT_SOURCE_DIR}/run_wheel_publish.cmake"
        COMMENT "Preparing qe6502 Python publish wheel"
        VERBATIM
    )

    add_custom_target(qe6502_python_wheel_smoke
        COMMAND "${CMAKE_COMMAND}"
            -DQE6502_PYTHON_EXECUTABLE=${QE6502_PYTHON_PACKAGE_EXECUTABLE}
            -DQE6502_PYTHON_DIST_DIR=${QE6502_PYTHON_PACKAGE_PUBLISH_DIST_DIR}
            -DQE6502_PYTHON_WHEEL_GLOB=${QE6502_PYTHON_PUBLISH_WHEEL_GLOB}
            -DQE6502_PYTHON_SMOKE_DIR=${QE6502_PYTHON_WHEEL_SMOKE_DIR}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/run_wheel_smoke.cmake"
        DEPENDS
            qe6502_python_publish_wheel
            "${CMAKE_CURRENT_SOURCE_DIR}/run_wheel_smoke.cmake"
        COMMENT "Installing and smoke-testing qe6502 Python publish wheel"
        VERBATIM
    )

    add_custom_target(qe6502_python_twine_check
        COMMAND "${CMAKE_COMMAND}"
            -DQE6502_PYTHON_EXECUTABLE=${QE6502_PYTHON_PACKAGE_EXECUTABLE}
            -DQE6502_PYTHON_DIST_DIR=${QE6502_PYTHON_PACKAGE_PUBLISH_DIST_DIR}
            -DQE6502_PYTHON_WHEEL_GLOB=${QE6502_PYTHON_PUBLISH_WHEEL_GLOB}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/run_twine_check.cmake"
        DEPENDS
            qe6502_python_publish_wheel
            "${CMAKE_CURRENT_SOURCE_DIR}/run_twine_check.cmake"
        COMMENT "Running twine check for qe6502 Python publish wheel"
        VERBATIM
    )
endif()
