enable_language(CXX)

set(target singlestep)

add_executable(${target}
    main.cpp
    ${PROJECT_SOURCE_DIR}/binds/cpp/src/cpu.cpp
)

target_link_libraries(${target}
    PRIVATE qe6502::static
)

target_include_directories(${target}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/harness/third_party
        ${PROJECT_SOURCE_DIR}/binds/cpp/include
)

qe6502_configure_warnings(${target})

target_compile_features(${target} PUBLIC cxx_std_17)
