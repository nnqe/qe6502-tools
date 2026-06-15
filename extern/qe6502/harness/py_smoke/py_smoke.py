import qe6502

ADDR = qe6502.TICK_ADDRESS_MASK
BUS_SHIFT = qe6502.TICK_BUS_SHIFT
WRITING = qe6502.TICK_WRITING


def run_bus(cpu, memory, bus_state, cycles):
    for _ in range(cycles):
        address = bus_state & ADDR

        if bus_state & WRITING:
            memory[address] = bus_state >> BUS_SHIFT
            bus_state = cpu.tick()
        else:
            bus_state = cpu.tick(memory[address])

    return bus_state


def main():
    if qe6502.version() != qe6502.ABI_VERSION:
        raise AssertionError("unexpected ABI version")

    cpu = qe6502.CPU(qe6502.MODEL_NMOS)
    memory = bytearray(65536)

    # INC $0200; JMP $8000
    memory[0x8000:0x8006] = bytes([0xEE, 0x00, 0x02, 0x4C, 0x00, 0x80])

    bus_state = cpu.jump_to(0x8000)
    bus_state = run_bus(cpu, memory, bus_state, 64)

    if memory[0x0200] == 0:
        raise AssertionError("program did not update memory")

    cpu.a = 0x42
    cpu.x = 0x11
    cpu.y = 0x22
    cpu.s = 0xFD
    cpu.p = 0x24
    cpu.carry_flag = True
    cpu.decimal_flag = True
    cpu.negative_flag = False
    cpu.nmi_asserted = True
    cpu.irq_asserted = True

    if cpu.a != 0x42 or cpu.x != 0x11 or cpu.y != 0x22 or cpu.s != 0xFD:
        raise AssertionError("register property round trip failed")

    if not cpu.carry_flag or not cpu.decimal_flag or cpu.negative_flag:
        raise AssertionError("flag property round trip failed")

    if not cpu.nmi_asserted or not cpu.irq_asserted:
        raise AssertionError("interrupt property round trip failed")

    snapshot = cpu.save()
    if len(snapshot) != qe6502.SNAPSHOT_SIZE:
        raise AssertionError("unexpected snapshot size")

    restored = qe6502.CPU(qe6502.MODEL_NES)
    restored_tick = restored.load(snapshot)

    if restored_tick != cpu.raw_tick:
        raise AssertionError("load did not restore cached tick")

    if restored.a != cpu.a or restored.x != cpu.x or restored.y != cpu.y or restored.s != cpu.s or restored.p != cpu.p:
        raise AssertionError("load did not restore registers")

    print("qe6502 Python smoke OK")


if __name__ == "__main__":
    main()
