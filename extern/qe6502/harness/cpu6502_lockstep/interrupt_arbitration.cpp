#include "common.hpp"

#include <asm6502/asm6502.h>
#include <cpu6502_bridge/cpu.hpp>
#include <lockstep.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr std::uint16_t program_start = 0x0400u;
constexpr std::uint16_t nmi_handler = 0x9000u;
constexpr std::uint16_t brk_irq_handler = 0x9100u;

constexpr std::size_t min_pulse_length = 1u;
constexpr std::size_t max_pulse_start_cycle = 8u;
constexpr std::size_t pulse_end_cycle_limit = 10u;
constexpr std::size_t scenario_total_steps = 28u;

struct ArbitrationCase
{
    const char* name = "";
    tools6502::testcase test{};
};

struct GroupStats
{
    std::size_t scenarios = 0u;
    std::size_t passed = 0u;
    std::size_t failures = 0u;
    std::size_t nmi_first_handler_fetches = 0u;
    std::size_t brk_irq_first_handler_fetches = 0u;
    std::size_t nmi_handler_fetches = 0u;
    std::size_t brk_irq_handler_fetches = 0u;
};

struct SummaryStats
{
    std::size_t groups_total = 0u;
    std::size_t groups_passed = 0u;
    std::size_t groups_failed = 0u;
    std::size_t scenarios_total = 0u;
    std::size_t scenarios_passed = 0u;
    std::size_t scenarios_failed = 0u;
    std::size_t nmi_first_handler_fetches = 0u;
    std::size_t brk_irq_first_handler_fetches = 0u;
    std::size_t nmi_handler_fetches = 0u;
    std::size_t brk_irq_handler_fetches = 0u;
};

struct FirstFailure
{
    bool set = false;
    std::string group{};
    std::string scenario{};
    std::size_t pulse_start = 0u;
    std::size_t pulse_length = 0u;
    tools6502::LockstepScenarioResult scenario_result{};
};

enum class FirstHandlerFetch
{
    None,
    Nmi,
    BrkIrq,
};

tools6502::memory_setup make_handlers()
{
    return asm6502::Asm6502::New()
        .begin()
            .org(nmi_handler, "nmi_handler")
                .inx()
                .jmp("nmi_handler")
            .org(brk_irq_handler, "brk_irq_handler")
                .iny()
                .jmp("brk_irq_handler")
        .end().compile();
}

tools6502::memory_setup make_returning_handlers()
{
    return asm6502::Asm6502::New()
        .begin()
            .org(nmi_handler, "nmi_handler")
                .inx()
                .rti()
            .org(brk_irq_handler, "brk_irq_handler")
                .iny()
                .jmp("brk_irq_handler")
        .end().compile();
}

ArbitrationCase make_brk_case(const char* name)
{
    auto program = asm6502::Asm6502::New()
        .begin()
            .org(program_start)
                .brk()
                .nop()
                .nop()
                .nop()
        .end().compile();

    const auto handlers = make_handlers();
    program.insert(program.end(), handlers.begin(), handlers.end());

    tools6502::testcase test{};
    test.opcode = 0x00u;
    test.bytes = 1u;
    test.start_at = program_start;
    test.vectors.reset = 0x0200u;
    test.vectors.nmi = nmi_handler;
    test.vectors.brk_irq = brk_irq_handler;
    test.A = 0x00u;
    test.X = 0x00u;
    test.Y = 0x00u;
    test.P = 0x24u;
    test.S = 0xfdu;
    test.program = std::move(program);
    test.description = name;

    return ArbitrationCase{name, std::move(test)};
}

