#if defined(QE6502_CPP_SHARED)
#    define QE6502_CPP_EMIT_IMPL 1
#endif

#include <qe6502/cpu.hpp>

#if defined(QE6502_CPP_EMIT_IMPL)
#    undef QE6502_CPP_EMIT_IMPL
#endif

#include <cstdio>
#include <stdexcept>

namespace qe6502 {

cpu::cpu(model cpu_model) noexcept
    : cpu_{qe6502_setup(static_cast<std::uint32_t>(cpu_model))}
    , tick_{}
{
}

cpu::cpu(const cpu_snapshot& snapshot)
    : cpu{}
{
    load(snapshot);
}

void cpu::restart() noexcept
{
    tick_ = qe6502_restart(&cpu_);
}

void cpu::jump_to(std::uint16_t address) noexcept
{
    tick_ = qe6502_goto(&cpu_, address);
}

void cpu::irq(bool assert_irq) noexcept
{
    qe6502_irq_assert(&cpu_, assert_irq ? 1 : 0);
}

bool cpu::is_irq_asserted() const noexcept
{
    return qe6502_is_irq_asserted(&cpu_) != 0;
}

void cpu::nmi(bool assert_nmi) noexcept
{
    qe6502_nmi_assert(&cpu_, assert_nmi ? 1 : 0);
}

bool cpu::is_nmi_asserted() const noexcept
{
    return qe6502_is_nmi_asserted(&cpu_) != 0;
}

cpu_snapshot cpu::save() const
{
    cpu_snapshot snapshot(QE6502_SNAPSHOT_SIZE);
    qe6502_save(&cpu_, tick_, snapshot.data());
    return snapshot;
}

const qe6502_tick_t& cpu::load(const cpu_snapshot& value)
{
    if (value.size() != QE6502_SNAPSHOT_SIZE) {
        throw std::invalid_argument("qe6502 snapshot has invalid size");
    }

    tick_ = qe6502_load(&cpu_, value.data());
    return tick_;
}

model cpu::cpu_model() const noexcept
{
    return static_cast<model>(qe6502_get_model(&cpu_));
}

std::uint16_t cpu::pc() const noexcept
{
    return qe6502_get_pc(&cpu_);
}

void cpu::pc(std::uint16_t value) noexcept
{
    qe6502_set_pc(&cpu_, value);
}

std::uint8_t cpu::s() const noexcept
{
    return qe6502_get_s(&cpu_);
}

void cpu::s(std::uint8_t value) noexcept
{
    qe6502_set_s(&cpu_, value);
}

std::uint8_t cpu::a() const noexcept
{
    return qe6502_get_a(&cpu_);
}

void cpu::a(std::uint8_t value) noexcept
{
    qe6502_set_a(&cpu_, value);
}

std::uint8_t cpu::x() const noexcept
{
    return qe6502_get_x(&cpu_);
}

void cpu::x(std::uint8_t value) noexcept
{
    qe6502_set_x(&cpu_, value);
}

std::uint8_t cpu::y() const noexcept
{
    return qe6502_get_y(&cpu_);
}

void cpu::y(std::uint8_t value) noexcept
{
    qe6502_set_y(&cpu_, value);
}

std::uint8_t cpu::p() const noexcept
{
    return qe6502_get_p(&cpu_);
}

void cpu::p(std::uint8_t value) noexcept
{
    qe6502_set_p(&cpu_, value);
}

std::string cpu::to_string() const
{
    char buffer[160];
    const char rw = is_write() ? 'W' : 'R';
    const int count = std::snprintf(buffer,
                                    sizeof(buffer),
                                    "PC=%04X A=%02X X=%02X Y=%02X S=%02X P=%02X BUS=%c %04X DATA=%02X STATUS=%02X",
                                    static_cast<unsigned>(pc()),
                                    static_cast<unsigned>(a()),
                                    static_cast<unsigned>(x()),
                                    static_cast<unsigned>(y()),
                                    static_cast<unsigned>(s()),
                                    static_cast<unsigned>(p()),
                                    rw,
                                    static_cast<unsigned>(bus_address()),
                                    static_cast<unsigned>(bus_data()),
                                    static_cast<unsigned>(bus_status()));

    if (count <= 0) {
        return {};
    }

    const auto length = static_cast<std::size_t>(count);
    if (length < sizeof(buffer)) {
        return std::string(buffer, length);
    }

    return std::string(buffer);
}

qe6502_t& cpu::raw_cpu() noexcept
{
    return cpu_;
}

const qe6502_t& cpu::raw_cpu() const noexcept
{
    return cpu_;
}

qe6502_tick_t& cpu::raw_tick() noexcept
{
    return tick_;
}

const qe6502_tick_t& cpu::raw_tick() const noexcept
{
    return tick_;
}

} // namespace qe6502
