if(QE6502_BUILD_WASM)
    add_subdirectory(js)
else()
    if(QE6502_BUILD_CPP)
        add_subdirectory(cpp)
    endif()

    if(QE6502_BUILD_CSHARP)
        add_subdirectory(csharp)
    endif()

    if(QE6502_BUILD_PYTHON)
        add_subdirectory(python)
    endif()

    if(QE6502_BUILD_JAVA)
        add_subdirectory(java)
    endif()

    if(QE6502_BUILD_RUST)
        add_subdirectory(rust)
    endif()
endif()