ArbitrationCase make_overlap_case(const char* name)
{
    auto program = asm6502::Asm6502::New()
        .begin()
            .org(program_start)
                .ldx(0xfdu)
                .txs()
                .nop()
                .nop()
                .nop()
                .nop()
        .end().compile();

    const auto handlers = make_handlers();
    program.insert(program.end(), handlers.begin(), handlers.end());

    tools6502::testcase test{};
    test.opcode = 0xeau;
    test.bytes = 1u;
    test.start_at = program_start;
    test.vectors.reset = 0x0200u;
    test.vectors.nmi = nmi_handler;
    test.vectors.brk_irq = brk_irq_handler;
    test.A = 0x00u;
    test.X = 0x00u;
    test.Y = 0x00u;
    test.P = 0x20u;
    test.S = 0xfdu;
    test.program = std::move(program);
    test.description = name;

    return ArbitrationCase{name, std::move(test)};
}

ArbitrationCase make_returning_interrupt_case(const char* name)
{
    auto program = asm6502::Asm6502::New()
        .begin()
            .org(program_start)
                .ldx(0xfdu)
                .txs()
                .nop()
                .nop()
                .nop()
                .nop()
        .end().compile();

    const auto handlers = make_returning_handlers();
    program.insert(program.end(), handlers.begin(), handlers.end());

    tools6502::testcase test{};
    test.opcode = 0xeau;
    test.bytes = 1u;
    test.start_at = program_start;
    test.vectors.reset = 0x0200u;
    test.vectors.nmi = nmi_handler;
    test.vectors.brk_irq = brk_irq_handler;
    test.A = 0x00u;
    test.X = 0x00u;
    test.Y = 0x00u;
    test.P = 0x20u;
    test.S = 0xfdu;
    test.program = std::move(program);
    test.description = name;

    return ArbitrationCase{name, std::move(test)};
}

FirstHandlerFetch observe_first_handler_fetch(const tools6502::LockstepScenarioResult& result)
{
    for (const auto& command_result : result.results) {
        for (const auto& entry : command_result.left_trace) {
            if (!entry.opcode_fetch) {
                continue;
            }
            if (entry.address == nmi_handler) {
                return FirstHandlerFetch::Nmi;
            }
            if (entry.address == brk_irq_handler) {
                return FirstHandlerFetch::BrkIrq;
            }
        }
    }

    return FirstHandlerFetch::None;
}

std::pair<std::size_t, std::size_t> count_handler_fetches(
    const tools6502::LockstepScenarioResult& result)
{
    std::size_t nmi_fetches = 0u;
    std::size_t brk_irq_fetches = 0u;

    for (const auto& command_result : result.results) {
        for (const auto& entry : command_result.left_trace) {
            if (!entry.opcode_fetch) {
                continue;
            }
            if (entry.address == nmi_handler) {
                ++nmi_fetches;
            }
            if (entry.address == brk_irq_handler) {
                ++brk_irq_fetches;
            }
        }
    }

    return {nmi_fetches, brk_irq_fetches};
}

tools6502::LockstepConfig make_lockstep_config()
{
    tools6502::LockstepConfig lockstep_config{};
    lockstep_config.memory = tools6502::MemoryFill{0xeau};
    lockstep_config.compare.address = true;
    lockstep_config.compare.data = true;
    lockstep_config.compare.read_write = true;
    lockstep_config.compare.opcode_fetch = true;
    lockstep_config.compare.registers_on_fetch = false;
    return lockstep_config;
}

tools6502::LockstepScenarioConfig make_scenario_config()
{
    tools6502::LockstepScenarioConfig scenario_config{};
    scenario_config.stop_on_failure = true;
    return scenario_config;
}

void record_result(const tools6502::LockstepScenarioResult& result,
                   SummaryStats& summary,
                   GroupStats& group)
{
    ++summary.scenarios_total;
    ++group.scenarios;

    const auto first_handler = observe_first_handler_fetch(result);
    if (first_handler == FirstHandlerFetch::Nmi) {
        ++summary.nmi_first_handler_fetches;
        ++group.nmi_first_handler_fetches;
    }
    if (first_handler == FirstHandlerFetch::BrkIrq) {
        ++summary.brk_irq_first_handler_fetches;
        ++group.brk_irq_first_handler_fetches;
    }

    const auto handler_fetches = count_handler_fetches(result);
    summary.nmi_handler_fetches += handler_fetches.first;
    group.nmi_handler_fetches += handler_fetches.first;
    summary.brk_irq_handler_fetches += handler_fetches.second;
    group.brk_irq_handler_fetches += handler_fetches.second;

    if (result.passed) {
        ++summary.scenarios_passed;
        ++group.passed;
    } else {
        ++summary.scenarios_failed;
        ++group.failures;
    }
}

