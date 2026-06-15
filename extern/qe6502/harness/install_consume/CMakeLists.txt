set(QE6502_INSTALL_CONSUME_SCRIPT
    ${CMAKE_CURRENT_SOURCE_DIR}/run_install_consume.cmake
)

function(qe6502_add_install_consume_test case_name)
    add_test(NAME qe6502.install_consume.${case_name}
        COMMAND ${CMAKE_COMMAND}
            -DQE6502_SOURCE_DIR=${PROJECT_SOURCE_DIR}
            -DQE6502_WORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/${case_name}
            -DQE6502_CONSUMER_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/consumer_cpp
            -DQE6502_INSTALL_CONSUME_CASE=${case_name}
            -DQE6502_GENERATOR=${CMAKE_GENERATOR}
            -DQE6502_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}
            -DQE6502_GENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}
            -DQE6502_TEST_CONFIG=$<CONFIG>
            -P ${QE6502_INSTALL_CONSUME_SCRIPT}
    )

    set_tests_properties(qe6502.install_consume.${case_name} PROPERTIES
        LABELS "install;consume;${case_name};cpp;packaging"
        TIMEOUT 180
    )
endfunction()

qe6502_add_install_consume_test(dynamic)
qe6502_add_install_consume_test(static)
qe6502_add_install_consume_test(both)
