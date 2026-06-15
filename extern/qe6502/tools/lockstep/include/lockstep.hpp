#pragma once

#include <cpu6502_bridge/cpu.hpp>
#include <tools6502/common.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace tools6502 {

using memory_image = std::array<std::uint8_t, 65536>;

struct MemoryUnchanged
{
};

struct MemoryFill
{
    std::uint8_t value = 0xeau;
};

struct MemoryRandom
{
    std::uint32_t seed = 0u;
};

struct MemoryCallback
{
    std::function<void(memory_image& memory)> apply;
};

using MemoryInit = std::variant<MemoryUnchanged, MemoryFill, MemoryRandom, MemoryCallback>;

struct CompareOptions
{
    bool address = true;
    bool data = true;
    bool read_write = true;
    bool opcode_fetch = true;
    bool registers_on_fetch = false;
};

struct LockstepConfig
{
    MemoryInit memory = MemoryFill{};
    CompareOptions compare{};
};

struct CpuTraceEntry
{
    std::size_t cycle = 0u;
    std::uint16_t address = 0u;
    std::uint8_t data = 0u;
    bool write = false;
    bool opcode_fetch = false;
    bool irq_asserted = false;
    bool nmi_asserted = false;
    std::uint16_t pc = 0u;
    std::uint8_t a = 0u;
    std::uint8_t x = 0u;
    std::uint8_t y = 0u;
    std::uint8_t s = 0u;
    std::uint8_t p = 0u;
};

struct LockstepRunResult
{
    bool passed = true;
    std::vector<CpuTraceEntry> left_trace{};
    std::vector<CpuTraceEntry> right_trace{};
    std::optional<std::size_t> first_mismatch{};
};

struct Step
{
};

struct StepCycles
{
    std::size_t count = 0u;
};

struct StepToFetch
{
    std::size_t max_steps = 0u;
};

struct StepToFetchAt
{
    std::uint16_t address = 0u;
    std::size_t max_steps = 0u;
};

struct NmiAssert
{
};

struct NmiDeassert
{
};

struct IrqAssert
{
};

struct IrqDeassert
{
};

using LockstepCommand = std::variant<
    Step,
    StepCycles,
    StepToFetch,
    StepToFetchAt,
    NmiAssert,
    NmiDeassert,
    IrqAssert,
    IrqDeassert>;

struct LockstepScenarioConfig
{
    bool stop_on_failure = true;
};

struct LockstepScenarioResult
{
    bool passed = true;
    std::vector<LockstepRunResult> results{};
    std::optional<std::size_t> first_failed_command{};
};

class LockstepRunner
{
public:
    LockstepRunner(std::unique_ptr<cpu6502_bridge::ICpu> left,
                   std::unique_ptr<cpu6502_bridge::ICpu> right);

    LockstepRunner(const LockstepRunner&) = delete;
    LockstepRunner& operator=(const LockstepRunner&) = delete;

    cpu6502_bridge::ICpu& left() noexcept;
    const cpu6502_bridge::ICpu& left() const noexcept;
    cpu6502_bridge::ICpu& right() noexcept;
    const cpu6502_bridge::ICpu& right() const noexcept;

    bool setup_and_run(const LockstepConfig& config = {});
    bool setup_and_run(const testcase& test,
                       const LockstepConfig& config = {});

    LockstepRunResult step();
    LockstepRunResult step_cycles(std::size_t count);
    LockstepRunResult step_to_fetch(std::size_t max_steps);
    LockstepRunResult step_to_fetch_at(std::uint16_t address,
                                       std::size_t max_steps);

    LockstepRunResult irq(bool asserted);
    LockstepRunResult nmi(bool asserted);

private:
    std::unique_ptr<cpu6502_bridge::ICpu> left_;
    std::unique_ptr<cpu6502_bridge::ICpu> right_;
    CompareOptions compare_{};
    std::size_t cycle_ = 0u;
};

class LockstepScenarioRunner
{
public:
    LockstepScenarioRunner(std::unique_ptr<cpu6502_bridge::ICpu> left,
                           std::unique_ptr<cpu6502_bridge::ICpu> right);

    LockstepScenarioRunner(const LockstepScenarioRunner&) = delete;
    LockstepScenarioRunner& operator=(const LockstepScenarioRunner&) = delete;

    bool setup(const testcase& test,
               const LockstepConfig& lockstep_config = {});

    LockstepScenarioResult restart_run(
        const std::vector<LockstepCommand>& commands,
        const LockstepScenarioConfig& scenario_config = {});

    LockstepRunner& lockstep() noexcept;
    const LockstepRunner& lockstep() const noexcept;

private:
    LockstepRunner lockstep_;
    testcase test_{};
    LockstepConfig lockstep_config_{};
    bool has_setup_ = false;
};

} // namespace tools6502
