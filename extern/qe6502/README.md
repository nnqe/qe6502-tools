# qe6502

`qe6502` is a small embeddable 6502/65C02 CPU core with an explicit external bus interface. The CPU asks for one bus cycle at a time; your emulator owns RAM, ROM, devices, mappers, wait states, and scheduling.

It is meant to be easy to drop into another project while still exposing deterministic, cycle-by-cycle bus behavior. The native C core has no hidden mutable global state and performs no memory or device I/O internally, so separate CPU contexts can be used independently.

Highlights:

- bus-level 6502-family CPU core, not a complete machine emulator;
- NMOS 6502, NES/2A03-style, WDC 65C02, Rockwell 65C02, and Synertek 65C02 models;
- fast native C API with a 16-byte `qe6502_t` CPU state;
- stable shared/FFI ABI with a 64-byte opaque context;
- fixed 64-byte save/load snapshots;
- C, C++, C#, Java, Python, Rust, and JavaScript/WebAssembly integration layers.

## Quick start

### Vendoring with CMake

The simplest native integration is to vendor the repository and add it as a subdirectory. In subproject builds, tests, tools, and install rules are off by default; the options below just make that explicit.

```cmake
set(QE6502_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(QE6502_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(QE6502_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(external/qe6502)

target_link_libraries(my_emulator PRIVATE qe6502::static)
# or, for the C++ wrapper:
# target_link_libraries(my_emulator PRIVATE qe6502::cpp)
```

### Minimal C tick loop

```c
#include <stdint.h>
#include <qe6502/qe6502.h>

static uint8_t memory[65536];

static uint8_t memory_read(uint16_t address)
{
    return memory[address];
}

static void memory_write(uint16_t address, uint8_t value)
{
    memory[address] = value;
}

int main(void)
{
    memory[0x8000] = 0xEA; /* NOP */
    memory[0xFFFC] = 0x00; /* reset vector low */
    memory[0xFFFD] = 0x80; /* reset vector high */

    qe6502_t cpu = qe6502_setup(qe6502_model_nmos);
    qe6502_tick_t tick = qe6502_restart(&cpu);

    for (int i = 0; i < 1000; ++i) {
        uint8_t input = 0;

        if (qe6502_is_write(tick)) {
            memory_write(tick.address, tick.bus);
        } else {
            input = memory_read(tick.address);
        }

        tick = qe6502_tick(&cpu, input);
    }

    return 0;
}
```

`qe6502_restart()` starts the normal reset sequence. If you want to skip reset-vector reads in a small harness, use `qe6502_goto(&cpu, 0x8000)` instead.

### Minimal C++ tick loop

```cpp
#include <cstdint>
#include <qe6502/cpu.hpp>

static std::uint8_t memory[65536]{};

std::uint8_t memory_read(std::uint16_t address)
{
    return memory[address];
}

void memory_write(std::uint16_t address, std::uint8_t value)
{
    memory[address] = value;
}

int main()
{
    memory[0x8000] = 0xEA; // NOP
    memory[0xFFFC] = 0x00; // reset vector low
    memory[0xFFFD] = 0x80; // reset vector high

    qe6502::cpu cpu(qe6502::model::nmos);

    cpu.restart();

    for (int i = 0; i < 1000; ++i) {
        std::uint8_t input = 0;

        if (cpu.is_write()) {
            memory_write(cpu.bus_address(), cpu.bus_data());
        } else {
            input = memory_read(cpu.bus_address());
        }

        cpu.tick(input);
    }
}
```

The C++ wrapper keeps the last bus request inside the `qe6502::cpu` object. The hot path uses `is_write()`, `bus_address()`, `bus_data()`, and `tick(input)`.

## Choosing an API

| Use this | When you want |
| --- | --- |
| `qe6502::static` + `<qe6502/qe6502.h>` | The fastest native C integration, compiled directly into your program. |
| `qe6502::shared` + `<qe6502/qe6502_abi.h>` | A stable ABI across dynamic-library, plugin, scripting, or FFI boundaries. |
| `qe6502::cpp` + `<qe6502/cpu.hpp>` | A small C++17 RAII-style wrapper around the native core. |
| C#, Java, Python, JavaScript/WASM | Higher-level bindings backed by the stable ABI. |
| Rust crate | A Cargo-managed wrapper that builds and statically links the native C core. |

Installed CMake packages export `qe6502::static`, `qe6502::shared`, and `qe6502::cpp` when those products are enabled. If both C++ variants are installed, explicit `qe6502::cpp_static` and `qe6502::cpp_shared` targets are also available.

## Build from source

A normal native release build:

```sh
cmake -S . -B build/release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DQE6502_BUILD_TESTS=OFF
cmake --build build/release
```

