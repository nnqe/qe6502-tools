#pragma once

#include <asm6502/asm6502.h>

#include <cstdint>
#include <string>
#include <vector>

namespace tools6502 {

using mem_value = asm6502::mem_value;
using memory_setup = std::vector<mem_value>;

struct cpu_state
{
    std::uint8_t A = 0x00u;
    std::uint8_t X = 0x00u;
    std::uint8_t Y = 0x00u;
    std::uint8_t P = 0x24u;
    std::uint8_t S = 0xfdu;
};

struct vector_set
{
    std::uint16_t reset = 0x0200u;
    std::uint16_t brk_irq = 0x9100u;
    std::uint16_t nmi = 0x9000u;
};

struct testcase
{
    std::uint8_t opcode = 0x00u;
    std::uint8_t bytes = 0u;
    std::uint16_t start_at = 0x0400u;
    vector_set vectors{};
    std::uint8_t A = 0x00u;
    std::uint8_t X = 0x00u;
    std::uint8_t Y = 0x00u;
    std::uint8_t P = 0x24u;
    std::uint8_t S = 0xfdu;
    memory_setup mem_setup{};
    memory_setup program{};
    std::string description{};
};

inline memory_setup make_bootstrap(const testcase& test)
{
    return asm6502::bootstrap_program(
        test.A,
        test.X,
        test.Y,
        test.P,
        test.S,
        test.start_at,
        test.vectors.reset,
        test.vectors.brk_irq,
        test.vectors.nmi);
}

} // namespace tools6502
