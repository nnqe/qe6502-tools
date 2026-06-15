# Toolchain for freestanding WebAssembly build using LLVM/Clang.
# Target: wasm32-unknown-unknown
#
# This does not use Emscripten or WASI.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR wasm32)

set(QE6502_WASM_TARGET "wasm32-unknown-unknown")

include("${CMAKE_CURRENT_LIST_DIR}/../modules/Qe6502FindLLVM.cmake")

set(CMAKE_C_COMPILER "${QE6502_CLANG}" CACHE FILEPATH "C compiler" FORCE)
set(CMAKE_C_COMPILER_TARGET "${QE6502_WASM_TARGET}" CACHE STRING "C compiler target" FORCE)

set(CMAKE_AR "${QE6502_LLVM_AR}" CACHE FILEPATH "LLVM archiver" FORCE)
set(CMAKE_RANLIB "${QE6502_LLVM_RANLIB}" CACHE FILEPATH "LLVM ranlib" FORCE)
set(CMAKE_LINKER "${QE6502_WASM_LD}" CACHE FILEPATH "WASM linker" FORCE)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_EXE_LINKER_FLAGS_INIT
    "-fuse-ld=lld"
)

message(STATUS "QE6502 wasm target: ${QE6502_WASM_TARGET}")
message(STATUS "QE6502 wasm C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "QE6502 wasm C compiler target: ${CMAKE_C_COMPILER_TARGET}")
message(STATUS "QE6502 wasm linker: ${CMAKE_LINKER}")