bool run_one_pulse_scenario(tools6502::LockstepScenarioRunner& runner,
                            bool nmi,
                            std::size_t pulse_start,
                            std::size_t pulse_length,
                            const tools6502::LockstepScenarioConfig& scenario_config,
                            SummaryStats& summary,
                            GroupStats& group,
                            FirstFailure& first_failure,
                            const char* group_name)
{
    const auto script = cpu6502_lockstep::make_pulse_script(
        nmi,
        pulse_start,
        pulse_length,
        scenario_total_steps);

    const auto result = runner.restart_run(script, scenario_config);

    record_result(result, summary, group);

    if (result.passed) {
        return true;
    }

    if (!first_failure.set) {
        first_failure.set = true;
        first_failure.group = group_name;
        first_failure.scenario = "pulse";
        first_failure.pulse_start = pulse_start;
        first_failure.pulse_length = pulse_length;
        first_failure.scenario_result = result;
    }

    return false;
}

GroupStats run_pulse_group(const char* group_name,
                           const ArbitrationCase& test_case,
                           bool nmi,
                           SummaryStats& summary,
                           FirstFailure& first_failure)
{
    const auto lockstep_config = make_lockstep_config();
    const auto scenario_config = make_scenario_config();

    tools6502::LockstepScenarioRunner runner(
        cpu6502_bridge::make_qe6502_cpu(),
        cpu6502_bridge::make_perfect6502_cpu());

    GroupStats group{};
    if (!runner.setup(test_case.test, lockstep_config)) {
        ++summary.scenarios_total;
        ++summary.scenarios_failed;
        ++group.scenarios;
        ++group.failures;
        if (!first_failure.set) {
            first_failure.set = true;
            first_failure.group = group_name;
        }
        return group;
    }

    for (std::size_t pulse_start = 0u;
         pulse_start <= max_pulse_start_cycle;
         ++pulse_start) {
        const std::size_t max_pulse_length = pulse_end_cycle_limit - pulse_start;
        for (std::size_t pulse_length = min_pulse_length;
             pulse_length <= max_pulse_length;
             ++pulse_length) {
            run_one_pulse_scenario(runner,
                                   nmi,
                                   pulse_start,
                                   pulse_length,
                                   scenario_config,
                                   summary,
                                   group,
                                   first_failure,
                                   group_name);
        }
    }

    return group;
}

std::vector<tools6502::LockstepCommand> make_same_cycle_overlap_script()
{
    return {
        tools6502::IrqAssert{},
        tools6502::NmiAssert{},
        tools6502::StepCycles{2u},
        tools6502::NmiDeassert{},
        tools6502::IrqDeassert{},
        tools6502::StepCycles{26u},
    };
}

std::vector<tools6502::LockstepCommand> make_irq_first_overlap_script()
{
    return {
        tools6502::IrqAssert{},
        tools6502::StepCycles{1u},
        tools6502::NmiAssert{},
        tools6502::StepCycles{2u},
        tools6502::NmiDeassert{},
        tools6502::IrqDeassert{},
        tools6502::StepCycles{25u},
    };
}

std::vector<tools6502::LockstepCommand> make_nmi_first_overlap_script()
{
    return {
        tools6502::NmiAssert{},
        tools6502::StepCycles{1u},
        tools6502::IrqAssert{},
        tools6502::StepCycles{2u},
        tools6502::NmiDeassert{},
        tools6502::IrqDeassert{},
        tools6502::StepCycles{25u},
    };
}

std::vector<tools6502::LockstepCommand> make_irq_held_nmi_pulse_script()
{
    return {
        tools6502::IrqAssert{},
        tools6502::StepCycles{2u},
        tools6502::NmiAssert{},
        tools6502::StepCycles{1u},
        tools6502::NmiDeassert{},
        tools6502::StepCycles{4u},
        tools6502::IrqDeassert{},
        tools6502::StepCycles{21u},
    };
}

