#include <qe6502/cpu.hpp>

#include <cstdint>

#ifndef QE6502_EXPECT_CPP_SHARED
#error "QE6502_EXPECT_CPP_SHARED must be defined by the install/consume harness"
#endif

#if QE6502_EXPECT_CPP_SHARED
#  ifndef QE6502_CPP_SHARED
#    error "This installed C++ target must define QE6502_CPP_SHARED"
#  endif
#else
#  ifdef QE6502_CPP_SHARED
#    error "This installed C++ target must not define QE6502_CPP_SHARED"
#  endif
#endif

int main()
{
    qe6502::cpu cpu;
    cpu.restart();

    const auto& first_tick = cpu.tick(0x00);
    const std::uint16_t first_address = cpu.bus_address();
    const std::uint8_t first_data = cpu.bus_data();

    if (&first_tick != &cpu.raw_tick()) {
        return 1;
    }

    if (cpu.is_jammed()) {
        return 2;
    }

    if (first_address != cpu.bus_address()) {
        return 3;
    }

    if (first_data != cpu.bus_data()) {
        return 4;
    }

    return 0;
}
