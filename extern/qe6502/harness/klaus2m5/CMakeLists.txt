set(target klaus2m5)

file(GLOB_RECURSE KLAUS2M5_PRIVATE_SOURCE
        main.c
        klaus2m5.c
        test_roms.c)

add_executable(${target}
    ${KLAUS2M5_PRIVATE_SOURCE}
)

target_link_libraries(${target}
    PRIVATE qe6502::static
)

qe6502_configure_warnings(${target})

target_compile_features(${target} PUBLIC c_std_11)

add_test(NAME qe6502.v2.klaus2m5.nmos.standard
    COMMAND ${target} nmos standard
)

set_tests_properties(
    qe6502.v2.klaus2m5.nmos.standard
    PROPERTIES
        LABELS "cpu;v2;klaus;functional"
        TIMEOUT 30
)

add_test(NAME qe6502.v2.klaus2m5.wdc.standard
    COMMAND ${target} wdc standard
)

add_test(NAME qe6502.v2.klaus2m5.wdc.extended
    COMMAND ${target} wdc extended
)

add_test(NAME qe6502.v2.klaus2m5.rw.standard
    COMMAND ${target} rw standard
)

add_test(NAME qe6502.v2.klaus2m5.rw.extended
    COMMAND ${target} rw extended
)

add_test(NAME qe6502.v2.klaus2m5.st.standard
    COMMAND ${target} st standard
)

set_tests_properties(
    qe6502.v2.klaus2m5.wdc.standard
    qe6502.v2.klaus2m5.wdc.extended
    qe6502.v2.klaus2m5.rw.standard
    qe6502.v2.klaus2m5.rw.extended
    qe6502.v2.klaus2m5.st.standard
    PROPERTIES
        LABELS "cpu;v2;klaus;functional"
        TIMEOUT 30
)

enable_language(CXX)

set(save_load_target klaus2m5_save_load)

add_executable(${save_load_target}
    save_load.cpp
    test_roms.c
)

target_link_libraries(${save_load_target}
    PRIVATE qe6502_cpp
)

qe6502_configure_warnings(${save_load_target})

target_compile_features(${save_load_target} PUBLIC cxx_std_17)

add_test(NAME qe6502.v2.save_load.klaus2m5.rw.extended
    COMMAND ${save_load_target} klaus-rw-extended
)

add_test(NAME qe6502.v2.save_load.nes.functional
    COMMAND ${save_load_target} nes-functional
)

set_tests_properties(qe6502.v2.save_load.klaus2m5.rw.extended PROPERTIES
    LABELS "cpu;v2;klaus;save_load;functional"
    TIMEOUT 30
)

set_tests_properties(qe6502.v2.save_load.nes.functional PROPERTIES
    LABELS "cpu;v2;nes;save_load;functional"
    TIMEOUT 30
)
