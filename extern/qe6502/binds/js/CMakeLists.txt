add_executable(qe6502_js
    qe6502_js.c
)

target_compile_features(qe6502_js PRIVATE c_std_11)

target_compile_options(qe6502_js PRIVATE
    -ffreestanding
    -fno-builtin
)

qe6502_configure_warnings(qe6502_js)

target_link_libraries(qe6502_js
    PRIVATE
        -Wl,--whole-archive
        qe6502_static
        -Wl,--no-whole-archive
)

target_link_options(qe6502_js PRIVATE
    -nostdlib
    LINKER:--no-entry
    ${QE6502_WASM_EXPORT_MEMORY_LINK_OPTION}
    LINKER:--export=qe6502js_context_pool_reset
    LINKER:--export=qe6502js_context_alloc
    LINKER:--export=qe6502js_context_free
    LINKER:--export=qe6502abi_version
    LINKER:--export=qe6502abi_setup
    LINKER:--export=qe6502abi_restart
    LINKER:--export=qe6502abi_goto
    LINKER:--export=qe6502abi_irq_assert
    LINKER:--export=qe6502abi_is_irq_asserted
    LINKER:--export=qe6502abi_save
    LINKER:--export=qe6502abi_load
    LINKER:--export=qe6502abi_nmi_assert
    LINKER:--export=qe6502abi_is_nmi_asserted
    LINKER:--export=qe6502abi_tick
    LINKER:--export=qe6502abi_get_pc
    LINKER:--export=qe6502abi_set_pc
    LINKER:--export=qe6502abi_get_s
    LINKER:--export=qe6502abi_set_s
    LINKER:--export=qe6502abi_get_a
    LINKER:--export=qe6502abi_set_a
    LINKER:--export=qe6502abi_get_x
    LINKER:--export=qe6502abi_set_x
    LINKER:--export=qe6502abi_get_y
    LINKER:--export=qe6502abi_set_y
    LINKER:--export=qe6502abi_get_p
    LINKER:--export=qe6502abi_set_p
    LINKER:--export=qe6502abi_get_model
    LINKER:--export=qe6502abi_set_model
)

set_target_properties(qe6502_js PROPERTIES
    OUTPUT_NAME "qe6502_js"
    SUFFIX ".wasm"
)

set(QE6502_JS_CONFIGURED_MODULE "${CMAKE_CURRENT_BINARY_DIR}/qe6502.js")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/qe6502.js"
    "${QE6502_JS_CONFIGURED_MODULE}"
    @ONLY
)

set(QE6502_JS_NPM_PACKAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/npm/qe6502")
set(QE6502_JS_NPM_PACKAGE_JSON "${QE6502_JS_NPM_PACKAGE_DIR}/package.json")
set(QE6502_JS_NPM_PACKAGE_README "${QE6502_JS_NPM_PACKAGE_DIR}/README.md")
set(QE6502_JS_NPM_PACKAGE_LICENSE "${QE6502_JS_NPM_PACKAGE_DIR}/LICENSE")
set(QE6502_JS_NPM_PACKAGE_MODULE "${QE6502_JS_NPM_PACKAGE_DIR}/qe6502.js")
set(QE6502_JS_NPM_PACKAGE_WASM "${QE6502_JS_NPM_PACKAGE_DIR}/qe6502_js.wasm")

file(MAKE_DIRECTORY "${QE6502_JS_NPM_PACKAGE_DIR}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/package.json.in"
    "${QE6502_JS_NPM_PACKAGE_JSON}"
    @ONLY
)

add_custom_command(
    OUTPUT
        "${QE6502_JS_NPM_PACKAGE_README}"
        "${QE6502_JS_NPM_PACKAGE_LICENSE}"
        "${QE6502_JS_NPM_PACKAGE_MODULE}"
        "${QE6502_JS_NPM_PACKAGE_WASM}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${QE6502_JS_NPM_PACKAGE_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
        "${QE6502_JS_NPM_PACKAGE_README}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_SOURCE_DIR}/LICENSE"
        "${QE6502_JS_NPM_PACKAGE_LICENSE}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${QE6502_JS_CONFIGURED_MODULE}"
        "${QE6502_JS_NPM_PACKAGE_MODULE}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE:qe6502_js>"
        "${QE6502_JS_NPM_PACKAGE_WASM}"
    DEPENDS
        qe6502_js
        "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
        "${QE6502_JS_CONFIGURED_MODULE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/qe6502.js"
        "${CMAKE_SOURCE_DIR}/LICENSE"
    VERBATIM
)

add_custom_target(qe6502_js_npm_package_stage ALL
    DEPENDS
        "${QE6502_JS_NPM_PACKAGE_JSON}"
        "${QE6502_JS_NPM_PACKAGE_README}"
        "${QE6502_JS_NPM_PACKAGE_LICENSE}"
        "${QE6502_JS_NPM_PACKAGE_MODULE}"
        "${QE6502_JS_NPM_PACKAGE_WASM}"
)
