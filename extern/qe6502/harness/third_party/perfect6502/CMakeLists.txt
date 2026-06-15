add_library(perfect6502 STATIC
    perfect6502.c
    netlist_sim.c
)

# perfect6502 exhibits incorrect behavior when built with optimizations enabled.
target_compile_options(perfect6502 PRIVATE
    $<$<C_COMPILER_ID:MSVC>:/Od>
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:-O0>
)

target_include_directories(perfect6502
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(perfect6502 PUBLIC c_std_99)
