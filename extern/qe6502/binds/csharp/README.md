# Qe6502

.NET binding for `qe6502`, a cycle-oriented 6502 CPU emulator.

The NuGet package includes the managed C# wrapper and bundled native `qe6502` runtime libraries for the supported platforms. A normal .NET consumer project should not need to build or copy the native library manually.

## Official 1.0 release package

The `qe6502` 1.0.0 GitHub Release provides `qe6502-csharp-1.0.0-nupkg.zip`, which contains the multi-RID `Qe6502.1.0.0.nupkg`. The release workflow uploads this package as a GitHub Release asset; it does not automatically publish it to NuGet.org.

Until the package is published to NuGet.org, unzip the release asset and install from the local `.nupkg` path:

```sh
dotnet add package Qe6502 --version 1.0.0 --source /path/to/unzipped/nupkg-directory
```

After a NuGet.org publication is announced, the normal registry install command is:

```sh
dotnet add package Qe6502 --version 1.0.0
```

## Minimal use

```csharp
using Qe6502;

var cpu = new Cpu(Model.Nmos);
var memory = new byte[65536];

cpu.JumpTo(0x8000);

for (int i = 0; i < 1_000_000; ++i) {
    if (cpu.IsWrite) {
        memory[cpu.Address] = cpu.Data;
        cpu.Tick();
    } else {
        cpu.Tick(memory[cpu.Address]);
    }
}
```

`qe6502` is bus-cycle oriented: the caller drives each CPU cycle by observing the current bus request and passing the byte read from memory back into `Tick`.

## Supported CPU models

```csharp
var cpu = new Cpu(Model.Nmos);
```

Available models:

- `Model.Nmos` — NMOS 6502
- `Model.Nes` — NES / RP2A03 CPU
- `Model.Wdc` — WDC 65C02
- `Model.Rockwell` — Rockwell 65C02
- `Model.St` — Synertek 65C02

The model can also be changed through the `Model` property.

## Bus state

After `JumpTo`, `Restart`, `Load`, or `Tick`, the wrapper exposes the last native CPU tick through these properties:

```csharp
cpu.Address;
cpu.Data;
cpu.IsWrite;
cpu.IsOpcodeFetch;
cpu.IsInternalReset;
cpu.IsJammed;
cpu.RawTick;
```

A read cycle is handled by passing the memory byte to `Tick`:

```csharp
cpu.Tick(memory[cpu.Address]);
```

A write cycle is handled by storing `cpu.Data`, then ticking:

```csharp
memory[cpu.Address] = cpu.Data;
cpu.Tick();
```

## Registers and flags

The wrapper exposes CPU registers:

```csharp
cpu.PC;
cpu.S;
cpu.A;
cpu.X;
cpu.Y;
cpu.P;
```

It also exposes individual status flags:

```csharp
cpu.CarryFlag;
cpu.ZeroFlag;
cpu.InterruptDisableFlag;
cpu.DecimalFlag;
cpu.BreakFlag;
cpu.UnusedFlag;
cpu.OverflowFlag;
cpu.NegativeFlag;
```

## Interrupt inputs

Use `NmiAsserted` and `IrqAsserted` to control the logical interrupt input state:

```csharp
cpu.NmiAsserted = true;
cpu.Tick(memory[cpu.Address]);

cpu.NmiAsserted = false;
```

```csharp
cpu.IrqAsserted = true;
cpu.Tick(memory[cpu.Address]);

cpu.IrqAsserted = false;
```

## Save and load

`Save` returns a portable 64-byte CPU snapshot, including the last tick:

```csharp
byte[] snapshot = cpu.Save();
```

Restore it with:

```csharp
cpu.Load(snapshot);
```

## Bundled native runtimes

The NuGet package bundles native runtime libraries using standard NuGet runtime asset paths.

Supported runtime identifiers:

- `linux-x64`
- `linux-arm64`
- `osx-x64`
- `osx-arm64`
- `win-x64`
- `win-arm64`

Native library names:

- Linux: `libqe6502.so`
- macOS: `libqe6502.dylib`
- Windows: `libqe6502.dll`

## ABI compatibility

The C# binding uses the stable native qe6502 ABI exported by `libqe6502`.

At runtime, the binding checks the native ABI version. It accepts the same ABI major version with a runtime minor version greater than or equal to the version it was compiled against.

## Build from source

From the repository root, when the .NET SDK is available:

```sh
cmake -S . -B build
cmake --build build --target qe6502_csharp
```

The CMake target builds the managed assembly and copies the native shared library next to the C# build output.

## Package validation

To create a current-platform NuGet package from a local build:

```sh
cmake --build build --target qe6502_csharp_pack
```

To validate that package end-to-end with an external .NET console app:

```sh
cmake --build build --target qe6502_csharp_package_smoke
```

GitHub CI also builds and validates a multi-RID NuGet package artifact containing all supported native runtime assets. The release workflow uploads the assembled package to GitHub Releases and does not automatically publish it to NuGet.org.

## Tests

When `QE6502_BUILD_TESTS` is enabled, CMake builds and registers C# smoke and Klaus2m5 harness tests:

```sh
cmake -S . -B build -DQE6502_BUILD_TESTS=ON
cmake --build build --target qe6502_cs_smoke
ctest --test-dir build -R qe6502_cs_smoke --output-on-failure
```

```sh
cmake --build build --target qe6502_cs_klaus2m5
ctest --test-dir build -R qe6502.cs.klaus2m5 --output-on-failure
```

## Repository

https://github.com/nnqe/qe6502

## License

MIT