std::vector<tools6502::LockstepCommand> make_nmi_pulse_irq_held_after_script()
{
    return {
        tools6502::NmiAssert{},
        tools6502::StepCycles{1u},
        tools6502::NmiDeassert{},
        tools6502::StepCycles{1u},
        tools6502::IrqAssert{},
        tools6502::StepCycles{26u},
        tools6502::IrqDeassert{},
    };
}

std::vector<tools6502::LockstepCommand> make_nmi_deasserts_first_overlap_script()
{
    return {
        tools6502::IrqAssert{},
        tools6502::NmiAssert{},
        tools6502::StepCycles{3u},
        tools6502::NmiDeassert{},
        tools6502::StepCycles{3u},
        tools6502::IrqDeassert{},
        tools6502::StepCycles{22u},
    };
}

std::vector<tools6502::LockstepCommand> make_irq_deasserts_first_overlap_script()
{
    return {
        tools6502::IrqAssert{},
        tools6502::NmiAssert{},
        tools6502::StepCycles{3u},
        tools6502::IrqDeassert{},
        tools6502::StepCycles{3u},
        tools6502::NmiDeassert{},
        tools6502::StepCycles{22u},
    };
}

struct OverlapScenario
{
    const char* name = "";
    std::vector<tools6502::LockstepCommand> script{};
};

std::vector<OverlapScenario> make_overlap_scenarios()
{
    return {
        {"IRQ and NMI asserted same cycle", make_same_cycle_overlap_script()},
        {"IRQ asserted first, NMI one cycle later", make_irq_first_overlap_script()},
        {"NMI asserted first, IRQ one cycle later", make_nmi_first_overlap_script()},
        {"IRQ held, short NMI pulse", make_irq_held_nmi_pulse_script()},
        {"NMI pulse, IRQ held after", make_nmi_pulse_irq_held_after_script()},
        {"IRQ/NMI overlap, NMI deasserts first", make_nmi_deasserts_first_overlap_script()},
        {"IRQ/NMI overlap, IRQ deasserts first", make_irq_deasserts_first_overlap_script()},
    };
}

std::vector<tools6502::LockstepCommand> returning_case_prefix()
{
    return {
        tools6502::StepToFetch{8u},
        tools6502::StepToFetch{8u},
    };
}

std::vector<tools6502::LockstepCommand> with_returning_case_prefix(
    std::vector<tools6502::LockstepCommand> script)
{
    auto prefixed = returning_case_prefix();
    prefixed.insert(prefixed.end(), script.begin(), script.end());
    return prefixed;
}

std::vector<tools6502::LockstepCommand> make_nmi_continuous_low_script()
{
    return with_returning_case_prefix({
        tools6502::StepCycles{1u},
        tools6502::NmiAssert{},
        tools6502::StepCycles{44u},
        tools6502::NmiDeassert{},
        tools6502::StepCycles{8u},
    });
}

std::vector<tools6502::LockstepCommand> make_nmi_reedge_script()
{
    return with_returning_case_prefix({
        tools6502::StepCycles{1u},
        tools6502::NmiAssert{},
        tools6502::StepCycles{1u},
        tools6502::NmiDeassert{},
        tools6502::StepCycles{20u},
        tools6502::NmiAssert{},
        tools6502::StepCycles{1u},
        tools6502::NmiDeassert{},
        tools6502::StepCycles{40u},
    });
}

std::vector<OverlapScenario> make_nmi_edge_lifetime_scenarios()
{
    return {
        {"NMI held continuously low through handler/RTI", make_nmi_continuous_low_script()},
        {"NMI deassert/reassert creates second edge", make_nmi_reedge_script()},
    };
}

