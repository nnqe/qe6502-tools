#include <qe6502/qe6502.h>
#include <stdint.h>
#include <time.h>

static uint8_t tick_is_write(qe6502_tick_t tick)
{
    return (uint8_t)((tick.status & qe6502_status_writing) != 0u);
}

static uint8_t tick_is_opcode_fetch(qe6502_tick_t tick)
{
    return (uint8_t)((tick.status & qe6502_status_opcode_fetch) != 0u);
}

static uint8_t tick_bus_data(qe6502_tick_t tick, uint8_t* memory)
{
    if (tick_is_write(tick))
    {
        return tick.bus;
    }

    return memory[tick.address];
}

static double elapsed_seconds(clock_t start, clock_t stop)
{
    const double ticks = (double)(stop - start);
    return ticks / (double)CLOCKS_PER_SEC;
}

static double emulated_mhz(uint64_t ticks, double seconds)
{
    if (seconds <= 0.0)
    {
        return 0.0;
    }

    return ((double)ticks / seconds) / 1000000.0;
}

const char* test_klaus2m5_v2(uint8_t cpu_model,
                             uint8_t* memory,
                             uint16_t success_address,
                             uint64_t expected_cycles,
                             uint8_t* result,
                             double* mhz)
{
    qe6502_t cpu;
    qe6502_t* cpu_ptr = &cpu;
    qe6502_tick_t tick;
    uint64_t cycles = 0;
    uint64_t bus_ticks = 0;
    clock_t start;
    clock_t stop;
    const char* result_msg = "CPU Error";

    *result = 0;
    *mhz = 0.0;

    cpu = qe6502_setup(cpu_model);
    (void)qe6502_restart(cpu_ptr);
    tick = qe6502_goto(cpu_ptr, 0x0400u);

    start = clock();

    for (;;)
    {
        uint8_t data = tick_bus_data(tick, memory);

        if (tick.address == success_address)
        {
            *result = 1;
            result_msg = "OK";
            break;
        }

        if (tick_is_write(tick))
        {
            memory[tick.address] = data;
        }
        else
        {
            data = memory[tick.address];
        }

        tick = qe6502_tick(cpu_ptr, data);
        bus_ticks++;

        if (tick_is_opcode_fetch(tick))
        {
            cycles++;
            if (cycles > 2u * expected_cycles)
            {
                result_msg = "Test fail, takes too many cycles!";
                break;
            }
        }
    }

    stop = clock();
    *mhz = emulated_mhz(bus_ticks, elapsed_seconds(start, stop));

    return result_msg;
}