Install and consume from another CMake project:

```sh
cmake --install build/release --prefix /path/to/prefix
```

```cmake
find_package(qe6502 CONFIG REQUIRED)
target_link_libraries(my_program PRIVATE qe6502::static)
```

For non-CMake consumers, installed builds also generate pkg-config files for the enabled products:

```sh
pkg-config --cflags --libs qe6502
pkg-config --cflags --libs qe6502-abi
pkg-config --cflags --libs qe6502-cpp
```

Useful build options:

| Option | Default | Notes |
| --- | ---: | --- |
| `QE6502_BUILD_STATIC` | `ON` | Build the native static C library. |
| `QE6502_BUILD_SHARED` | `ON` | Build the shared stable ABI library. |
| `QE6502_BUILD_CPP` | `ON` | Build the C++ wrapper. |
| `QE6502_BUILD_TESTS` | `${BUILD_TESTING}` top-level, `OFF` as subproject | Build regression harnesses. |
| `QE6502_BUILD_TOOLS` | `${BUILD_TESTING}` top-level, `OFF` as subproject | Build developer tools used by some harnesses. |
| `QE6502_BUILD_CSHARP`, `QE6502_BUILD_JAVA`, `QE6502_BUILD_PYTHON`, `QE6502_BUILD_RUST` | `ON` | Build bindings when the required toolchains are available. |
| `QE6502_BUILD_WASM` | `OFF` | Build the JavaScript/WebAssembly target. |
| `QE6502_INSTALL` | `ON` top-level, `OFF` as subproject | Install headers, libraries, and package metadata. |

For dependency/package builds, you usually want `QE6502_BUILD_TESTS=OFF` and `QE6502_ENABLE_WERROR=OFF`.

## CPU models

The public model constants cover:

- NMOS 6502;
- NES/2A03-style 6502 behavior;
- WDC 65C02;
- Rockwell 65C02;
- Synertek 65C02.

The selected model controls the opcode matrix, bus-cycle behavior, interrupt behavior, and CPU-specific instruction details used by the core.

## Execution model

Each tick describes the next externally visible bus request:

- 16-bit bus address;
- current data bus value;
- status bits such as write/read, opcode fetch, reset-internal cycle, and jammed state.

For writes, store the outgoing bus byte into your memory or device map. For reads, load a byte from your memory or device map and pass it to the next tick.

This is the same basic loop exposed by the C API, the C++ wrapper, and the higher-level bindings.

## Other language snippets

The binding-specific README files and directories contain the fuller packaging/runtime notes. The central idea stays the same: create a CPU, start it with `restart()` or `jump_to()`/`JumpTo()`/`jumpTo()`, then service one bus request per tick.

### Rust

```rust
use qe6502::{Cpu, Model};

fn main() {
    let mut memory = [0u8; 65_536];
    memory[0x8000] = 0xEA; // NOP

    let mut cpu = Cpu::new(Model::Nmos);
    cpu.jump_to(0x8000);

    for _ in 0..1000 {
        let input = if cpu.is_write() {
            memory[cpu.bus_address() as usize] = cpu.bus_data();
            0
        } else {
            memory[cpu.bus_address() as usize]
        };

        cpu.tick(input);
    }
}
```

### Python

```python
import qe6502

memory = bytearray(65536)
memory[0x8000] = 0xEA  # NOP

cpu = qe6502.CPU(qe6502.MODEL_NMOS)
bus = cpu.jump_to(0x8000)

for _ in range(1000):
    address = bus & qe6502.TICK_ADDRESS_MASK

    if bus & qe6502.TICK_WRITING:
        memory[address] = (bus >> qe6502.TICK_BUS_SHIFT) & 0xff
        bus = cpu.tick()
    else:
        bus = cpu.tick(memory[address])
```

### C#

```csharp
using Qe6502;

var memory = new byte[65536];
memory[0x8000] = 0xEA; // NOP

var cpu = new Cpu(Model.Nmos);
cpu.JumpTo(0x8000);

for (var i = 0; i < 1000; ++i)
{
    if (cpu.IsWrite)
    {
        memory[cpu.Address] = cpu.Data;
        cpu.Tick();
    }
    else
    {
        cpu.Tick(memory[cpu.Address]);
    }
}
```

### Java

```java
import qe6502.Cpu;
import qe6502.Model;

public final class Example {
    public static void main(String[] args) {
        byte[] memory = new byte[65536];
        memory[0x8000] = (byte)0xEA; // NOP

        try (Cpu cpu = new Cpu(Model.NMOS)) {
            cpu.jumpTo(0x8000);

            for (int i = 0; i < 1000; ++i) {
                int address = cpu.address();

                if (cpu.isWrite()) {
                    memory[address] = (byte)cpu.data();
                    cpu.tick();
                } else {
                    cpu.tick(memory[address] & 0xff);
                }
            }
        }
    }
}
```

