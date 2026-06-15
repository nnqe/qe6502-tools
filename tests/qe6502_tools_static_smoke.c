#include <stdint.h>
#include <qe6502/qe6502.h>

static uint8_t memory[65536];

static uint8_t memory_read(uint16_t address)
{
    return memory[address];
}

static void memory_write(uint16_t address, uint8_t value)
{
    memory[address] = value;
}

int main(void)
{
    memory[0x8000] = 0xEA; /* NOP */
    memory[0xFFFC] = 0x00; /* reset vector low */
    memory[0xFFFD] = 0x80; /* reset vector high */

    qe6502_t cpu = qe6502_setup(qe6502_model_nmos);
    qe6502_tick_t tick = qe6502_restart(&cpu);

    for (int i = 0; i < 1000; ++i) {
        uint8_t input = 0;

        if (qe6502_is_write(tick)) {
            memory_write(tick.address, tick.bus);
        } else {
            input = memory_read(tick.address);
        }

        tick = qe6502_tick(&cpu, input);
    }

    return 0;
}
