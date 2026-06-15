set(QE6502_RUST_TOOL_HINTS)

foreach(QE6502_RUST_HOME_VAR CARGO_HOME RUSTUP_HOME)
    if(DEFINED ENV{${QE6502_RUST_HOME_VAR}} AND NOT "$ENV{${QE6502_RUST_HOME_VAR}}" STREQUAL "")
        file(TO_CMAKE_PATH "$ENV{${QE6502_RUST_HOME_VAR}}" QE6502_RUST_HOME_PATH)
        list(APPEND QE6502_RUST_TOOL_HINTS "${QE6502_RUST_HOME_PATH}/bin")
    endif()
endforeach()

if(DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
    file(TO_CMAKE_PATH "$ENV{HOME}" QE6502_RUST_USER_HOME)
    list(APPEND QE6502_RUST_TOOL_HINTS "${QE6502_RUST_USER_HOME}/.cargo/bin")
endif()

if(WIN32 AND DEFINED ENV{USERPROFILE} AND NOT "$ENV{USERPROFILE}" STREQUAL "")
    file(TO_CMAKE_PATH "$ENV{USERPROFILE}" QE6502_RUST_USERPROFILE)
    list(APPEND QE6502_RUST_TOOL_HINTS "${QE6502_RUST_USERPROFILE}/.cargo/bin")
endif()

list(REMOVE_DUPLICATES QE6502_RUST_TOOL_HINTS)

find_program(CARGO_EXECUTABLE
    NAMES cargo cargo.exe
    HINTS ${QE6502_RUST_TOOL_HINTS}
)

if(NOT CARGO_EXECUTABLE)
    message(STATUS "cargo not found; skipping qe6502 Rust binding")
    return()
endif()

set(QE6502_RUST_MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml")
set(QE6502_RUST_TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/target")
set(QE6502_RUST_PACKAGE_DIR "${CMAKE_BINARY_DIR}/rust-package/qe6502")
set(QE6502_RUST_PACKAGE_MANIFEST "${QE6502_RUST_PACKAGE_DIR}/Cargo.toml")

add_custom_target(qe6502_rust ALL
    COMMAND "${CARGO_EXECUTABLE}" build
        --manifest-path "${QE6502_RUST_MANIFEST}"
        --target-dir "${QE6502_RUST_TARGET_DIR}"
        --profile $<IF:$<CONFIG:Debug>,dev,release>
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Building qe6502 Rust binding"
    VERBATIM
)

find_package(Python3 COMPONENTS Interpreter QUIET)

if(Python3_Interpreter_FOUND)
    add_custom_target(qe6502_rust_package_stage
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/package_crate.py"
            --repo-root "${CMAKE_SOURCE_DIR}"
            --out "${QE6502_RUST_PACKAGE_DIR}"
            --version "${QE6502_PACKAGE_VERSION}"
        COMMENT "Staging self-contained qe6502 Rust crate"
        VERBATIM
    )

    add_custom_target(qe6502_rust_package_check
        COMMAND "${CARGO_EXECUTABLE}" package
            --manifest-path "${QE6502_RUST_PACKAGE_MANIFEST}"
            --allow-dirty
        WORKING_DIRECTORY "${QE6502_RUST_PACKAGE_DIR}"
        COMMENT "Checking qe6502 Rust crate package"
        DEPENDS qe6502_rust_package_stage
        VERBATIM
    )

    add_custom_target(qe6502_rust_publish_dry_run
        COMMAND "${CARGO_EXECUTABLE}" publish
            --manifest-path "${QE6502_RUST_PACKAGE_MANIFEST}"
            --dry-run
            --allow-dirty
        WORKING_DIRECTORY "${QE6502_RUST_PACKAGE_DIR}"
        COMMENT "Dry-running qe6502 Rust crate publish"
        DEPENDS qe6502_rust_package_check
        VERBATIM
    )
else()
    message(STATUS "Python3 interpreter not found; skipping qe6502 Rust package staging targets")
endif()

set(QE6502_RUST_MANIFEST "${QE6502_RUST_MANIFEST}" CACHE INTERNAL "qe6502 Rust binding manifest")
set(QE6502_RUST_TARGET_DIR "${QE6502_RUST_TARGET_DIR}" CACHE INTERNAL "qe6502 Rust binding target directory")
set(QE6502_RUST_PACKAGE_DIR "${QE6502_RUST_PACKAGE_DIR}" CACHE INTERNAL "qe6502 Rust generated package directory")
set(QE6502_RUST_PACKAGE_MANIFEST "${QE6502_RUST_PACKAGE_MANIFEST}" CACHE INTERNAL "qe6502 Rust generated package manifest")
