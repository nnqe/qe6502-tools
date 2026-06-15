set(QE6502_JS_MANUAL_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(QE6502_JS_MANUAL_ROM_DIR "${QE6502_JS_MANUAL_DIR}/roms")

set(QE6502_JS_MANUAL_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/index.html"
    "${CMAKE_CURRENT_SOURCE_DIR}/js_manual.js"
    "${CMAKE_CURRENT_SOURCE_DIR}/js_singlestep.js"
    "${CMAKE_CURRENT_SOURCE_DIR}/single_step_sources.js"
)

set(QE6502_JS_MANUAL_BYPRODUCTS
    "${QE6502_JS_MANUAL_DIR}/index.html"
    "${QE6502_JS_MANUAL_DIR}/js_manual.js"
    "${QE6502_JS_MANUAL_DIR}/js_singlestep.js"
    "${QE6502_JS_MANUAL_DIR}/single_step_sources.js"
    "${QE6502_JS_MANUAL_DIR}/qe6502.js"
    "${QE6502_JS_MANUAL_DIR}/qe6502_js.wasm"
    "${QE6502_JS_MANUAL_ROM_DIR}/6502_functional_test.hex"
    "${QE6502_JS_MANUAL_ROM_DIR}/65C02_extended_opcodes_test.hex"
)

add_custom_target(qe6502_js_manual ALL
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${QE6502_JS_MANUAL_ROM_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/index.html"
        "${QE6502_JS_MANUAL_DIR}/index.html"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/js_manual.js"
        "${QE6502_JS_MANUAL_DIR}/js_manual.js"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/js_singlestep.js"
        "${QE6502_JS_MANUAL_DIR}/js_singlestep.js"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/single_step_sources.js"
        "${QE6502_JS_MANUAL_DIR}/single_step_sources.js"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_SOURCE_DIR}/binds/js/qe6502.js"
        "${QE6502_JS_MANUAL_DIR}/qe6502.js"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE:qe6502_js>"
        "${QE6502_JS_MANUAL_DIR}/qe6502_js.wasm"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_SOURCE_DIR}/harness/klaus2m5/6502_functional_test.hex"
        "${QE6502_JS_MANUAL_ROM_DIR}/6502_functional_test.hex"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_SOURCE_DIR}/harness/klaus2m5/65C02_extended_opcodes_test.hex"
        "${QE6502_JS_MANUAL_ROM_DIR}/65C02_extended_opcodes_test.hex"
    BYPRODUCTS
        ${QE6502_JS_MANUAL_BYPRODUCTS}
    DEPENDS
        qe6502_js
        ${QE6502_JS_MANUAL_SOURCES}
        "${CMAKE_SOURCE_DIR}/binds/js/qe6502.js"
        "${CMAKE_SOURCE_DIR}/harness/klaus2m5/6502_functional_test.hex"
        "${CMAKE_SOURCE_DIR}/harness/klaus2m5/65C02_extended_opcodes_test.hex"
    COMMENT "Copying qe6502 browser manual JS test files"
    VERBATIM
)
