find_program(QE6502_NODE_EXECUTABLE node)
find_program(QE6502_NPM_EXECUTABLE NAMES npm.cmd npm)

if(NOT QE6502_NODE_EXECUTABLE)
    message(STATUS "node not found; skipping qe6502 JS npm package smoke")
    return()
endif()

if(NOT QE6502_NPM_EXECUTABLE)
    message(STATUS "npm not found; skipping qe6502 JS npm package smoke")
    return()
endif()

if(NOT TARGET qe6502_js_npm_package_stage)
    message(STATUS "qe6502_js_npm_package_stage target is not available; skipping qe6502 JS npm package smoke")
    return()
endif()

set(QE6502_JS_NPM_PACKAGE_DIR "${CMAKE_BINARY_DIR}/binds/js/npm/qe6502")
set(QE6502_JS_NPM_PACK_DIR "${CMAKE_CURRENT_BINARY_DIR}/pack")
set(QE6502_JS_NPM_TARBALL "${QE6502_JS_NPM_PACK_DIR}/qe6502-${QE6502_PACKAGE_VERSION}.tgz")
set(QE6502_JS_NPM_FILE_LIST "${QE6502_JS_NPM_PACK_DIR}/qe6502-${QE6502_PACKAGE_VERSION}-files.txt")
set(QE6502_JS_NPM_CONSUMER_DIR "${CMAKE_CURRENT_BINARY_DIR}/consumer")
set(QE6502_JS_NPM_SMOKE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/package_consumer_smoke.mjs")
set(QE6502_JS_NPM_SMOKE_DRIVER "${CMAKE_CURRENT_SOURCE_DIR}/run_npm_package_smoke.cmake")
set(QE6502_JS_NPM_FILE_LIST_DRIVER "${CMAKE_CURRENT_SOURCE_DIR}/write_npm_package_file_list.cmake")
set(QE6502_JS_NPM_PUBLISH_DRY_RUN_DRIVER "${CMAKE_CURRENT_SOURCE_DIR}/run_npm_publish_dry_run.cmake")

add_custom_target(qe6502_js_npm_package_pack
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${QE6502_JS_NPM_PACK_DIR}"
    COMMAND "${QE6502_NPM_EXECUTABLE}" pack --pack-destination "${QE6502_JS_NPM_PACK_DIR}"
    DEPENDS qe6502_js_npm_package_stage
    WORKING_DIRECTORY "${QE6502_JS_NPM_PACKAGE_DIR}"
    COMMENT "Packing qe6502 JavaScript npm package"
    VERBATIM
)

add_custom_command(
    OUTPUT "${QE6502_JS_NPM_FILE_LIST}"
    COMMAND "${CMAKE_COMMAND}"
        -DQE6502_TARBALL=${QE6502_JS_NPM_TARBALL}
        -DQE6502_FILE_LIST=${QE6502_JS_NPM_FILE_LIST}
        -P ${QE6502_JS_NPM_FILE_LIST_DRIVER}
    DEPENDS qe6502_js_npm_package_pack
    COMMENT "Writing qe6502 JavaScript npm package file list"
    VERBATIM
)

add_custom_target(qe6502_js_npm_package_file_list
    DEPENDS "${QE6502_JS_NPM_FILE_LIST}"
)

add_custom_target(qe6502_js_npm_publish_dry_run
    COMMAND "${CMAKE_COMMAND}"
        -DQE6502_NPM_EXECUTABLE=${QE6502_NPM_EXECUTABLE}
        -DQE6502_PACKAGE_DIR=${QE6502_JS_NPM_PACKAGE_DIR}
        -P ${QE6502_JS_NPM_PUBLISH_DRY_RUN_DRIVER}
    DEPENDS qe6502_js_npm_package_stage
    COMMENT "Running qe6502 JavaScript npm publish dry-run"
    VERBATIM
)

add_test(
    NAME qe6502_js_npm_package_smoke
    COMMAND "${CMAKE_COMMAND}"
        -DQE6502_NPM_EXECUTABLE=${QE6502_NPM_EXECUTABLE}
        -DQE6502_NODE_EXECUTABLE=${QE6502_NODE_EXECUTABLE}
        -DQE6502_PACKAGE_DIR=${QE6502_JS_NPM_PACKAGE_DIR}
        -DQE6502_PACK_DIR=${QE6502_JS_NPM_PACK_DIR}
        -DQE6502_TARBALL=${QE6502_JS_NPM_TARBALL}
        -DQE6502_CONSUMER_DIR=${QE6502_JS_NPM_CONSUMER_DIR}
        -DQE6502_CONSUMER_SCRIPT=${QE6502_JS_NPM_SMOKE_SCRIPT}
        -P ${QE6502_JS_NPM_SMOKE_DRIVER}
)

set_tests_properties(qe6502_js_npm_package_smoke PROPERTIES
    LABELS "javascript;npm;packaging;smoke"
    TIMEOUT 120
)
