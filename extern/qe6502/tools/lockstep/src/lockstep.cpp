#include <lockstep.hpp>

#include <algorithm>
#include <cstring>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace tools6502 {
namespace {

constexpr unsigned setup_restart_max_steps = 256u;
constexpr unsigned setup_fetch_max_steps = 32u;
constexpr unsigned setup_bootstrap_max_steps = 4096u;
constexpr std::uint16_t safe_restart_address = 0x0400u;
constexpr std::uint16_t internal_bootstrap_address = 0xfe00u;
constexpr std::uint8_t opcode_nop = 0xeau;

struct SavedMemoryPatch
{
    static constexpr std::uint16_t addresses[] = {
        safe_restart_address + 0u,
        safe_restart_address + 1u,
        safe_restart_address + 2u,
        0xfffcu,
        0xfffdu
    };

    std::uint8_t values[sizeof(addresses) / sizeof(addresses[0])]{};
    bool active = false;

    void save(cpu6502_bridge::ICpu& cpu) noexcept
    {
        auto* memory = cpu.memory();
        for (std::size_t i = 0u; i < sizeof(addresses) / sizeof(addresses[0]); ++i) {
            values[i] = memory[addresses[i]];
        }
        active = true;
    }

    void restore(cpu6502_bridge::ICpu& cpu) noexcept
    {
        if (!active) {
            return;
        }

        auto* memory = cpu.memory();
        for (std::size_t i = 0u; i < sizeof(addresses) / sizeof(addresses[0]); ++i) {
            memory[addresses[i]] = values[i];
        }
        active = false;
    }
};

void copy_memory_to_cpu(cpu6502_bridge::ICpu& cpu, const memory_image& image)
{
    std::memcpy(cpu.memory(), image.data(), image.size());
}

void apply_mem_values_to_cpu(cpu6502_bridge::ICpu& cpu, const memory_setup& setup)
{
    auto* memory = cpu.memory();
    for (const auto& [address, value] : setup) {
        memory[address] = value;
    }
}

void write_vector_to_cpu(cpu6502_bridge::ICpu& cpu,
                         std::uint16_t vector_address,
                         std::uint16_t target)
{
    auto* memory = cpu.memory();
    memory[vector_address] = static_cast<std::uint8_t>(target & 0xffu);
    memory[static_cast<std::uint16_t>(vector_address + 1u)] =
        static_cast<std::uint8_t>((target >> 8u) & 0xffu);
}

void install_safe_restart_stub(cpu6502_bridge::ICpu& cpu)
{
    auto* memory = cpu.memory();
    memory[safe_restart_address + 0u] = opcode_nop;
    memory[safe_restart_address + 1u] = opcode_nop;
    memory[safe_restart_address + 2u] = opcode_nop;
    write_vector_to_cpu(cpu, 0xfffcu, safe_restart_address);
}

void initialize_memory_image(memory_image& image, const MemoryInit& init)
{
    std::visit([&image](const auto& policy) {
        using Policy = std::decay_t<decltype(policy)>;
        if constexpr (std::is_same_v<Policy, MemoryUnchanged>) {
            // No full-memory initialization. Callers apply only their explicit patches.
        } else if constexpr (std::is_same_v<Policy, MemoryFill>) {
            image.fill(policy.value);
        } else if constexpr (std::is_same_v<Policy, MemoryRandom>) {
            std::mt19937 rng(policy.seed);
            std::uniform_int_distribution<int> dist(0, 255);
            for (auto& byte : image) {
                byte = static_cast<std::uint8_t>(dist(rng));
            }
        } else if constexpr (std::is_same_v<Policy, MemoryCallback>) {
            if (policy.apply) {
                policy.apply(image);
            }
        }
    }, init);
}

bool initializes_full_memory(const MemoryInit& init)
{
    return !std::holds_alternative<MemoryUnchanged>(init);
}

void initialize_cpu_pair_memory(cpu6502_bridge::ICpu& left,
                                cpu6502_bridge::ICpu& right,
                                const MemoryInit& init)
{
    if (!initializes_full_memory(init)) {
        return;
    }

    memory_image image{};
    initialize_memory_image(image, init);
    copy_memory_to_cpu(left, image);
    copy_memory_to_cpu(right, image);
}

bool cpu_at_fetch_at(const cpu6502_bridge::ICpu& cpu,
                     std::uint16_t address) noexcept
{
    return cpu.is_opcode_fetch()
        && !cpu.is_write()
        && cpu.bus_address() == address;
}

std::uint16_t reset_vector(const cpu6502_bridge::ICpu& cpu) noexcept
{
    const auto* mem = const_cast<cpu6502_bridge::ICpu&>(cpu).memory();
    return static_cast<std::uint16_t>(mem[0xfffcu])
        | static_cast<std::uint16_t>(static_cast<std::uint16_t>(mem[0xfffdu]) << 8u);
}

bool restart_to_reset_fetch(cpu6502_bridge::ICpu& cpu)
{
    const std::uint16_t target = reset_vector(cpu);
    cpu.restart_to_start_fetch(setup_restart_max_steps);
    return cpu_at_fetch_at(cpu, target);
}

bool step_to_next_fetch_without_compare(cpu6502_bridge::ICpu& cpu,
                                        unsigned max_steps)
{
    for (unsigned steps = 0u; steps < max_steps; ++steps) {
        cpu.step();
        if (cpu.is_opcode_fetch() && !cpu.is_write()) {
            return true;
        }
    }

    return false;
}

bool step_to_fetch_at_without_compare(cpu6502_bridge::ICpu& cpu,
                                      std::uint16_t address,
                                      unsigned max_steps = setup_bootstrap_max_steps)
{
    for (unsigned steps = 0u; steps < max_steps; ++steps) {
        if (cpu_at_fetch_at(cpu, address)) {
            return true;
        }
        cpu.step();
    }

    return cpu_at_fetch_at(cpu, address);
}

bool hard_restart_to_safe_fetch(cpu6502_bridge::ICpu& cpu,
                                SavedMemoryPatch& patch)
{
    cpu.irq(false);
    cpu.nmi(false);
    patch.save(cpu);
    install_safe_restart_stub(cpu);
    cpu.restart_to_start_fetch(setup_restart_max_steps);
    return cpu_at_fetch_at(cpu, safe_restart_address);
}

testcase make_temporary_bootstrap_testcase(const testcase& test)
{
    auto temporary = test;
    temporary.vectors.reset = internal_bootstrap_address;
    temporary.vectors.brk_irq = internal_bootstrap_address;
    temporary.vectors.nmi = internal_bootstrap_address;
    return temporary;
}

void install_user_testcase(cpu6502_bridge::ICpu& cpu, const testcase& test)
{
    apply_mem_values_to_cpu(cpu, test.mem_setup);
    apply_mem_values_to_cpu(cpu, test.program);
    write_vector_to_cpu(cpu, 0xfffcu, test.vectors.reset);
    write_vector_to_cpu(cpu, 0xfffau, test.vectors.nmi);
    write_vector_to_cpu(cpu, 0xfffeu, test.vectors.brk_irq);
}

bool try_soft_setup_and_run(cpu6502_bridge::ICpu& cpu,
                            const testcase& test,
                            SavedMemoryPatch* hard_restart_patch = nullptr)
{
    const auto temporary_test = make_temporary_bootstrap_testcase(test);
    const auto temporary_bootstrap = make_bootstrap(temporary_test);

    apply_mem_values_to_cpu(cpu, temporary_bootstrap);
    cpu.nmi(false);
    cpu.irq(false);

    if (!step_to_next_fetch_without_compare(cpu, setup_fetch_max_steps)) {
        return false;
    }

    cpu.nmi(true);
    const bool entered_bootstrap =
        step_to_fetch_at_without_compare(cpu, internal_bootstrap_address, setup_fetch_max_steps);
    cpu.nmi(false);

    if (!entered_bootstrap) {
        return false;
    }

    if (hard_restart_patch != nullptr) {
        hard_restart_patch->restore(cpu);
    }

    install_user_testcase(cpu, test);
    return step_to_fetch_at_without_compare(cpu, test.start_at, setup_bootstrap_max_steps);
}

bool setup_and_run_one_backend(cpu6502_bridge::ICpu& cpu, const testcase& test)
{
    if (try_soft_setup_and_run(cpu, test)) {
        return true;
    }

    SavedMemoryPatch hard_restart_patch{};
    if (!hard_restart_to_safe_fetch(cpu, hard_restart_patch)) {
        hard_restart_patch.restore(cpu);
        return false;
    }

    const bool setup_completed = try_soft_setup_and_run(cpu, test, &hard_restart_patch);
    hard_restart_patch.restore(cpu);
    return setup_completed;
}

CpuTraceEntry capture_trace(const cpu6502_bridge::ICpu& cpu,
                            std::size_t cycle)
{
    return CpuTraceEntry{
        cycle,
        cpu.bus_address(),
        cpu.bus_data(),
        cpu.is_write(),
        cpu.is_opcode_fetch(),
        cpu.is_irq_asserted(),
        cpu.is_nmi_asserted(),
        cpu.pc(),
        cpu.a(),
        cpu.x(),
        cpu.y(),
        cpu.s(),
        cpu.p()
    };
}

bool traces_match(const CpuTraceEntry& left,
                  const CpuTraceEntry& right,
                  const CompareOptions& options)
{
    if (options.address && left.address != right.address) {
        return false;
    }
    if (options.data && left.data != right.data) {
        return false;
    }
    if (options.read_write && left.write != right.write) {
        return false;
    }
    if (options.opcode_fetch && left.opcode_fetch != right.opcode_fetch) {
        return false;
    }

    if (options.registers_on_fetch && left.opcode_fetch && right.opcode_fetch) {
        return left.pc == right.pc
            && left.a == right.a
            && left.x == right.x
            && left.y == right.y
            && left.s == right.s
            && left.p == right.p;
    }

    return true;
}

void append_result(LockstepRunResult& destination,
                   const LockstepRunResult& source)
{
    const std::size_t old_size = destination.left_trace.size();
    destination.left_trace.insert(destination.left_trace.end(),
                                  source.left_trace.begin(),
                                  source.left_trace.end());
    destination.right_trace.insert(destination.right_trace.end(),
                                   source.right_trace.begin(),
                                   source.right_trace.end());
    if (!source.passed) {
        destination.passed = false;
    }
    if (!destination.first_mismatch && source.first_mismatch) {
        destination.first_mismatch = old_size + *source.first_mismatch;
    }
}

LockstepRunResult empty_success_result()
{
    return LockstepRunResult{};
}

} // namespace

