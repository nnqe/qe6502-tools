#include "common.hpp"

#include <asm6502/asm6502.h>
#include <cpu6502_bridge/cpu.hpp>
#include <lockstep.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr std::uint16_t program_start = 0x0400u;
constexpr std::uint16_t irq_handler = 0x0800u;
constexpr std::uint16_t nmi_handler = 0x0900u;

constexpr std::size_t min_pulse_length = 1u;
constexpr std::size_t max_irq_start_cycle = 6u;
constexpr std::size_t pulse_end_cycle_limit = 8u;
constexpr std::size_t scenario_total_steps = 18u;

struct IFlagCase
{
    const char* name = "";
    tools6502::testcase test{};
};

struct GroupStats
{
    std::size_t scenarios = 0u;
    std::size_t failures = 0u;
};

struct SummaryStats
{
    std::size_t groups_total = 0u;
    std::size_t groups_passed = 0u;
    std::size_t groups_failed = 0u;
    std::size_t scenarios_total = 0u;
    std::size_t scenarios_passed = 0u;
    std::size_t scenarios_failed = 0u;
};

struct FirstFailure
{
    bool set = false;
    std::string group{};
    std::size_t irq_start = 0u;
    std::size_t irq_length = 0u;
    tools6502::LockstepScenarioResult scenario_result{};
};

tools6502::memory_setup make_common_handlers()
{
    return asm6502::Asm6502::New()
        .begin()
            .org(irq_handler, "irq_handler")
                .nop()
                .jmp("irq_handler")
            .org(nmi_handler, "nmi_handler")
                .nop()
                .jmp("nmi_handler")
        .end().compile();
}

IFlagCase make_cli_case()
{
    auto program = asm6502::Asm6502::New()
        .begin()
            .org(program_start)
                .cli()
                .nop()
                .nop()
                .nop()
        .end().compile();

    const auto handlers = make_common_handlers();
    program.insert(program.end(), handlers.begin(), handlers.end());

    tools6502::testcase test{};
    test.opcode = 0x58u;
    test.bytes = 1u;
    test.start_at = program_start;
    test.vectors.reset = 0x0200u;
    test.vectors.brk_irq = irq_handler;
    test.vectors.nmi = nmi_handler;
    test.A = 0x00u;
    test.X = 0x00u;
    test.Y = 0x00u;
    test.P = 0x24u; // I set before CLI.
    test.S = 0xfdu;
    test.program = std::move(program);
    test.description = "CLI IRQ window";

    return IFlagCase{"CLI IRQ window", std::move(test)};
}

IFlagCase make_sei_case()
{
    auto program = asm6502::Asm6502::New()
        .begin()
            .org(program_start)
                .sei()
                .nop()
                .nop()
                .nop()
        .end().compile();

    const auto handlers = make_common_handlers();
    program.insert(program.end(), handlers.begin(), handlers.end());

    tools6502::testcase test{};
    test.opcode = 0x78u;
    test.bytes = 1u;
    test.start_at = program_start;
    test.vectors.reset = 0x0200u;
    test.vectors.brk_irq = irq_handler;
    test.vectors.nmi = nmi_handler;
    test.A = 0x00u;
    test.X = 0x00u;
    test.Y = 0x00u;
    test.P = 0x20u; // I clear before SEI.
    test.S = 0xfdu;
    test.program = std::move(program);
    test.description = "SEI IRQ window";

    return IFlagCase{"SEI IRQ window", std::move(test)};
}


IFlagCase make_plp_case(const char* name, std::uint8_t pulled_p)
{
    auto program = asm6502::Asm6502::New()
        .begin()
            .org(0x01fdu, "saved_p")
                .db(pulled_p)
            .org(program_start)
                .plp()
                .nop()
                .nop()
                .nop()
        .end().compile();

    const auto handlers = make_common_handlers();
    program.insert(program.end(), handlers.begin(), handlers.end());

    tools6502::testcase test{};
    test.opcode = 0x28u;
    test.bytes = 1u;
    test.start_at = program_start;
    test.vectors.reset = 0x0200u;
    test.vectors.brk_irq = irq_handler;
    test.vectors.nmi = nmi_handler;
    test.A = 0x00u;
    test.X = 0x00u;
    test.Y = 0x00u;
    test.P = 0x24u;
    test.S = 0xfcu; // PLP pulls from $01FD.
    test.program = std::move(program);
    test.description = name;

    return IFlagCase{name, std::move(test)};
}

IFlagCase make_rti_case(const char* name, std::uint8_t pulled_p)
{
    constexpr std::uint16_t return_target = 0x0450u;

    auto program = asm6502::Asm6502::New()
        .begin()
            .org(0x01fbu, "saved_p")
                .db(pulled_p)
                .dw(return_target)
            .org(program_start)
                .rti()
            .org(return_target)
                .nop()
                .nop()
                .nop()
        .end().compile();

    const auto handlers = make_common_handlers();
    program.insert(program.end(), handlers.begin(), handlers.end());

    tools6502::testcase test{};
    test.opcode = 0x40u;
    test.bytes = 1u;
    test.start_at = program_start;
    test.vectors.reset = 0x0200u;
    test.vectors.brk_irq = irq_handler;
    test.vectors.nmi = nmi_handler;
    test.A = 0x00u;
    test.X = 0x00u;
    test.Y = 0x00u;
    test.P = 0x24u;
    test.S = 0xfau; // RTI pulls P/PCL/PCH from $01FB..$01FD.
    test.program = std::move(program);
    test.description = name;

    return IFlagCase{name, std::move(test)};
}

