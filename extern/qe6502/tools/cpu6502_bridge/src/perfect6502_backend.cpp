#include <cpu6502_bridge/cpu.hpp>

#include <cstdint>
#include <cstring>
#include <memory>

extern "C" {
#include <types.h>
#include <perfect6502.h>
#include <netlist_sim.h>
}

namespace cpu6502_bridge {
namespace {

constexpr nodenum_t perfect_clk0 = 1171;
constexpr nodenum_t perfect_irq = 103;
constexpr nodenum_t perfect_nmi = 1297;
constexpr nodenum_t perfect_sync = 539;

bool next_step_is_memory_half(state_t* state) noexcept
{
    return isNodeHigh(state, perfect_clk0) == 0;
}

class Perfect6502Cpu final : public ICpu {
public:
    Perfect6502Cpu() noexcept
    {
        std::memset(::memory, 0, 65536u);
        restart();
    }

    ~Perfect6502Cpu() override
    {
        if (state_ != nullptr) {
            destroyChip(state_);
        }
    }

    Perfect6502Cpu(const Perfect6502Cpu&) = delete;
    Perfect6502Cpu& operator=(const Perfect6502Cpu&) = delete;

    const char* get_name() const noexcept override { return "perfect6502"; }

    void restart() noexcept override
    {
        if (state_ != nullptr) {
            destroyChip(state_);
            state_ = nullptr;
        }

        state_ = initAndResetChip();
        irq_asserted_ = false;
        nmi_asserted_ = false;
        clear_snapshot();
    }

    void irq(bool assert_irq) noexcept override
    {
        irq_asserted_ = assert_irq;
        if (state_ != nullptr) {
            setNode(state_, perfect_irq, assert_irq ? 0 : 1);
            recalcNodeList(state_);
        }
    }

    bool is_irq_asserted() const noexcept override
    {
        return irq_asserted_;
    }

    void nmi(bool assert_nmi) noexcept override
    {
        nmi_asserted_ = assert_nmi;
        if (state_ != nullptr) {
            setNode(state_, perfect_nmi, assert_nmi ? 0 : 1);
            recalcNodeList(state_);
        }
    }

    bool is_nmi_asserted() const noexcept override
    {
        return nmi_asserted_;
    }

    void set_bus_data(std::uint8_t data) noexcept override
    {
        if (!snapshot_.write) {
            ::memory[snapshot_.address] = data;
            snapshot_.data = data;
        }
    }

    void step() noexcept override
    {
        if (state_ == nullptr) {
            return;
        }

        step_until_cpu_half_completed();
        capture_request_snapshot_after_cpu_half();
        step_one_half(); /* memory/bus half-step; services the visible bus request */
        capture_bus_data_after_memory_half();
    }

    std::uint16_t bus_address() const noexcept override
    {
        return snapshot_.address;
    }

    std::uint8_t bus_data() const noexcept override
    {
        return snapshot_.data;
    }

    std::uint8_t* memory() noexcept override
    {
        return ::memory;
    }

    bool is_write() const noexcept override
    {
        return snapshot_.write;
    }

    bool is_opcode_fetch() const noexcept override
    {
        return snapshot_.opcode_fetch;
    }

    std::uint16_t pc() const noexcept override { return snapshot_.pc; }
    std::uint8_t s() const noexcept override { return snapshot_.s; }
    std::uint8_t a() const noexcept override { return snapshot_.a; }
    std::uint8_t x() const noexcept override { return snapshot_.x; }
    std::uint8_t y() const noexcept override { return snapshot_.y; }
    std::uint8_t p() const noexcept override { return snapshot_.p; }

private:
    struct Snapshot {
        std::uint16_t address = 0;
        std::uint8_t data = 0;
        bool write = false;
        bool opcode_fetch = false;
        std::uint16_t pc = 0;
        std::uint8_t s = 0;
        std::uint8_t a = 0;
        std::uint8_t x = 0;
        std::uint8_t y = 0;
        std::uint8_t p = 0;
    };

    void step_one_half() noexcept
    {
        ::step(state_);
    }

    bool step_one_half_and_report_cpu_half() noexcept
    {
        const bool cpu_half = !next_step_is_memory_half(state_);
        step_one_half();
        return cpu_half;
    }

    void step_until_cpu_half_completed() noexcept
    {
        while (!step_one_half_and_report_cpu_half()) {
        }
    }

    void capture_request_snapshot_after_cpu_half() noexcept
    {
        snapshot_.address = readAddressBus(state_);
        snapshot_.write = readRW(state_) == 0u;
        snapshot_.opcode_fetch = !snapshot_.write && isNodeHigh(state_, perfect_sync) != 0;
        snapshot_.pc = readPC(state_);
        snapshot_.s = readSP(state_);
        snapshot_.a = readA(state_);
        snapshot_.x = readX(state_);
        snapshot_.y = readY(state_);
        snapshot_.p = readP(state_);
        snapshot_.data = 0;
    }

    void capture_bus_data_after_memory_half() noexcept
    {
        snapshot_.data = readDataBus(state_);
    }

    void clear_snapshot() noexcept
    {
        snapshot_ = Snapshot{};
    }

    state_t* state_ = nullptr;
    Snapshot snapshot_{};
    bool irq_asserted_ = false;
    bool nmi_asserted_ = false;
};

} // namespace

std::unique_ptr<ICpu> make_perfect6502_cpu()
{
    return std::make_unique<Perfect6502Cpu>();
}

} // namespace cpu6502_bridge