LockstepRunner::LockstepRunner(std::unique_ptr<cpu6502_bridge::ICpu> left,
                               std::unique_ptr<cpu6502_bridge::ICpu> right)
    : left_(std::move(left)),
      right_(std::move(right))
{
    if (!left_ || !right_) {
        throw std::invalid_argument("LockstepRunner requires two CPU backends");
    }
}

cpu6502_bridge::ICpu& LockstepRunner::left() noexcept { return *left_; }
const cpu6502_bridge::ICpu& LockstepRunner::left() const noexcept { return *left_; }
cpu6502_bridge::ICpu& LockstepRunner::right() noexcept { return *right_; }
const cpu6502_bridge::ICpu& LockstepRunner::right() const noexcept { return *right_; }

bool LockstepRunner::setup_and_run(const LockstepConfig& config)
{
    initialize_cpu_pair_memory(*left_, *right_, config.memory);

    compare_ = config.compare;
    cycle_ = 0u;

    if (!restart_to_reset_fetch(*left_)) {
        return false;
    }
    if (!restart_to_reset_fetch(*right_)) {
        return false;
    }

    return true;
}

bool LockstepRunner::setup_and_run(const testcase& test,
                                   const LockstepConfig& config)
{
    compare_ = config.compare;
    cycle_ = 0u;

    initialize_cpu_pair_memory(*left_, *right_, config.memory);

    if (!setup_and_run_one_backend(*left_, test)) {
        return false;
    }
    if (!setup_and_run_one_backend(*right_, test)) {
        return false;
    }

    return true;
}

