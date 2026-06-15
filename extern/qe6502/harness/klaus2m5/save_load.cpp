#include <qe6502/cpu.hpp>
#include <qe6502/qe6502.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

extern "C" void copy_klaus2m5_extended_image(std::uint8_t* dst,
                                               std::uint16_t* success_address,
                                               std::uint64_t* expected_cycles);

namespace {

constexpr std::uint16_t klaus_start_address = 0x0400u;
constexpr std::uint64_t klaus_checkpoint_interval = 256u;
constexpr std::uint16_t nes_start_address = 0x8000u;
constexpr std::uint16_t nes_success_address = 0x80ffu;
constexpr std::uint64_t nes_checkpoint_interval = 3u;
constexpr std::uint64_t nes_max_opcode_cycles = 200u;

using memory_t = std::array<std::uint8_t, 0x10000u>;

struct run_result {
    bool pass = false;
    const char* message = "CPU Error";
    std::uint64_t bus_ticks = 0;
    std::uint64_t opcode_cycles = 0;
    qe6502::cpu_snapshot state{};
    memory_t memory{};
};

bool tick_is_write(const qe6502_tick_t& tick) noexcept
{
    return (tick.status & qe6502_status_writing) != 0u;
}

bool tick_is_opcode_fetch(const qe6502_tick_t& tick) noexcept
{
    return (tick.status & qe6502_status_opcode_fetch) != 0u;
}

std::uint8_t tick_bus_data(const qe6502_tick_t& tick, const memory_t& memory) noexcept
{
    return tick_is_write(tick) ? tick.bus : memory[tick.address];
}

void poison_cpu(qe6502::cpu& cpu) noexcept
{
    std::memset(&cpu.raw_cpu(), 0xa5, sizeof(cpu.raw_cpu()));
    std::memset(&cpu.raw_tick(), 0xa5, sizeof(cpu.raw_tick()));
}

int compare_results(const run_result& expected, const run_result& actual)
{
    int failures = 0;

    if (!expected.pass) {
        std::fprintf(stderr, "reference run failed: %s\n", expected.message);
        ++failures;
    }

    if (!actual.pass) {
        std::fprintf(stderr, "checkpointed run failed: %s\n", actual.message);
        ++failures;
    }

    if (expected.state != actual.state) {
        std::fprintf(stderr,
                     "snapshot mismatch: expected %zu bytes, got %zu bytes\n",
                     expected.state.size(),
                     actual.state.size());

        const std::size_t count = expected.state.size() < actual.state.size()
                                      ? expected.state.size()
                                      : actual.state.size();
        for (std::size_t i = 0; i < count; ++i) {
            if (expected.state[i] != actual.state[i]) {
                std::fprintf(stderr,
                             "snapshot byte mismatch at %zu: expected 0x%02X, got 0x%02X\n",
                             i,
                             static_cast<unsigned>(expected.state[i]),
                             static_cast<unsigned>(actual.state[i]));
                break;
            }
        }

        ++failures;
    }

    if (expected.bus_ticks != actual.bus_ticks) {
        std::fprintf(stderr,
                     "bus tick count mismatch: expected %llu, got %llu\n",
                     static_cast<unsigned long long>(expected.bus_ticks),
                     static_cast<unsigned long long>(actual.bus_ticks));
        ++failures;
    }

    if (expected.opcode_cycles != actual.opcode_cycles) {
        std::fprintf(stderr,
                     "opcode cycle count mismatch: expected %llu, got %llu\n",
                     static_cast<unsigned long long>(expected.opcode_cycles),
                     static_cast<unsigned long long>(actual.opcode_cycles));
        ++failures;
    }

    for (std::size_t i = 0; i < expected.memory.size(); ++i) {
        if (expected.memory[i] != actual.memory[i]) {
            std::fprintf(stderr,
                         "memory mismatch at 0x%04X: expected 0x%02X, got 0x%02X\n",
                         static_cast<unsigned>(i),
                         static_cast<unsigned>(expected.memory[i]),
                         static_cast<unsigned>(actual.memory[i]));
            ++failures;
            break;
        }
    }

    return failures;
}

void checkpoint_reload(qe6502::cpu& cpu, qe6502_tick_t& tick)
{
    const qe6502::cpu_snapshot snapshot = cpu.save();
    poison_cpu(cpu);
    cpu.load(snapshot);
    tick = cpu.raw_tick();
}

void install_nes_functional_program(memory_t& memory)
{
    /*
     * A small NES-model functional program. SED is intentionally executed before
     * ADC; on the NES CPU, decimal arithmetic is not implemented, so 0x15 +
     * 0x27 must produce binary 0x3C rather than BCD 0x42.
     */
    static constexpr std::uint8_t program[] = {
        0xf8,                         /* SED */
        0xa9, 0x15,                   /* LDA #$15 */
        0x18,                         /* CLC */
        0x69, 0x27,                   /* ADC #$27 */
        0x8d, 0x00, 0x02,             /* STA $0200 */
        0xd8,                         /* CLD */
        0xa2, 0x05,                   /* LDX #$05 */
        0xca,                         /* loop: DEX */
        0x8e, 0x01, 0x02,             /* STX $0201 */
        0xd0, 0xfa,                   /* BNE loop */
        0x08,                         /* PHP */
        0x68,                         /* PLA */
        0x8d, 0x02, 0x02,             /* STA $0202 */
        0x4c, 0xff, 0x80              /* JMP $80FF */
    };

    for (std::size_t i = 0; i < sizeof(program); ++i) {
        memory[static_cast<std::size_t>(nes_start_address) + i] = program[i];
    }
}

run_result run_until_success(qe6502::model model,
                             std::uint16_t start_address,
                             std::uint16_t success_address,
                             std::uint64_t max_opcode_cycles,
                             std::uint64_t checkpoint_interval,
                             bool checkpointed,
                             memory_t memory)
{
    run_result result{};
    result.memory = memory;
    qe6502::cpu cpu(model);

    cpu.restart();
    cpu.jump_to(start_address);
    qe6502_tick_t tick = cpu.raw_tick();

    for (;;) {
        std::uint8_t data = tick_bus_data(tick, result.memory);

        if (tick.address == success_address) {
            result.pass = true;
            result.message = "OK";
            break;
        }

        if (tick_is_write(tick)) {
            result.memory[tick.address] = data;
        } else {
            data = result.memory[tick.address];
        }

        tick = cpu.tick(data);
        ++result.bus_ticks;

        if (checkpointed && checkpoint_interval != 0u &&
            (result.bus_ticks % checkpoint_interval) == 0u) {
            checkpoint_reload(cpu, tick);
        }

        if (tick_is_opcode_fetch(tick)) {
            ++result.opcode_cycles;
            if (result.opcode_cycles > max_opcode_cycles) {
                result.message = "Test fail, takes too many cycles!";
                break;
            }
        }
    }

    result.state = cpu.save();
    return result;
}

int test_klaus_rw_extended_save_load()
{
    memory_t memory{};
    std::uint16_t success_address = 0;
    std::uint64_t expected_cycles = 0;

    copy_klaus2m5_extended_image(memory.data(), &success_address, &expected_cycles);

    const run_result reference = run_until_success(qe6502::model::rw,
                                                   klaus_start_address,
                                                   success_address,
                                                   2u * expected_cycles,
                                                   klaus_checkpoint_interval,
                                                   false,
                                                   memory);
    const run_result checkpointed = run_until_success(qe6502::model::rw,
                                                      klaus_start_address,
                                                      success_address,
                                                      2u * expected_cycles,
                                                      klaus_checkpoint_interval,
                                                      true,
                                                      memory);
    const int failures = compare_results(reference, checkpointed);

    if (failures != 0) {
        std::fprintf(stderr,
                     "Klaus2m5 RW extended save/load checkpoint test failed with %d failure(s)\n",
                     failures);
        return 1;
    }

    std::printf(
        "Rockwell 65C02 extended save/load checkpoint test [PASS] OK "
        "(%llu bus ticks, %llu opcode cycles, checkpoint every %llu ticks)\n",
        static_cast<unsigned long long>(checkpointed.bus_ticks),
        static_cast<unsigned long long>(checkpointed.opcode_cycles),
        static_cast<unsigned long long>(klaus_checkpoint_interval));

    return 0;
}

int test_nes_functional_save_load()
{
    memory_t memory{};
    install_nes_functional_program(memory);

    const run_result reference = run_until_success(qe6502::model::nes,
                                                   nes_start_address,
                                                   nes_success_address,
                                                   nes_max_opcode_cycles,
                                                   nes_checkpoint_interval,
                                                   false,
                                                   memory);
    const run_result checkpointed = run_until_success(qe6502::model::nes,
                                                      nes_start_address,
                                                      nes_success_address,
                                                      nes_max_opcode_cycles,
                                                      nes_checkpoint_interval,
                                                      true,
                                                      memory);
    int failures = compare_results(reference, checkpointed);

    if (checkpointed.memory[0x0200u] != 0x3cu) {
        std::fprintf(stderr,
                     "NES decimal-disabled ADC mismatch: expected memory[0x0200] = 0x3C, got 0x%02X\n",
                     static_cast<unsigned>(checkpointed.memory[0x0200u]));
        ++failures;
    }

    if (checkpointed.memory[0x0201u] != 0x00u) {
        std::fprintf(stderr,
                     "NES loop result mismatch: expected memory[0x0201] = 0x00, got 0x%02X\n",
                     static_cast<unsigned>(checkpointed.memory[0x0201u]));
        ++failures;
    }

    if (failures != 0) {
        std::fprintf(stderr,
                     "NES functional save/load checkpoint test failed with %d failure(s)\n",
                     failures);
        return 1;
    }

    std::printf(
        "NES functional save/load checkpoint test [PASS] OK "
        "(%llu bus ticks, %llu opcode cycles, checkpoint every %llu ticks)\n",
        static_cast<unsigned long long>(checkpointed.bus_ticks),
        static_cast<unsigned long long>(checkpointed.opcode_cycles),
        static_cast<unsigned long long>(nes_checkpoint_interval));

    return 0;
}

void print_usage(const char* argv0)
{
    std::fprintf(stderr,
                 "Usage: %s [klaus-rw-extended|nes-functional|all]\n",
                 argv0);
}

} // namespace

int main(int argc, char** argv)
{
    const std::string test = argc > 1 ? argv[1] : "all";

    if (argc > 2) {
        print_usage(argv[0]);
        return 2;
    }

    if (test == "klaus-rw-extended") {
        return test_klaus_rw_extended_save_load();
    }

    if (test == "nes-functional") {
        return test_nes_functional_save_load();
    }

    if (test == "all") {
        int failures = 0;
        failures += test_klaus_rw_extended_save_load();
        failures += test_nes_functional_save_load();
        return failures == 0 ? 0 : 1;
    }

    print_usage(argv[0]);
    return 2;
}