### JavaScript / WebAssembly

```js
import { loadQe6502Node, Model } from "./qe6502.js";

const qe = await loadQe6502Node();
const cpu = qe.createCpu(Model.nmos);
const memory = new Uint8Array(65536);
memory[0x8000] = 0xEA; // NOP

try {
  cpu.jumpTo(0x8000);

  for (let i = 0; i < 1000; ++i) {
    if (cpu.isWrite()) {
      memory[cpu.busAddress()] = cpu.busData();
      cpu.tick();
    } else {
      cpu.tick(memory[cpu.busAddress()]);
    }
  }
} finally {
  cpu.dispose();
}
```

## Save/load snapshots

Snapshots are fixed 64-byte CPU-state images. They include the current internal bus-cycle phase and the last externally visible bus request, so a restored CPU resumes deterministically instead of only restoring registers.

```cpp
qe6502::cpu cpu(qe6502::model::nmos);
cpu.restart();

qe6502::cpu_snapshot snapshot = cpu.save();

// ... later ...

qe6502::cpu restored(snapshot);
```

The same snapshot format is exposed through the native C API, stable ABI API, C++ wrapper, and the higher-level bindings.

## Interrupt pins

IRQ and NMI are explicit input pins. Drive them the same way an emulated machine would drive the real processor lines: assert the pin while the device holds it active, then deassert it when the device releases it.

```c
qe6502_irq_assert(&cpu, 1); /* assert IRQ */
qe6502_irq_assert(&cpu, 0); /* deassert IRQ */

qe6502_nmi_assert(&cpu, 1); /* assert NMI */
qe6502_nmi_assert(&cpu, 0); /* deassert NMI */
```

IRQ is level-sensitive. NMI is edge-sensitive. The exact tick on which the pin changes can matter, especially in NMOS interrupt corner cases.

## Accuracy and tests

The repository includes harnesses for:

- [Klaus2m5 functional ROM tests](https://github.com/Klaus2m5/6502_65C02_functional_tests), used as functional validation for supported 6502/65C02 models;
- [SingleStepTests / ProcessorTests](https://github.com/SingleStepTests/ProcessorTests), used for cycle-by-cycle instruction tests and final CPU-state checks;
- [`perfect6502`](https://github.com/mist64/perfect6502) lockstep checks for NMOS bus timing and interrupt behavior;
- save/load replay and ABI-surface checks;
- smoke/functional tests for the C++, C#, Java, Python, Rust, and JavaScript/WASM bindings.

For stable NMOS instructions, including illegal opcodes with stable behavior, `qe6502` is intended to match externally visible cycle-by-cycle bus behavior. The unstable illegal opcodes below intentionally follow the SingleStepTests data rather than `perfect6502` final register behavior:

| Opcode | Mnemonic |
| ---: | --- |
| `0x0B` | `ANC #imm` |
| `0x2B` | `ANC #imm` |
| `0x4B` | `ALR #imm` |
| `0x6B` | `ARR #imm` |
| `0x8B` | `XAA #imm` |
| `0xAB` | `LXA #imm` |
| `0xBB` | `LAS abs,Y` |

Maintainer-style native test run:

```sh
cmake --preset release_native
cmake --build --preset release_native
ctest --test-dir build/release_native --output-on-failure
```

WebAssembly/JavaScript test run, when the wasm toolchain is available:

```sh
cmake --preset release_wasm
cmake --build --preset release_wasm
ctest --test-dir build/release_wasm --output-on-failure
```

## Packages and bindings

GitHub Releases may include prebuilt native packages and binding-specific packages. For the most current install details, use the release notes and the binding README files:

- [C#](binds/csharp/README.md)
- [Java](binds/java/README.md)
- [Python](binds/python/README.md)
- [Rust](binds/rust/)
- [JavaScript/WebAssembly](binds/js/README.md)

## Related project: qe6502-benchmark

[`qe6502-benchmark`](https://github.com/nnqe/qe6502-benchmark) is a companion comparison of 6502-family CPU cores across correctness, cycle timing, host-visible bus behavior, model coverage, performance, portability, and integration trade-offs.

It is not required to use `qe6502`, but it gives useful background on the design choices made here.

## Status

`qe6502` 1.0.0 is the first public 1.x release line. ABI-backed bindings are expected to accept a runtime with the same major version and a minor version greater than or equal to the version they were compiled against.

## License

`qe6502` is distributed under the MIT License. See [LICENSE](LICENSE) for details.