LockstepRunResult LockstepRunner::step()
{
    LockstepRunResult result{};
    result.left_trace.push_back(capture_trace(*left_, cycle_));
    result.right_trace.push_back(capture_trace(*right_, cycle_));

    if (!traces_match(result.left_trace.back(), result.right_trace.back(), compare_)) {
        result.passed = false;
        result.first_mismatch = 0u;
        return result;
    }

    left_->step();
    right_->step();
    ++cycle_;
    return result;
}

LockstepRunResult LockstepRunner::step_cycles(std::size_t count)
{
    LockstepRunResult result{};
    for (std::size_t index = 0u; index < count; ++index) {
        const auto one = step();
        append_result(result, one);
        if (!one.passed) {
            break;
        }
    }
    return result;
}

LockstepRunResult LockstepRunner::step_to_fetch(std::size_t max_steps)
{
    LockstepRunResult result{};
    bool reached = left_->is_opcode_fetch() && right_->is_opcode_fetch();
    for (std::size_t index = 0u; index < max_steps; ++index) {
        const auto one = step();
        append_result(result, one);
        if (!one.passed) {
            return result;
        }
        reached = left_->is_opcode_fetch() && right_->is_opcode_fetch();
        if (reached) {
            return result;
        }
    }

    if (!reached) {
        result.passed = false;
    }
    return result;
}

