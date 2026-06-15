#include <qe6502/qe6502_abi.h>

static uint32_t expect_u32(uint32_t actual, uint32_t expected)
{
    return actual == expected ? 0u : 1u;
}

int main(void)
{
    qe6502abi_context_t cpu;
    uint8_t snapshot[QE6502_ABI_SNAPSHOT_SIZE];
    qe6502abi_tick_t tick;
    uint32_t failures = 0u;

    qe6502abi_setup(&cpu, QE6502_ABI_MODEL_WDC);
    tick = qe6502abi_restart(&cpu);
    qe6502abi_set_pc(&cpu, 0x1234u);
    qe6502abi_set_s(&cpu, 0x56u);
    qe6502abi_set_a(&cpu, 0x78u);
    qe6502abi_set_x(&cpu, 0x9au);
    qe6502abi_set_y(&cpu, 0xbcu);
    qe6502abi_set_p(&cpu, 0xdeu);

    qe6502abi_save(&cpu, tick, snapshot);

    failures += expect_u32(snapshot[0], 0x65u);
    failures += expect_u32(snapshot[1], 0x02u);
    failures += expect_u32(snapshot[2], 0x00u);
    failures += expect_u32(snapshot[3], 0x01u);
    failures += expect_u32(snapshot[4], QE6502_ABI_MODEL_WDC);
    failures += expect_u32(snapshot[12], 0x12u);
    failures += expect_u32(snapshot[13], 0x34u);
    failures += expect_u32(snapshot[14], 0x56u);
    failures += expect_u32(snapshot[15], 0x78u);
    failures += expect_u32(snapshot[16], 0x9au);
    failures += expect_u32(snapshot[17], 0xbcu);
    failures += expect_u32(snapshot[18], 0xdeu);
    failures += expect_u32(snapshot[20], (tick >> 24u) & 0xffu);
    failures += expect_u32(snapshot[21], (tick >> 16u) & 0xffu);
    failures += expect_u32(snapshot[22], (tick >> 8u) & 0xffu);
    failures += expect_u32(snapshot[23], tick & 0xffu);

    qe6502abi_setup(&cpu, QE6502_ABI_MODEL_NMOS);
    qe6502abi_set_pc(&cpu, 0u);
    qe6502abi_set_s(&cpu, 0u);
    qe6502abi_set_a(&cpu, 0u);
    qe6502abi_set_x(&cpu, 0u);
    qe6502abi_set_y(&cpu, 0u);
    qe6502abi_set_p(&cpu, 0u);

    failures += expect_u32(qe6502abi_load(&cpu, snapshot), tick);
    failures += expect_u32(qe6502abi_get_model(&cpu), QE6502_ABI_MODEL_WDC);
    failures += expect_u32(qe6502abi_get_pc(&cpu), 0x1234u);
    failures += expect_u32(qe6502abi_get_s(&cpu), 0x56u);
    failures += expect_u32(qe6502abi_get_a(&cpu), 0x78u);
    failures += expect_u32(qe6502abi_get_x(&cpu), 0x9au);
    failures += expect_u32(qe6502abi_get_y(&cpu), 0xbcu);
    failures += expect_u32(qe6502abi_get_p(&cpu), 0xdeu);

    return failures == 0u ? 0 : 1;
}
