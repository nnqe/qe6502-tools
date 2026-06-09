#include <asm6502/asm6502.h>
#include <qe6502/qe6502.h>

#include <array>
#include <cstdint>
#include <cstdio>

int main()
{
    std::array<std::uint8_t, 65536> memory{};
    memory.fill(0xeau);

    auto program = asm6502::Asm6502::New()
        .begin()
            .reset_vector("boot")
            .org(0x0400u, "boot")
                .inc(asm6502::absolute, 0x0200u)
                .jmp("boot")
        .end();

    const auto image = program.compile_to_map();
    asm6502::Asm6502::apply(image, memory.data());

    qe6502_t cpu{};
    cpu.model = qe6502_model_nmos;

    qe6502_tick_t tick = qe6502_restart(&cpu);

    for (std::uint32_t cycle = 0; cycle < 1000000u; ++cycle)
    {
        std::uint8_t data = memory[tick.address];

        if ((tick.status & qe6502_status_writing) != 0u)
        {
            memory[tick.address] = tick.bus;
            data = tick.bus;
        }

        tick = qe6502_tick(&cpu, data);
    }

    std::printf("memory[$0200] = $%02x\n", static_cast<unsigned>(memory[0x0200u]));
    return 0;
}
