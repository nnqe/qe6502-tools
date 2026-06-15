if(NOT QE6502_BUILD_RUST)
    message(STATUS "QE6502_BUILD_RUST is OFF; skipping qe6502 Rust smoke harness")
    return()
endif()

if(NOT CARGO_EXECUTABLE)
    message(STATUS "cargo not found; skipping qe6502 Rust smoke harness")
    return()
endif()

if(NOT TARGET qe6502_rust)
    message(STATUS "qe6502_rust target is not available; skipping qe6502 Rust smoke harness")
    return()
endif()

set(QE6502_RUST_SMOKE_MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml")
set(QE6502_RUST_SMOKE_TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/target")

add_custom_target(qe6502_rust_smoke ALL
    COMMAND "${CARGO_EXECUTABLE}" build
        --manifest-path "${QE6502_RUST_SMOKE_MANIFEST}"
        --target-dir "${QE6502_RUST_SMOKE_TARGET_DIR}"
        --profile $<IF:$<CONFIG:Debug>,dev,release>
    DEPENDS qe6502_rust
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Building qe6502 Rust smoke harness"
    VERBATIM
)

add_custom_target(qe6502_rust_smoke_run
    COMMAND "${CARGO_EXECUTABLE}" run
        --manifest-path "${QE6502_RUST_SMOKE_MANIFEST}"
        --target-dir "${QE6502_RUST_SMOKE_TARGET_DIR}"
        --profile $<IF:$<CONFIG:Debug>,dev,release>
    DEPENDS qe6502_rust_smoke
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Running qe6502 Rust smoke harness"
    VERBATIM
)

add_test(NAME qe6502_rust_smoke
    COMMAND "${CARGO_EXECUTABLE}" run
        --manifest-path "${QE6502_RUST_SMOKE_MANIFEST}"
        --target-dir "${QE6502_RUST_SMOKE_TARGET_DIR}"
        --profile $<IF:$<CONFIG:Debug>,dev,release>
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

set_tests_properties(qe6502_rust_smoke PROPERTIES
    LABELS "rust;smoke;functional"
    TIMEOUT 30
)
