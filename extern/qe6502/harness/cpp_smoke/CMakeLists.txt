enable_language(CXX)
add_executable(qe6502_cpp_smoke
    main.cpp
)

target_link_libraries(qe6502_cpp_smoke
    PRIVATE qe6502_cpp_static
)

qe6502_configure_warnings(qe6502_cpp_smoke)

target_compile_features(qe6502_cpp_smoke PRIVATE cxx_std_17)

add_test(NAME cpp_smoke COMMAND qe6502_cpp_smoke)
