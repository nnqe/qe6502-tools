find_program(DOTNET_EXECUTABLE dotnet)

if(NOT QE6502_BUILD_CSHARP)
    message(STATUS "QE6502_BUILD_CSHARP is OFF; skipping qe6502 C# Klaus2m5 harness")
    return()
endif()

if(NOT DOTNET_EXECUTABLE)
    message(STATUS "dotnet not found; skipping qe6502 C# Klaus2m5 harness")
    return()
endif()

if(NOT TARGET qe6502_shared)
    message(STATUS "qe6502_shared target is not available; skipping qe6502 C# Klaus2m5 harness")
    return()
endif()

if(NOT TARGET qe6502_csharp)
    message(STATUS "qe6502_csharp target is not available; skipping qe6502 C# Klaus2m5 harness")
    return()
endif()

set(QE6502_CS_KLAUS2M5_CONFIGURATION "$<IF:$<CONFIG:Debug>,Debug,Release>")
set(QE6502_CS_KLAUS2M5_PROJECT ${CMAKE_CURRENT_SOURCE_DIR}/Qe6502.CsKlaus2m5.csproj)
set(QE6502_CS_KLAUS2M5_OUTPUT_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/Qe6502.CsKlaus2m5/bin/${QE6502_CS_KLAUS2M5_CONFIGURATION}/net8.0
)
set(QE6502_CS_KLAUS2M5_ROM_OUTPUT_DIR
    ${QE6502_CS_KLAUS2M5_OUTPUT_DIR}/klaus2m5
)
set(QE6502_CS_BINDING_OUTPUT_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/Qe6502/bin/${QE6502_CS_KLAUS2M5_CONFIGURATION}/netstandard2.0
)
set(QE6502_CS_BINDING_DLL ${QE6502_CS_BINDING_OUTPUT_DIR}/Qe6502.dll)
set(QE6502_CS_KLAUS2M5_DLL ${QE6502_CS_KLAUS2M5_OUTPUT_DIR}/Qe6502.CsKlaus2m5.dll)

if(WIN32)
    set(QE6502_CS_KLAUS2M5_NATIVE_LIBRARY_NAME "$<TARGET_FILE_NAME:qe6502_shared>")
else()
    set(QE6502_CS_KLAUS2M5_NATIVE_LIBRARY_NAME "$<TARGET_LINKER_FILE_NAME:qe6502_shared>")
endif()

set(QE6502_CS_KLAUS2M5_STANDARD_ROM ${CMAKE_SOURCE_DIR}/harness/klaus2m5/6502_functional_test.hex)
set(QE6502_CS_KLAUS2M5_EXTENDED_ROM ${CMAKE_SOURCE_DIR}/harness/klaus2m5/65C02_extended_opcodes_test.hex)

add_custom_target(qe6502_cs_klaus2m5 ALL
    COMMAND ${DOTNET_EXECUTABLE} build ${QE6502_CS_KLAUS2M5_PROJECT}
        --configuration ${QE6502_CS_KLAUS2M5_CONFIGURATION}
        -p:Platform=AnyCPU
        -p:PlatformTarget=AnyCPU
        -p:Version=${QE6502_PACKAGE_VERSION}
        -p:Qe6502CMakeArtifactsRoot=${CMAKE_CURRENT_BINARY_DIR}
        -p:Qe6502GeneratedVersionSource=${QE6502_CSHARP_GENERATED_VERSION_SOURCE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QE6502_CS_KLAUS2M5_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:qe6502_shared>
        ${QE6502_CS_KLAUS2M5_OUTPUT_DIR}/${QE6502_CS_KLAUS2M5_NATIVE_LIBRARY_NAME}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${QE6502_CS_BINDING_DLL}
        ${QE6502_CS_KLAUS2M5_OUTPUT_DIR}/Qe6502.dll
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QE6502_CS_KLAUS2M5_ROM_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${QE6502_CS_KLAUS2M5_STANDARD_ROM}
        ${QE6502_CS_KLAUS2M5_ROM_OUTPUT_DIR}/6502_functional_test.hex
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${QE6502_CS_KLAUS2M5_EXTENDED_ROM}
        ${QE6502_CS_KLAUS2M5_ROM_OUTPUT_DIR}/65C02_extended_opcodes_test.hex
    DEPENDS qe6502_shared qe6502_csharp
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building qe6502 C# Klaus2m5 harness"
    VERBATIM
)

add_custom_target(qe6502_cs_klaus2m5_run
    COMMAND ${DOTNET_EXECUTABLE} ${QE6502_CS_KLAUS2M5_DLL}
    DEPENDS qe6502_cs_klaus2m5
    WORKING_DIRECTORY ${QE6502_CS_KLAUS2M5_OUTPUT_DIR}
    COMMENT "Running qe6502 C# Klaus2m5 harness"
    VERBATIM
)

set(QE6502_CS_KLAUS2M5_TESTS
    nmos standard
    wdc standard
    wdc extended
    rw standard
    rw extended
    st standard
)

list(LENGTH QE6502_CS_KLAUS2M5_TESTS QE6502_CS_KLAUS2M5_TEST_COUNT)
math(EXPR QE6502_CS_KLAUS2M5_LAST_INDEX "${QE6502_CS_KLAUS2M5_TEST_COUNT} - 1")

foreach(index RANGE 0 ${QE6502_CS_KLAUS2M5_LAST_INDEX} 2)
    list(GET QE6502_CS_KLAUS2M5_TESTS ${index} model)
    math(EXPR test_index "${index} + 1")
    list(GET QE6502_CS_KLAUS2M5_TESTS ${test_index} test_name)

    add_test(NAME qe6502.cs.klaus2m5.${model}.${test_name}
        COMMAND ${DOTNET_EXECUTABLE} ${QE6502_CS_KLAUS2M5_DLL} ${model} ${test_name}
        WORKING_DIRECTORY ${QE6502_CS_KLAUS2M5_OUTPUT_DIR}
    )

    set_tests_properties(qe6502.cs.klaus2m5.${model}.${test_name} PROPERTIES
        LABELS "cs;v2;klaus;functional;performance"
        TIMEOUT 180
    )
endforeach()
