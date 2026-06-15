# qe6502 for Python

`qe6502` is a lightweight bus-cycle 6502 CPU emulator exposed as a CPython extension module.

The Python package wraps the stable qe6502 C ABI and provides a small stateful `CPU` object for stepping the emulator one bus cycle at a time.

## Official 1.0 release package

The `qe6502` 1.0.0 GitHub Release provides `qe6502-python-1.0.0-wheels.zip`, which contains platform wheels and the `qe6502-1.0.0.tar.gz` source distribution. The release workflow uploads these files as GitHub Release assets; it does not automatically publish them to PyPI.

Until the package is published to PyPI, unzip the release asset and install the wheel matching your platform:

```sh
python -m pip install /path/to/qe6502-1.0.0-*.whl
```

Source builds are available from the included source distribution and may require a C compiler, CMake, and Python development headers:

```sh
python -m pip install /path/to/qe6502-1.0.0.tar.gz
```

After a PyPI publication is announced, the normal registry install command is:

```sh
python -m pip install qe6502==1.0.0
```

Most users should use a prebuilt wheel. The package is a native CPython extension targeting Python 3.10 and newer through CPython's stable ABI.

## Quick start

```python
import qe6502

cpu = qe6502.CPU(qe6502.MODEL_NMOS)
memory = bytearray(65536)

# INC $0200; JMP $8000
memory[0x8000:0x8006] = bytes([0xEE, 0x00, 0x02, 0x4C, 0x00, 0x80])

bus = cpu.jump_to(0x8000)

for _ in range(64):
    address = bus & qe6502.TICK_ADDRESS_MASK
    if bus & qe6502.TICK_WRITING:
        memory[address] = bus >> qe6502.TICK_BUS_SHIFT
        bus = cpu.tick()
    else:
        bus = cpu.tick(memory[address])

assert memory[0x0200] != 0
```

## CPU models

The binding exposes the same model constants as the C ABI, including:

- `MODEL_NMOS`
- `MODEL_NES`
- `MODEL_WDC`
- `MODEL_ROCKWELL`
- `MODEL_SYNERTEK`

## Bus state

`CPU.tick()` returns the packed qe6502 bus state. Use the exported masks and shifts to inspect it:

- `TICK_ADDRESS_MASK`
- `TICK_BUS_SHIFT`
- `TICK_WRITING`
- `TICK_FETCH`
- `TICK_INTERNAL_RESET`
- `TICK_CPU_JAMMED`

## Registers and flags

The `CPU` object exposes register properties such as `pc`, `a`, `x`, `y`, `s`, and `p`, plus individual flag properties such as `carry_flag`, `decimal_flag`, and `negative_flag`.

## Interrupt pins

Use `cpu.nmi_asserted` and `cpu.irq_asserted` to inspect or update the interrupt input pins.

## Snapshots

```python
snapshot = cpu.save()
restored_tick = cpu.load(snapshot)
```

Snapshots are byte strings with length `SNAPSHOT_SIZE`.

## Lifetime

`CPU` owns its emulator context. No explicit close or dispose call is required; the context is released with the Python object.

## Versioning

The Python package version is derived from the qe6502 core major/minor version as `major.minor.0`.

The extension targets CPython's stable ABI for Python 3.10 and newer.

## License

MIT