std::vector<tools6502::LockstepCommand> make_second_nmi_offset_script(std::size_t offset)
{
    return with_returning_case_prefix({
        tools6502::StepCycles{1u},
        tools6502::NmiAssert{},
        tools6502::StepCycles{1u},
        tools6502::NmiDeassert{},
        tools6502::StepCycles{offset},
        tools6502::NmiAssert{},
        tools6502::StepCycles{1u},
        tools6502::NmiDeassert{},
        tools6502::StepCycles{48u},
    });
}

std::vector<OverlapScenario> make_nested_nmi_scenarios()
{
    std::vector<OverlapScenario> scenarios{};
    for (std::size_t offset = 0u; offset <= 12u; ++offset) {
        scenarios.push_back({
            "second NMI edge offset sweep",
            make_second_nmi_offset_script(offset),
        });
    }
    return scenarios;
}

std::vector<tools6502::LockstepCommand> make_irq_held_during_nmi_script()
{
    return with_returning_case_prefix({
        tools6502::StepCycles{1u},
        tools6502::IrqAssert{},
        tools6502::StepCycles{1u},
        tools6502::NmiAssert{},
        tools6502::StepCycles{1u},
        tools6502::NmiDeassert{},
        tools6502::StepCycles{54u},
        tools6502::IrqDeassert{},
    });
}

std::vector<tools6502::LockstepCommand> make_irq_asserted_inside_nmi_service_script()
{
    return with_returning_case_prefix({
        tools6502::StepCycles{1u},
        tools6502::NmiAssert{},
        tools6502::StepCycles{1u},
        tools6502::NmiDeassert{},
        tools6502::StepCycles{7u},
        tools6502::IrqAssert{},
        tools6502::StepCycles{48u},
        tools6502::IrqDeassert{},
    });
}

std::vector<OverlapScenario> make_irq_during_nmi_scenarios()
{
    return {
        {"IRQ held before and through NMI service", make_irq_held_during_nmi_script()},
        {"IRQ asserted inside NMI service", make_irq_asserted_inside_nmi_service_script()},
    };
}

GroupStats run_named_script_group(const char* group_name,
                                 const ArbitrationCase& test_case,
                                 const std::vector<OverlapScenario>& scenarios,
                                 SummaryStats& summary,
                                 FirstFailure& first_failure)
{
    const auto lockstep_config = make_lockstep_config();
    const auto scenario_config = make_scenario_config();

    tools6502::LockstepScenarioRunner runner(
        cpu6502_bridge::make_qe6502_cpu(),
        cpu6502_bridge::make_perfect6502_cpu());

    GroupStats group{};
    if (!runner.setup(test_case.test, lockstep_config)) {
        ++summary.scenarios_total;
        ++summary.scenarios_failed;
        ++group.scenarios;
        ++group.failures;
        if (!first_failure.set) {
            first_failure.set = true;
            first_failure.group = group_name;
            first_failure.scenario = "setup";
        }
        return group;
    }

    for (const auto& scenario : scenarios) {
        const auto result = runner.restart_run(scenario.script, scenario_config);
        record_result(result, summary, group);

        if (!result.passed && !first_failure.set) {
            first_failure.set = true;
            first_failure.group = group_name;
            first_failure.scenario = scenario.name;
            first_failure.scenario_result = result;
        }
    }

    return group;
}

} // namespace

