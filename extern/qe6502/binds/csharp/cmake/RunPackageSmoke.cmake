if(NOT DEFINED DOTNET_EXECUTABLE OR DOTNET_EXECUTABLE STREQUAL "")
    message(FATAL_ERROR "DOTNET_EXECUTABLE is required")
endif()

if(NOT DEFINED QE6502_CSHARP_PACKAGE_DIR OR QE6502_CSHARP_PACKAGE_DIR STREQUAL "")
    message(FATAL_ERROR "QE6502_CSHARP_PACKAGE_DIR is required")
endif()

if(NOT DEFINED QE6502_CSHARP_PACKAGE_VERSION OR QE6502_CSHARP_PACKAGE_VERSION STREQUAL "")
    message(FATAL_ERROR "QE6502_CSHARP_PACKAGE_VERSION is required")
endif()

if(NOT DEFINED QE6502_CSHARP_SMOKE_DIR OR QE6502_CSHARP_SMOKE_DIR STREQUAL "")
    message(FATAL_ERROR "QE6502_CSHARP_SMOKE_DIR is required")
endif()

file(GLOB qe6502_csharp_packages
    "${QE6502_CSHARP_PACKAGE_DIR}/Qe6502.${QE6502_CSHARP_PACKAGE_VERSION}.nupkg"
)

list(LENGTH qe6502_csharp_packages qe6502_csharp_package_count)
if(NOT qe6502_csharp_package_count EQUAL 1)
    message(FATAL_ERROR
        "Expected exactly one Qe6502.${QE6502_CSHARP_PACKAGE_VERSION}.nupkg in ${QE6502_CSHARP_PACKAGE_DIR}, found ${qe6502_csharp_package_count}"
    )
endif()

file(REMOVE_RECURSE "${QE6502_CSHARP_SMOKE_DIR}")
file(MAKE_DIRECTORY "${QE6502_CSHARP_SMOKE_DIR}")

execute_process(
    COMMAND "${DOTNET_EXECUTABLE}" new console --framework net8.0 --output "${QE6502_CSHARP_SMOKE_DIR}"
    RESULT_VARIABLE qe6502_csharp_new_result
)
if(NOT qe6502_csharp_new_result EQUAL 0)
    message(FATAL_ERROR "dotnet new console failed")
endif()

execute_process(
    COMMAND "${DOTNET_EXECUTABLE}" add "${QE6502_CSHARP_SMOKE_DIR}" package Qe6502
        --version "${QE6502_CSHARP_PACKAGE_VERSION}"
        --source "${QE6502_CSHARP_PACKAGE_DIR}"
    RESULT_VARIABLE qe6502_csharp_add_result
)
if(NOT qe6502_csharp_add_result EQUAL 0)
    message(FATAL_ERROR "dotnet add package Qe6502 failed")
endif()

file(WRITE "${QE6502_CSHARP_SMOKE_DIR}/Program.cs" [=[
using System;
using Qe6502;

var cpu = new Cpu(Model.Nmos);
cpu.JumpTo(0x0200);

if (cpu.Address != 0x0200 || !cpu.IsOpcodeFetch || cpu.IsWrite) {
    throw new Exception("Unexpected qe6502 C# smoke-test bus state after JumpTo.");
}

Console.WriteLine("qe6502 C# NuGet package smoke passed");
]=])

execute_process(
    COMMAND "${DOTNET_EXECUTABLE}" run --project "${QE6502_CSHARP_SMOKE_DIR}" --configuration Release
    RESULT_VARIABLE qe6502_csharp_run_result
)
if(NOT qe6502_csharp_run_result EQUAL 0)
    message(FATAL_ERROR "dotnet run for Qe6502 package smoke failed")
endif()
