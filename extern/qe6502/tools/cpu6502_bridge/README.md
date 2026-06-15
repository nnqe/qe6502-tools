# cpu6502_bridge

Small C++17 facade over the local `qe6502` core and the vendored `perfect6502`
netlist core.

Public API lives in:

```cpp
#include <cpu6502_bridge/cpu.hpp>
```

Factory functions:

```cpp
auto cpu = cpu6502_bridge::make_qe6502_cpu();
auto oracle = cpu6502_bridge::make_perfect6502_cpu();
```

Each backend exposes a 64 KiB memory buffer through `ICpu::memory()`:

```cpp
auto* ram = cpu->memory();
ram[0xfffc] = 0x00;
ram[0xfffd] = 0x04;
ram[0x0400] = 0xea;
cpu->restart();
```

`ICpu::step()` represents one external 6502 bus cycle. For the `qe6502`
backend, read cycles consume bytes from the backend's private 64 KiB memory and
write cycles store into that memory. `set_bus_data()` remains available as a
compatibility helper and patches the current read address.

For the `perfect6502` backend, `step()` reports a cached complete-cycle
snapshot. It first runs to a completed CPU half-step and samples the request
address, R/W direction, SYNC/opcode-fetch state, and registers. It then runs
the corresponding memory/bus half-step and samples the data bus. Public getters
return that cached snapshot rather than reading the live netlist phase.

The `perfect6502` adapter uses the upstream global `memory[65536]` storage, so
multiple concurrent perfect6502 instances are intentionally not supported as
independent machines.

## Fetch comparison smoke test

The optional `cpu6502_bridge_fetch_compare` executable runs the same short
program through the `qe6502` and `perfect6502` backends, logs each opcode fetch
with the external bus cycle number and bus state, and compares normalized fetch
cycles after the first real program fetch.

```sh
cmake --build <build-dir> --target cpu6502_bridge_fetch_compare
<build-dir>/cpu6502_bridge/cpu6502_bridge_fetch_compare
```

The raw restart phase is intentionally reported. `perfect6502` currently exposes
one pre-program `SYNC`/fetch-like state, and the two backends do not start with
the same absolute wrapper-step counter. After normalizing to the first `$0400`
opcode fetch, the test compares the program fetch bus sequence.
