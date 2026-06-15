#include "common.hpp"

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <sstream>

namespace cpu6502_lockstep {

std::string hex8(std::uint8_t value)
{
    std::ostringstream out;
    out << '$' << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<unsigned>(value);
    return out.str();
}

std::string hex16(std::uint16_t value)
{
    std::ostringstream out;
    out << '$' << std::uppercase << std::hex << std::setw(4) << std::setfill('0')
        << static_cast<unsigned>(value);
    return out.str();
}

namespace {

void print_entry(std::ostream& out,
                 const tools6502::CpuTraceEntry& entry)
{
    out << "cycle=" << std::dec << entry.cycle
        << ' ' << (entry.opcode_fetch ? "FETCH" : "     ")
        << ' ' << (entry.write ? 'W' : 'R')
        << " addr=" << hex16(entry.address)
        << " data=" << hex8(entry.data)
        << " irq=" << (entry.irq_asserted ? '1' : '0')
        << " nmi=" << (entry.nmi_asserted ? '1' : '0');
}

} // namespace

void print_failure_trace(std::ostream& out,
                         const tools6502::LockstepRunResult& result,
                         std::size_t context_before,
                         std::size_t context_after)
{
    if (!result.first_mismatch) {
        out << "no trace mismatch index available\n";
        return;
    }

    const std::size_t mismatch = *result.first_mismatch;
    const std::size_t trace_size = std::min(result.left_trace.size(), result.right_trace.size());
    const std::size_t begin = mismatch > context_before ? mismatch - context_before : 0u;
    const std::size_t end = std::min(trace_size, mismatch + context_after + 1u);

    for (std::size_t index = begin; index < end; ++index) {
        out << (index == mismatch ? "  -> " : "     ") << "left  ";
        print_entry(out, result.left_trace[index]);
        out << '\n';
        out << (index == mismatch ? "  -> " : "     ") << "right ";
        print_entry(out, result.right_trace[index]);
        out << '\n';
    }
}

std::vector<tools6502::LockstepCommand> make_pulse_script(bool nmi,
                                                          std::size_t pulse_start_cycle,
                                                          std::size_t pulse_length,
                                                          std::size_t total_steps)
{
    std::vector<tools6502::LockstepCommand> script;
    if (pulse_start_cycle > 0u) {
        script.push_back(tools6502::StepCycles{pulse_start_cycle});
    }

    script.push_back(nmi
        ? tools6502::LockstepCommand{tools6502::NmiAssert{}}
        : tools6502::LockstepCommand{tools6502::IrqAssert{}});

    if (pulse_length > 0u) {
        script.push_back(tools6502::StepCycles{pulse_length});
    }

    script.push_back(nmi
        ? tools6502::LockstepCommand{tools6502::NmiDeassert{}}
        : tools6502::LockstepCommand{tools6502::IrqDeassert{}});

    const std::size_t used = pulse_start_cycle + pulse_length;
    if (used < total_steps) {
        script.push_back(tools6502::StepCycles{total_steps - used});
    }

    return script;
}

} // namespace cpu6502_lockstep