std::vector<IFlagCase> make_i_flag_cases()
{
    std::vector<IFlagCase> cases;
    cases.push_back(make_cli_case());
    cases.push_back(make_sei_case());
    cases.push_back(make_plp_case("PLP IRQ window I=0", 0x20u));
    cases.push_back(make_plp_case("PLP IRQ window I=1", 0x24u));
    cases.push_back(make_rti_case("RTI IRQ window I=0", 0x20u));
    cases.push_back(make_rti_case("RTI IRQ window I=1", 0x24u));
    return cases;
}

bool run_one_irq_scenario(tools6502::LockstepScenarioRunner& runner,
                          const IFlagCase& test_case,
                          std::size_t irq_start,
                          std::size_t irq_length,
                          const tools6502::LockstepScenarioConfig& scenario_config,
                          SummaryStats& summary,
                          GroupStats& group_stats,
                          FirstFailure& first_failure)
{
    const auto script = cpu6502_lockstep::make_pulse_script(
        false,
        irq_start,
        irq_length,
        scenario_total_steps);

    const auto result = runner.restart_run(script, scenario_config);

    ++summary.scenarios_total;
    ++group_stats.scenarios;

    if (result.passed) {
        ++summary.scenarios_passed;
        return true;
    }

    ++summary.scenarios_failed;
    ++group_stats.failures;

    if (!first_failure.set) {
        first_failure.set = true;
        first_failure.group = test_case.name;
        first_failure.irq_start = irq_start;
        first_failure.irq_length = irq_length;
        first_failure.scenario_result = result;
    }

    return false;
}

} // namespace

int main()
{
    const auto cases = make_i_flag_cases();

    tools6502::LockstepConfig lockstep_config{};
    lockstep_config.memory = tools6502::MemoryFill{0xeau};
    lockstep_config.compare.address = true;
    lockstep_config.compare.data = true;
    lockstep_config.compare.read_write = true;
    lockstep_config.compare.opcode_fetch = true;
    lockstep_config.compare.registers_on_fetch = false;

    tools6502::LockstepScenarioConfig scenario_config{};
    scenario_config.stop_on_failure = true;

    SummaryStats summary{};
    FirstFailure first_failure{};

    for (const auto& test_case : cases) {
        ++summary.groups_total;

        tools6502::LockstepScenarioRunner runner(
            cpu6502_bridge::make_qe6502_cpu(),
            cpu6502_bridge::make_perfect6502_cpu());

        GroupStats group{};
        bool group_passed = true;

        if (!runner.setup(test_case.test, lockstep_config)) {
            group_passed = false;
            ++summary.scenarios_total;
            ++summary.scenarios_failed;
            ++group.scenarios;
            ++group.failures;
            if (!first_failure.set) {
                first_failure.set = true;
                first_failure.group = test_case.name;
            }
        } else {
            for (std::size_t irq_start = 0u;
                 irq_start <= max_irq_start_cycle;
                 ++irq_start) {
                const std::size_t max_pulse_length = pulse_end_cycle_limit - irq_start;

                for (std::size_t irq_length = min_pulse_length;
                     irq_length <= max_pulse_length;
                     ++irq_length) {
                    if (!run_one_irq_scenario(runner,
                                              test_case,
                                              irq_start,
                                              irq_length,
                                              scenario_config,
                                              summary,
                                              group,
                                              first_failure)) {
                        group_passed = false;
                    }
                }
            }
        }

        if (group_passed) {
            ++summary.groups_passed;
        } else {
            ++summary.groups_failed;
        }

        std::cout << test_case.name << ": " << (group_passed ? "PASS" : "FAIL")
                  << " scenarios=" << group.scenarios
                  << " failures=" << group.failures
                  << '\n' << std::flush;
    }

    std::cout << "\nsummary\n"
              << "  groups:    total=" << summary.groups_total
              << " pass=" << summary.groups_passed
              << " fail=" << summary.groups_failed << '\n'
              << "  scenarios: total=" << summary.scenarios_total
              << " pass=" << summary.scenarios_passed
              << " fail=" << summary.scenarios_failed << '\n';

    if (first_failure.set) {
        std::cout << "\nfirst failure\n"
                  << "  group:      " << first_failure.group << '\n'
                  << "  irq_start:  " << first_failure.irq_start << '\n'
                  << "  irq_length: " << first_failure.irq_length << '\n';

        if (first_failure.scenario_result.first_failed_command) {
            const auto command_index = *first_failure.scenario_result.first_failed_command;
            std::cout << "  command:    " << command_index << '\n';
            if (command_index < first_failure.scenario_result.results.size()) {
                cpu6502_lockstep::print_failure_trace(
                    std::cout,
                    first_failure.scenario_result.results[command_index]);
            }
        }
    }

    return summary.scenarios_failed == 0u ? 0 : 1;
}