int main()
{
    SummaryStats summary{};
    FirstFailure first_failure{};

    const auto brk_nmi_case = make_brk_case("BRK + NMI hijack windows");
    const auto brk_irq_case = make_brk_case("BRK + IRQ windows");
    const auto overlap_case = make_overlap_case("IRQ/NMI overlap priority");
    const auto returning_case = make_returning_interrupt_case("returning interrupt handler case");

    auto report_group = [&summary](const char* name,
                                   const GroupStats& group,
                                   std::size_t hijacks) {
        const bool group_passed = group.failures == 0u;
        if (group_passed) {
            ++summary.groups_passed;
        } else {
            ++summary.groups_failed;
        }

        std::cout << name << ": " << (group_passed ? "PASS" : "FAIL")
                  << " scenarios=" << group.scenarios
                  << " failures=" << group.failures
                  << " hijacks=" << hijacks
                  << " nmi_first_handler_fetches=" << group.nmi_first_handler_fetches
                  << " brk_irq_first_handler_fetches=" << group.brk_irq_first_handler_fetches
                  << " nmi_handler_fetches=" << group.nmi_handler_fetches
                  << " brk_irq_handler_fetches=" << group.brk_irq_handler_fetches
                  << '\n' << std::flush;
    };

    ++summary.groups_total;
    const auto brk_nmi_group = run_pulse_group(
        "BRK + NMI hijack windows",
        brk_nmi_case,
        true,
        summary,
        first_failure);
    report_group("BRK + NMI hijack windows",
                 brk_nmi_group,
                 brk_nmi_group.nmi_first_handler_fetches);

    ++summary.groups_total;
    const auto brk_irq_group = run_pulse_group(
        "BRK + IRQ windows",
        brk_irq_case,
        false,
        summary,
        first_failure);
    report_group("BRK + IRQ windows", brk_irq_group, 0u);

    ++summary.groups_total;
    const auto overlap_group = run_named_script_group(
        "IRQ/NMI overlap priority",
        overlap_case,
        make_overlap_scenarios(),
        summary,
        first_failure);
    report_group("IRQ/NMI overlap priority",
                 overlap_group,
                 overlap_group.nmi_first_handler_fetches);

    ++summary.groups_total;
    const auto nmi_edge_lifetime_group = run_named_script_group(
        "NMI continuous-low vs re-edge",
        returning_case,
        make_nmi_edge_lifetime_scenarios(),
        summary,
        first_failure);
    report_group("NMI continuous-low vs re-edge",
                 nmi_edge_lifetime_group,
                 nmi_edge_lifetime_group.nmi_first_handler_fetches);

    ++summary.groups_total;
    const auto nested_nmi_group = run_named_script_group(
        "NMI during NMI service / lost windows",
        returning_case,
        make_nested_nmi_scenarios(),
        summary,
        first_failure);
    report_group("NMI during NMI service / lost windows",
                 nested_nmi_group,
                 nested_nmi_group.nmi_first_handler_fetches);

    ++summary.groups_total;
    const auto irq_during_nmi_group = run_named_script_group(
        "IRQ held during NMI service",
        returning_case,
        make_irq_during_nmi_scenarios(),
        summary,
        first_failure);
    report_group("IRQ held during NMI service",
                 irq_during_nmi_group,
                 irq_during_nmi_group.brk_irq_first_handler_fetches);

    std::cout << "\nsummary\n"
              << "  groups:    total=" << summary.groups_total
              << " pass=" << summary.groups_passed
              << " fail=" << summary.groups_failed << '\n'
              << "  scenarios: total=" << summary.scenarios_total
              << " pass=" << summary.scenarios_passed
              << " fail=" << summary.scenarios_failed << '\n'
              << "  first handler fetches: nmi=" << summary.nmi_first_handler_fetches
              << " brk_irq=" << summary.brk_irq_first_handler_fetches << '\n'
              << "  all handler fetches:   nmi=" << summary.nmi_handler_fetches
              << " brk_irq=" << summary.brk_irq_handler_fetches << '\n';

    if (first_failure.set) {
        std::cout << "\nfirst failure\n"
                  << "  group:        " << first_failure.group << '\n';
        if (!first_failure.scenario.empty()) {
            std::cout << "  scenario:     " << first_failure.scenario << '\n';
        }
        std::cout << "  pulse_start:  " << first_failure.pulse_start << '\n'
                  << "  pulse_length: " << first_failure.pulse_length << '\n';

        if (first_failure.scenario_result.first_failed_command) {
            const auto command_index = *first_failure.scenario_result.first_failed_command;
            std::cout << "  command:      " << command_index << '\n';
            if (command_index < first_failure.scenario_result.results.size()) {
                cpu6502_lockstep::print_failure_trace(
                    std::cout,
                    first_failure.scenario_result.results[command_index]);
            }
        }
    }

    return summary.scenarios_failed == 0u ? 0 : 1;
}
