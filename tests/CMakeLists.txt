add_executable(qe6502_smoke
    qe6502_smoke.c
)

target_link_libraries(qe6502_smoke PRIVATE
    qe6502::static
    qe6502_tools_sokol
)

add_test(NAME qe6502_smoke COMMAND qe6502_smoke)


add_executable(qeaii_diskii_spindown_smoke
    qeaii_diskii_spindown_smoke.c
)

target_link_libraries(qeaii_diskii_spindown_smoke PRIVATE
    qeaii::qeaii
)

add_test(NAME qeaii_diskii_spindown_smoke COMMAND qeaii_diskii_spindown_smoke)

add_executable(qeaii_diskii_boot_smoke
    qeaii_diskii_boot_smoke.c
    ../appleii_sokol/src/dsk2nib.c
)

target_include_directories(qeaii_diskii_boot_smoke PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../appleii_sokol/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/../appleii/roms"
)

target_link_libraries(qeaii_diskii_boot_smoke PRIVATE
    qeaii::qeaii
)

add_test(NAME qeaii_diskii_boot_smoke COMMAND qeaii_diskii_boot_smoke)
