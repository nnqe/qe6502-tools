#include <qe6502/cpu.hpp>

#include <cstdint>
#include <stdexcept>

namespace {

int expect_u8(std::uint8_t actual, std::uint8_t expected)
{
    return actual == expected ? 0 : 1;
}

int expect_u16(std::uint16_t actual, std::uint16_t expected)
{
    return actual == expected ? 0 : 1;
}

int expect_bool(bool actual, bool expected)
{
    return actual == expected ? 0 : 1;
}

int expect_tick(const qe6502_tick_t& actual, const qe6502_tick_t& expected)
{
    int failures = 0;

    failures += expect_u16(actual.address, expected.address);
    failures += expect_u8(actual.bus, expected.bus);
    failures += expect_u8(actual.status, expected.status);

    return failures;
}

int test_save_load_mid_instruction()
{
    int failures = 0;
    qe6502::cpu cpu(qe6502::model::nmos);

    cpu.jump_to(0x8000u);
    failures += expect_u16(cpu.bus_address(), 0x8000u);
    failures += expect_bool(cpu.is_opcode_fetch(), true);

    const qe6502_tick_t operand_tick = cpu.tick(0xa2u); // LDX #imm
    failures += expect_u16(cpu.bus_address(), 0x8001u);
    failures += expect_bool(cpu.is_write(), false);

    const qe6502::cpu_snapshot snapshot = cpu.save();
    failures += expect_bool(snapshot.size() == QE6502_SNAPSHOT_SIZE, true);

    const qe6502_tick_t next_tick = cpu.tick(0x11u);
    failures += expect_u8(cpu.x(), 0x11u);
    failures += expect_u16(cpu.pc(), 0x8002u);
    failures += expect_bool(cpu.is_opcode_fetch(), true);

    qe6502::cpu restored_cpu(snapshot);
    failures += expect_tick(restored_cpu.raw_tick(), operand_tick);
    failures += expect_u16(restored_cpu.bus_address(), 0x8001u);
    failures += expect_u8(restored_cpu.x(), 0x00u);
    failures += expect_u16(restored_cpu.pc(), 0x8002u);
    failures += expect_bool(restored_cpu.is_opcode_fetch(), false);

    const qe6502_tick_t& restored_tick = cpu.load(snapshot);
    failures += expect_tick(restored_tick, operand_tick);
    failures += expect_u16(cpu.bus_address(), 0x8001u);
    failures += expect_u8(cpu.x(), 0x00u);
    failures += expect_u16(cpu.pc(), 0x8002u);
    failures += expect_bool(cpu.is_opcode_fetch(), false);

    const qe6502_tick_t replay_tick = cpu.tick(0x22u);
    failures += expect_tick(replay_tick, next_tick);
    failures += expect_u8(cpu.x(), 0x22u);
    failures += expect_u16(cpu.pc(), 0x8002u);
    failures += expect_bool(cpu.is_opcode_fetch(), true);

    return failures;
}

int test_save_load_public_registers_and_model()
{
    int failures = 0;
    qe6502::cpu cpu(qe6502::model::wdc);

    cpu.jump_to(0x1234u);
    cpu.s(0x56u);
    cpu.a(0x78u);
    cpu.x(0x9au);
    cpu.y(0xbcu);
    cpu.p(0xdeu);

    const qe6502::cpu_snapshot snapshot = cpu.save();

    cpu.jump_to(0u);
    cpu.s(0u);
    cpu.a(0u);
    cpu.x(0u);
    cpu.y(0u);
    cpu.p(0u);

    cpu.load(snapshot);

    failures += expect_bool(cpu.cpu_model() == qe6502::model::wdc, true);
    failures += expect_u16(cpu.pc(), 0x1234u);
    failures += expect_u8(cpu.s(), 0x56u);
    failures += expect_u8(cpu.a(), 0x78u);
    failures += expect_u8(cpu.x(), 0x9au);
    failures += expect_u8(cpu.y(), 0xbcu);
    failures += expect_u8(cpu.p(), 0xdeu);

    return failures;
}

int test_load_rejects_invalid_snapshot_size()
{
    int failures = 0;
    qe6502::cpu cpu(qe6502::model::nmos);
    qe6502::cpu_snapshot invalid(QE6502_SNAPSHOT_SIZE - 1u);

    bool threw = false;
    try {
        static_cast<void>(cpu.load(invalid));
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    failures += expect_bool(threw, true);
    return failures;
}

} // namespace

int main()
{
    int failures = 0;

    failures += test_save_load_mid_instruction();
    failures += test_save_load_public_registers_and_model();
    failures += test_load_rejects_invalid_snapshot_size();

    return failures == 0 ? 0 : 1;
}
