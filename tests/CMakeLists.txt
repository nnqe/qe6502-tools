add_executable(qe6502_smoke
    qe6502_smoke.c
)

target_link_libraries(qe6502_smoke PRIVATE
    qe6502::static
    qe6502_tools_sokol
)

add_test(NAME qe6502_smoke COMMAND qe6502_smoke)
