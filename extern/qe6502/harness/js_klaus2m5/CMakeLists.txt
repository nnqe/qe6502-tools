find_program(QE6502_NODE_EXECUTABLE node)

if(NOT QE6502_NODE_EXECUTABLE)
    message(FATAL_ERROR "qe6502 JS Klaus2m5 tests require Node.js when QE6502_BUILD_WASM and BUILD_TESTING are enabled")
endif()

if(QE6502_NODE_EXECUTABLE)
    set(QE6502_JS_KLAUS2M5_TESTS
        nmos standard
        wdc standard
        wdc extended
        rw standard
        rw extended
        st standard
    )

    list(LENGTH QE6502_JS_KLAUS2M5_TESTS QE6502_JS_KLAUS2M5_TEST_COUNT)
    math(EXPR QE6502_JS_KLAUS2M5_LAST_INDEX "${QE6502_JS_KLAUS2M5_TEST_COUNT} - 1")

    foreach(index RANGE 0 ${QE6502_JS_KLAUS2M5_LAST_INDEX} 2)
        list(GET QE6502_JS_KLAUS2M5_TESTS ${index} model)
        math(EXPR test_index "${index} + 1")
        list(GET QE6502_JS_KLAUS2M5_TESTS ${test_index} test_name)

        add_test(
            NAME qe6502.js.klaus2m5.${model}.${test_name}
            COMMAND
                "${QE6502_NODE_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/js_klaus2m5.mjs"
                "$<TARGET_FILE:qe6502_js>"
                "${CMAKE_SOURCE_DIR}/harness/klaus2m5"
                "${model}"
                "${test_name}"
        )

        set_tests_properties(qe6502.js.klaus2m5.${model}.${test_name} PROPERTIES
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            LABELS "js;wasm;v2;klaus;functional;performance"
            TIMEOUT 180
        )
    endforeach()


    add_test(
        NAME qe6502.js.save_load.klaus2m5.rw.extended
        COMMAND
            "${QE6502_NODE_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/js_save_load_klaus2m5.mjs"
            "$<TARGET_FILE:qe6502_js>"
            "${CMAKE_SOURCE_DIR}/harness/klaus2m5"
    )

    add_test(
        NAME qe6502.js.save_load.nes.functional
        COMMAND
            "${QE6502_NODE_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/js_save_load_functional.mjs"
            "$<TARGET_FILE:qe6502_js>"
    )

    set_tests_properties(qe6502.js.save_load.klaus2m5.rw.extended PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        LABELS "js;wasm;v2;klaus;save_load;functional"
        TIMEOUT 180
    )

    set_tests_properties(qe6502.js.save_load.nes.functional PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        LABELS "js;wasm;v2;nes;save_load;functional"
        TIMEOUT 30
    )
endif()
