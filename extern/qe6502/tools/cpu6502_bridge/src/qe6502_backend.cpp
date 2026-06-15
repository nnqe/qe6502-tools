#include <cpu6502_bridge/cpu.hpp>

#include <qe6502/qe6502.h>

#include <array>
#include <memory>

namespace cpu6502_bridge {
namespace {

class Qe6502Cpu final : public ICpu {
public:
    Qe6502Cpu() noexcept
        : cpu_{qe6502_setup(qe6502_model_nmos)}
    {
        restart();
    }

    const char* get_name() const noexcept override { return "qe6502"; }

    void restart() noexcept override
    {
        tick_ = qe6502_restart(&cpu_);
        while(tick_.status & qe6502_status_internal_reset){
            step();
        }
    }

    void irq(bool assert_irq) noexcept override
    {
        qe6502_irq_assert(&cpu_, assert_irq ? 1u : 0u);
    }

    bool is_irq_asserted() const noexcept override
    {
        return qe6502_is_irq_asserted(&cpu_) != 0u;
    }

    void nmi(bool assert_nmi) noexcept override
    {
        qe6502_nmi_assert(&cpu_, assert_nmi ? 1u : 0u);
    }

    bool is_nmi_asserted() const noexcept override
    {
        return qe6502_is_nmi_asserted(&cpu_) != 0u;
    }

    void set_bus_data(std::uint8_t data) noexcept override
    {
        if (!is_write()) {
            memory_[bus_address()] = data;
        }
    }

    void step() noexcept override
    {
        const std::uint16_t address = tick_.address;
        const std::uint8_t input = memory_[address];

        if (is_write()) {
            memory_[address] = tick_.bus;
        }

        tick_ = qe6502_tick(&cpu_, input);
    }

    std::uint16_t bus_address() const noexcept override { return tick_.address; }

    std::uint8_t bus_data() const noexcept override
    {
        return is_write() ? tick_.bus : memory_[tick_.address];
    }

    std::uint8_t* memory() noexcept override { return memory_.data(); }

    bool is_write() const noexcept override { return (tick_.status & qe6502_status_writing) != 0u; }
    bool is_opcode_fetch() const noexcept override { return (tick_.status & qe6502_status_opcode_fetch) != 0u; }

    std::uint16_t pc() const noexcept override { return cpu_.PC; }
    std::uint8_t s() const noexcept override { return cpu_.S; }
    std::uint8_t a() const noexcept override { return cpu_.A; }
    std::uint8_t x() const noexcept override { return cpu_.X; }
    std::uint8_t y() const noexcept override { return cpu_.Y; }
    std::uint8_t p() const noexcept override { return cpu_.P; }

private:
    qe6502_t cpu_;
    qe6502_tick_t tick_{};
    std::array<std::uint8_t, 65536> memory_{};
};

} // namespace

std::unique_ptr<ICpu> make_qe6502_cpu()
{
    return std::make_unique<Qe6502Cpu>();
}

} // namespace cpu6502_bridge