LockstepRunResult LockstepRunner::step_to_fetch_at(std::uint16_t address,
                                                   std::size_t max_steps)
{
    LockstepRunResult result{};
    bool reached = cpu_at_fetch_at(*left_, address) && cpu_at_fetch_at(*right_, address);
    for (std::size_t index = 0u; index < max_steps; ++index) {
        const auto one = step();
        append_result(result, one);
        if (!one.passed) {
            return result;
        }
        reached = cpu_at_fetch_at(*left_, address) && cpu_at_fetch_at(*right_, address);
        if (reached) {
            return result;
        }
    }

    if (!reached) {
        result.passed = false;
    }
    return result;
}

LockstepRunResult LockstepRunner::irq(bool asserted)
{
    left_->irq(asserted);
    right_->irq(asserted);
    return empty_success_result();
}

LockstepRunResult LockstepRunner::nmi(bool asserted)
{
    left_->nmi(asserted);
    right_->nmi(asserted);
    return empty_success_result();
}

LockstepScenarioRunner::LockstepScenarioRunner(
    std::unique_ptr<cpu6502_bridge::ICpu> left,
    std::unique_ptr<cpu6502_bridge::ICpu> right)
    : lockstep_(std::move(left), std::move(right))
{
}

bool LockstepScenarioRunner::setup(const testcase& test,
                                   const LockstepConfig& lockstep_config)
{
    test_ = test;
    lockstep_config_ = lockstep_config;
    has_setup_ = true;
    return has_setup_;
}

LockstepScenarioResult LockstepScenarioRunner::restart_run(
    const std::vector<LockstepCommand>& commands,
    const LockstepScenarioConfig& scenario_config)
{
    LockstepScenarioResult scenario_result{};

    if (!has_setup_) {
        scenario_result.passed = false;
        return scenario_result;
    }

    if (!lockstep_.setup_and_run(test_, lockstep_config_)) {
        scenario_result.passed = false;
        return scenario_result;
    }

    scenario_result.results.reserve(commands.size());

    for (std::size_t index = 0u; index < commands.size(); ++index) {
        const auto& command = commands[index];
        LockstepRunResult command_result = std::visit([this](const auto& cmd) -> LockstepRunResult {
            using Command = std::decay_t<decltype(cmd)>;
            if constexpr (std::is_same_v<Command, Step>) {
                return lockstep_.step();
            } else if constexpr (std::is_same_v<Command, StepCycles>) {
                return lockstep_.step_cycles(cmd.count);
            } else if constexpr (std::is_same_v<Command, StepToFetch>) {
                return lockstep_.step_to_fetch(cmd.max_steps);
            } else if constexpr (std::is_same_v<Command, StepToFetchAt>) {
                return lockstep_.step_to_fetch_at(cmd.address, cmd.max_steps);
            } else if constexpr (std::is_same_v<Command, NmiAssert>) {
                return lockstep_.nmi(true);
            } else if constexpr (std::is_same_v<Command, NmiDeassert>) {
                return lockstep_.nmi(false);
            } else if constexpr (std::is_same_v<Command, IrqAssert>) {
                return lockstep_.irq(true);
            } else if constexpr (std::is_same_v<Command, IrqDeassert>) {
                return lockstep_.irq(false);
            }
        }, command);

        if (!command_result.passed && !scenario_result.first_failed_command) {
            scenario_result.passed = false;
            scenario_result.first_failed_command = index;
        }

        scenario_result.results.push_back(std::move(command_result));

        if (!scenario_result.passed && scenario_config.stop_on_failure) {
            break;
        }
    }

    return scenario_result;
}

LockstepRunner& LockstepScenarioRunner::lockstep() noexcept
{
    return lockstep_;
}

const LockstepRunner& LockstepScenarioRunner::lockstep() const noexcept
{
    return lockstep_;
}

} // namespace tools6502
