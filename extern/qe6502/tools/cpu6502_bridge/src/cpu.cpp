#include <cpu6502_bridge/cpu.hpp>
#include <cstring>
#include <cstdio>

namespace cpu6502_bridge {

void ICpu::clear_memory(uint8_t value) noexcept
{
    uint8_t* mem = memory();
    std::memset(mem, value, 65536);
}

unsigned ICpu::restart_to_start_fetch(
    unsigned max_steps)
{
    const auto* mem = memory();
    const std::uint16_t start_address =
        static_cast<std::uint16_t>(mem[0xfffc])
        | static_cast<std::uint16_t>(mem[0xfffd] << 8);

    restart();

    for (unsigned steps = 0; steps < max_steps; ++steps) {
        if (is_opcode_fetch()
            && !is_write()
            && bus_address() == start_address) {
            return steps;
        }

        step();
    }

    return max_steps;
}

std::string ICpu::to_string() const
{
    char buffer[160];
    const char rw = is_write() ? 'W' : 'R';
    const int count = std::snprintf(buffer,
                                    sizeof(buffer),
                                    "PC=%04X A=%02X X=%02X Y=%02X S=%02X P=%02X BUS=%c %04X DATA=%02X%s",
                                    static_cast<unsigned>(pc()),
                                    static_cast<unsigned>(a()),
                                    static_cast<unsigned>(x()),
                                    static_cast<unsigned>(y()),
                                    static_cast<unsigned>(s()),
                                    static_cast<unsigned>(p()),
                                    rw,
                                    static_cast<unsigned>(bus_address()),
                                    static_cast<unsigned>(bus_data()),
                                    is_opcode_fetch() ? " FETCH" : "");

    if (count <= 0) {
        return {};
    }

    const auto length = static_cast<std::size_t>(count);
    if (length < sizeof(buffer)) {
        return std::string(buffer, length);
    }

    return std::string(buffer);
}

} // namespace cpu6502_bridge
