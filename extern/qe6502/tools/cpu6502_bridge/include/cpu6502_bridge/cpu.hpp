#ifndef CPU6502_BRIDGE_CPU_HPP
#define CPU6502_BRIDGE_CPU_HPP

#include <cstdint>
#include <memory>
#include <string>

namespace cpu6502_bridge {

class ICpu
{
public:
    virtual ~ICpu() = default;

    virtual const char* get_name() const noexcept = 0;

    void clear_memory(uint8_t value) noexcept;
    virtual void restart() noexcept = 0;

    // Performs restart sequencing and stops when the first real opcode fetch
    // from the reset-vector start address is visible on the bus.
    virtual unsigned restart_to_start_fetch(unsigned max_steps = 64);

    virtual void irq(bool assert_irq) noexcept = 0;
    virtual bool is_irq_asserted() const noexcept = 0;
    virtual void nmi(bool assert_nmi) noexcept = 0;
    virtual bool is_nmi_asserted() const noexcept = 0;
    virtual void set_bus_data(std::uint8_t data) noexcept = 0;
    virtual void step() noexcept = 0;
    virtual std::uint16_t bus_address() const noexcept = 0;
    virtual std::uint8_t bus_data() const noexcept = 0;
    virtual std::uint8_t* memory() noexcept = 0;
    virtual bool is_write() const noexcept = 0;
    virtual bool is_opcode_fetch() const noexcept = 0;

    virtual std::uint16_t pc() const noexcept = 0;
    virtual std::uint8_t s() const noexcept = 0;
    virtual std::uint8_t a() const noexcept = 0;
    virtual std::uint8_t x() const noexcept = 0;
    virtual std::uint8_t y() const noexcept = 0;
    virtual std::uint8_t p() const noexcept = 0;

    std::string to_string() const;
};

std::unique_ptr<ICpu> make_qe6502_cpu();
std::unique_ptr<ICpu> make_perfect6502_cpu();

} // namespace cpu6502_bridge

#endif // CPU6502_BRIDGE_CPU_HPP
