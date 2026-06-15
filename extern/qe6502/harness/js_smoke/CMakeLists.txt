find_program(QE6502_NODE_EXECUTABLE node)

if(NOT QE6502_NODE_EXECUTABLE)
    message(FATAL_ERROR "qe6502_js_smoke requires Node.js when QE6502_BUILD_WASM and BUILD_TESTING are enabled")
endif()

if(QE6502_NODE_EXECUTABLE)
    set(QE6502_JS_SMOKE_PACKAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/package_like")
    set(QE6502_JS_SMOKE_PACKAGE_MODULE "${QE6502_JS_SMOKE_PACKAGE_DIR}/qe6502.js")
    set(QE6502_JS_SMOKE_PACKAGE_WASM "${QE6502_JS_SMOKE_PACKAGE_DIR}/qe6502_js.wasm")

    add_custom_command(
        OUTPUT
            "${QE6502_JS_SMOKE_PACKAGE_MODULE}"
            "${QE6502_JS_SMOKE_PACKAGE_WASM}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${QE6502_JS_SMOKE_PACKAGE_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_SOURCE_DIR}/binds/js/qe6502.js"
            "${QE6502_JS_SMOKE_PACKAGE_MODULE}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:qe6502_js>"
            "${QE6502_JS_SMOKE_PACKAGE_WASM}"
        DEPENDS
            qe6502_js
            "${CMAKE_SOURCE_DIR}/binds/js/qe6502.js"
        VERBATIM
    )

    add_custom_target(qe6502_js_smoke_package_fixture ALL
        DEPENDS
            "${QE6502_JS_SMOKE_PACKAGE_MODULE}"
            "${QE6502_JS_SMOKE_PACKAGE_WASM}"
    )

    add_test(
        NAME qe6502_js_smoke
        COMMAND
            "${QE6502_NODE_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/js_smoke.mjs"
            "$<TARGET_FILE:qe6502_js>"
    )

    add_test(
        NAME qe6502_js_smoke_node_default
        COMMAND
            "${QE6502_NODE_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/js_smoke_node_default.mjs"
            "${QE6502_JS_SMOKE_PACKAGE_MODULE}"
    )

    set_tests_properties(qe6502_js_smoke qe6502_js_smoke_node_default PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
endif()
