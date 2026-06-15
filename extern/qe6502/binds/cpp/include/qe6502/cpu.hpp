#ifndef QE6502_CPP_CPU_HPP
#define QE6502_CPP_CPU_HPP

#include <qe6502/qe6502.h>

#include <cstdint>
#include <string>
#include <vector>

#if defined(QE6502_CPP_SHARED)
#    if defined(_WIN32)
#        if defined(QE6502_CPP_EMIT_IMPL)
#            define QE6502_CPP_API __declspec(dllexport)
#        else
#            define QE6502_CPP_API __declspec(dllimport)
#        endif
#    elif defined(__GNUC__) || defined(__clang__)
#        if defined(QE6502_CPP_EMIT_IMPL)
#            define QE6502_CPP_API __attribute__((visibility("default")))
#        else
#            define QE6502_CPP_API
#        endif
#    else
#        define QE6502_CPP_API
#    endif
#else
#    define QE6502_CPP_API
#endif

namespace qe6502 {

enum class model : std::uint8_t {
    nmos = qe6502_model_nmos,
    nes  = qe6502_model_nes,
    wdc  = qe6502_model_wdc,
    rw   = qe6502_model_rw,
    st   = qe6502_model_st,
};

inline constexpr std::uint8_t flag_c  = qe6502_flag_C;
inline constexpr std::uint8_t flag_z  = qe6502_flag_Z;
inline constexpr std::uint8_t flag_i  = qe6502_flag_I;
inline constexpr std::uint8_t flag_d  = qe6502_flag_D;
inline constexpr std::uint8_t flag_b  = qe6502_flag_B;
inline constexpr std::uint8_t flag_un = qe6502_flag_UN;
inline constexpr std::uint8_t flag_v  = qe6502_flag_V;
inline constexpr std::uint8_t flag_n  = qe6502_flag_N;

using cpu_snapshot = std::vector<std::uint8_t>;

class QE6502_CPP_API cpu {
public:
    explicit cpu(model cpu_model = model::nmos) noexcept;
    explicit cpu(const cpu_snapshot& snapshot);

    void restart() noexcept;
    void jump_to(std::uint16_t address) noexcept;

    void irq(bool assert_irq) noexcept;
    bool is_irq_asserted() const noexcept;
    void nmi(bool assert_nmi) noexcept;
    bool is_nmi_asserted() const noexcept;

    const qe6502_tick_t& tick(std::uint8_t input = 0) noexcept;

    std::uint16_t bus_address() const noexcept;
    std::uint8_t bus_data() const noexcept;
    std::uint8_t bus_status() const noexcept;

    bool is_write() const noexcept;
    bool is_opcode_fetch() const noexcept;
    bool is_jammed() const noexcept;

    cpu_snapshot save() const;
    const qe6502_tick_t& load(const cpu_snapshot& value);

    model cpu_model() const noexcept;

    std::uint16_t pc() const noexcept;
    void pc(std::uint16_t value) noexcept;

    std::uint8_t s() const noexcept;
    void s(std::uint8_t value) noexcept;

    std::uint8_t a() const noexcept;
    void a(std::uint8_t value) noexcept;

    std::uint8_t x() const noexcept;
    void x(std::uint8_t value) noexcept;

    std::uint8_t y() const noexcept;
    void y(std::uint8_t value) noexcept;

    std::uint8_t p() const noexcept;
    void p(std::uint8_t value) noexcept;

    std::string to_string() const;

    qe6502_t& raw_cpu() noexcept;
    const qe6502_t& raw_cpu() const noexcept;

    qe6502_tick_t& raw_tick() noexcept;
    const qe6502_tick_t& raw_tick() const noexcept;

private:
    qe6502_t cpu_{};
    qe6502_tick_t tick_{};
};

#if defined(QE6502_CPP_SHARED) && !defined(QE6502_CPP_EMIT_IMPL)

/*
 * Shared C++ consumer mode.
 *
 * Do not expose header implementations from this header. The function
 * definitions are provided by the qe6502_cpp shared library.
 */

#else

#    if defined(QE6502_CPP_SHARED)
#        define QE6502_CPP_IMPL
#    else
#        define QE6502_CPP_IMPL inline
#    endif

QE6502_CPP_IMPL const qe6502_tick_t& cpu::tick(std::uint8_t input) noexcept
{
    tick_ = qe6502_tick(&cpu_, input);
    return tick_;
}

QE6502_CPP_IMPL std::uint16_t cpu::bus_address() const noexcept
{
    return tick_.address;
}

QE6502_CPP_IMPL std::uint8_t cpu::bus_data() const noexcept
{
    return tick_.bus;
}

QE6502_CPP_IMPL std::uint8_t cpu::bus_status() const noexcept
{
    return tick_.status;
}

QE6502_CPP_IMPL bool cpu::is_write() const noexcept
{
    return qe6502_is_write(tick_) != 0u;
}

QE6502_CPP_IMPL bool cpu::is_opcode_fetch() const noexcept
{
    return qe6502_is_fetch(tick_) != 0u;
}

QE6502_CPP_IMPL bool cpu::is_jammed() const noexcept
{
    return qe6502_is_jammed(tick_) != 0u;
}

#    undef QE6502_CPP_IMPL

#endif

} // namespace qe6502

#undef QE6502_CPP_API

#endif // QE6502_CPP_CPU_HPP
