#pragma once

#include <lockstep.hpp>

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

namespace cpu6502_lockstep {

std::string hex8(std::uint8_t value);
std::string hex16(std::uint16_t value);

void print_failure_trace(std::ostream& out,
                         const tools6502::LockstepRunResult& result,
                         std::size_t context_before = 4u,
                         std::size_t context_after = 4u);

std::vector<tools6502::LockstepCommand> make_pulse_script(bool nmi,
                                                          std::size_t pulse_start_cycle,
                                                          std::size_t pulse_length,
                                                          std::size_t total_steps);

} // namespace cpu6502_lockstep
