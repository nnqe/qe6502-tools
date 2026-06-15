from __future__ import annotations

import re
import sys
import time
from pathlib import Path

import qe6502

STANDARD_ROM = "6502_functional_test.hex"
EXTENDED_ROM = "65C02_extended_opcodes_test.hex"

STANDARD_SUCCESS_ADDRESS = 0x3469
STANDARD_EXPECTED_CYCLES = 30_646_176
EXTENDED_SUCCESS_ADDRESS = 0x24F1
EXTENDED_EXPECTED_CYCLES = 21_986_985

START_ADDRESS = 0x0400
MEMORY_SIZE = 0x10000

ADDR = qe6502.TICK_ADDRESS_MASK
BUS_SHIFT = qe6502.TICK_BUS_SHIFT
WRITING = qe6502.TICK_WRITING
FETCH = qe6502.TICK_FETCH

MODEL_INFO = {
    "nmos": (qe6502.MODEL_NMOS, "NMOS 6502"),
    "mos": (qe6502.MODEL_NMOS, "NMOS 6502"),
    "wdc": (qe6502.MODEL_WDC, "WDC 65C02"),
    "rw": (qe6502.MODEL_ROCKWELL, "Rockwell 65C02"),
    "rockwell": (qe6502.MODEL_ROCKWELL, "Rockwell 65C02"),
    "st": (qe6502.MODEL_SYNERTEK, "Synertek 65C02"),
    "synertek": (qe6502.MODEL_SYNERTEK, "Synertek 65C02"),
}

DEFAULT_SUITE = (
    ("nmos", "standard"),
    ("wdc", "standard"),
    ("wdc", "extended"),
    ("rw", "standard"),
    ("rw", "extended"),
    ("st", "standard"),
)


def usage(script_name: str) -> str:
    return (
        f"Usage: python {script_name} <rom-dir> [<model> <test>]\n"
        f"       python {script_name}\n\n"
        "With no model/test arguments, runs the default v2 Klaus suite.\n\n"
        "Models: nmos, wdc, rw, st\n"
        "Tests: standard, extended\n"
    )


def load_hex_rom(path: Path) -> bytearray:
    text = path.read_text(encoding="utf-8")
    matches = re.findall(r"0x[0-9a-fA-F]+", text)

    if len(matches) != MEMORY_SIZE:
        raise RuntimeError(f"{path} contains {len(matches)} bytes; expected {MEMORY_SIZE}")

    return bytearray(int(value, 16) & 0xFF for value in matches)


def parse_model(model_name: str) -> tuple[int, str]:
    try:
        return MODEL_INFO[model_name]
    except KeyError as exc:
        raise RuntimeError(f"Unknown model: {model_name}") from exc


def parse_test(test_name: str, model: int, display_name: str) -> None:
    if test_name not in ("standard", "extended"):
        raise RuntimeError(f"Unknown test: {test_name}")

    if test_name == "extended" and model not in (qe6502.MODEL_WDC, qe6502.MODEL_ROCKWELL):
        raise RuntimeError(
            f"Extended test is only valid for WDC/Rockwell 65C02 v2 models, not {display_name}."
        )


def test_config(test_name: str, roms: dict[str, bytearray]) -> tuple[bytearray, int, int]:
    if test_name == "standard":
        return bytearray(roms["standard"]), STANDARD_SUCCESS_ADDRESS, STANDARD_EXPECTED_CYCLES

    return bytearray(roms["extended"]), EXTENDED_SUCCESS_ADDRESS, EXTENDED_EXPECTED_CYCLES


def emulated_mhz(bus_ticks: int, seconds: float) -> float:
    if seconds <= 0.0:
        return 0.0

    return bus_ticks / seconds / 1_000_000.0


def run_klaus_test(roms: dict[str, bytearray], model_name: str, test_name: str) -> bool:
    model, display_name = parse_model(model_name)
    parse_test(test_name, model, display_name)

    memory, success_address, expected_cycles = test_config(test_name, roms)
    cpu = qe6502.CPU(model)

    passed = False
    result_message = "CPU Error"
    opcode_cycles = 0
    bus_ticks = 0
    mhz = 0.0

    cpu.restart()
    bus_state = cpu.jump_to(START_ADDRESS)
    start = time.perf_counter()

    while True:
        address = bus_state & ADDR
        data = bus_state >> BUS_SHIFT if bus_state & WRITING else memory[address]

        if address == success_address:
            passed = True
            result_message = "OK"
            break

        if bus_state & WRITING:
            memory[address] = data

        bus_state = cpu.tick(data)
        bus_ticks += 1

        if bus_state & FETCH:
            opcode_cycles += 1
            if opcode_cycles > 2 * expected_cycles:
                result_message = "Test fail, takes too many cycles!"
                break

    mhz = emulated_mhz(bus_ticks, time.perf_counter() - start)

    print(
        f"{display_name} CPU {test_name} Python test "
        f"{'[PASS]' if passed else '[FAIL]'} {result_message} "
        f"({mhz:.2f} MHz, {bus_ticks} bus ticks, {opcode_cycles} opcode cycles)"
    )

    return passed


def parse_command_line(argv: list[str]) -> tuple[Path, tuple[tuple[str, str], ...]]:
    script_name = argv[0] if argv else "py_klaus2m5.py"
    args = argv[1:]

    default_rom_dir = Path(__file__).resolve().parent / "roms"

    if not args:
        return default_rom_dir, DEFAULT_SUITE

    if len(args) == 1:
        return Path(args[0]), DEFAULT_SUITE

    if len(args) == 3:
        return Path(args[0]), ((args[1], args[2]),)

    raise RuntimeError(usage(script_name))


def main(argv: list[str]) -> int:
    if qe6502.version() != qe6502.ABI_VERSION:
        raise RuntimeError("unexpected ABI version")

    rom_dir, suite = parse_command_line(argv)
    roms = {
        "standard": load_hex_rom(rom_dir / STANDARD_ROM),
        "extended": load_hex_rom(rom_dir / EXTENDED_ROM),
    }

    failures = 0
    for model_name, test_name in suite:
        if not run_klaus_test(roms, model_name, test_name):
            failures += 1

    return 1 if failures else 0


if __name__ == "__main__":
    try:
        raise SystemExit(main(sys.argv))
    except Exception as exc:
        print(exc, file=sys.stderr)
        raise SystemExit(1)
