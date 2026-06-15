find_program(DOTNET_EXECUTABLE dotnet)

if(NOT QE6502_BUILD_CSHARP)
    message(STATUS "QE6502_BUILD_CSHARP is OFF; skipping qe6502 C# smoke harness")
    return()
endif()

if(NOT DOTNET_EXECUTABLE)
    message(STATUS "dotnet not found; skipping qe6502 C# smoke harness")
    return()
endif()

if(NOT TARGET qe6502_shared)
    message(STATUS "qe6502_shared target is not available; skipping qe6502 C# smoke harness")
    return()
endif()

if(NOT TARGET qe6502_csharp)
    message(STATUS "qe6502_csharp target is not available; skipping qe6502 C# smoke harness")
    return()
endif()

set(QE6502_CS_SMOKE_CONFIGURATION "$<IF:$<CONFIG:Debug>,Debug,Release>")
set(QE6502_CS_SMOKE_PROJECT ${CMAKE_CURRENT_SOURCE_DIR}/Qe6502.CsSmoke.csproj)
set(QE6502_CS_BINDING_OUTPUT_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/Qe6502/bin/${QE6502_CS_SMOKE_CONFIGURATION}/netstandard2.0
)
set(QE6502_CS_SMOKE_OUTPUT_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/Qe6502.CsSmoke/bin/${QE6502_CS_SMOKE_CONFIGURATION}/net8.0
)
set(QE6502_CS_BINDING_DLL ${QE6502_CS_BINDING_OUTPUT_DIR}/Qe6502.dll)
set(QE6502_CS_SMOKE_DLL ${QE6502_CS_SMOKE_OUTPUT_DIR}/Qe6502.CsSmoke.dll)

if(WIN32)
    set(QE6502_CS_SMOKE_NATIVE_LIBRARY_NAME "$<TARGET_FILE_NAME:qe6502_shared>")
else()
    set(QE6502_CS_SMOKE_NATIVE_LIBRARY_NAME "$<TARGET_LINKER_FILE_NAME:qe6502_shared>")
endif()

add_custom_target(qe6502_cs_smoke ALL
    COMMAND ${DOTNET_EXECUTABLE} build ${QE6502_CS_SMOKE_PROJECT}
        --configuration ${QE6502_CS_SMOKE_CONFIGURATION}
        -p:Platform=AnyCPU
        -p:PlatformTarget=AnyCPU
        -p:Version=${QE6502_PACKAGE_VERSION}
        -p:Qe6502CMakeArtifactsRoot=${CMAKE_CURRENT_BINARY_DIR}
        -p:Qe6502GeneratedVersionSource=${QE6502_CSHARP_GENERATED_VERSION_SOURCE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QE6502_CS_SMOKE_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:qe6502_shared>
        ${QE6502_CS_SMOKE_OUTPUT_DIR}/${QE6502_CS_SMOKE_NATIVE_LIBRARY_NAME}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${QE6502_CS_BINDING_DLL}
        ${QE6502_CS_SMOKE_OUTPUT_DIR}/Qe6502.dll
    DEPENDS qe6502_shared qe6502_csharp
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building qe6502 C# smoke harness"
    VERBATIM
)

add_custom_target(qe6502_cs_smoke_run
    COMMAND ${DOTNET_EXECUTABLE} ${QE6502_CS_SMOKE_DLL}
    DEPENDS qe6502_cs_smoke
    WORKING_DIRECTORY ${QE6502_CS_SMOKE_OUTPUT_DIR}
    COMMENT "Running qe6502 C# smoke harness"
    VERBATIM
)

add_test(NAME qe6502_cs_smoke
    COMMAND ${DOTNET_EXECUTABLE} ${QE6502_CS_SMOKE_DLL}
    WORKING_DIRECTORY ${QE6502_CS_SMOKE_OUTPUT_DIR}
)
