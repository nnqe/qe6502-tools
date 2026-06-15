#include <cpu6502_bridge/cpu.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

using cpu6502_bridge::ICpu;

struct FetchLog {
    std::uint64_t cycle = 0;
    std::uint16_t bus_address = 0;
    std::uint8_t bus_data = 0;
    bool is_write = false;
    std::uint16_t pc = 0;
    std::uint8_t a = 0;
    std::uint8_t x = 0;
    std::uint8_t y = 0;
    std::uint8_t s = 0;
    std::uint8_t p = 0;
};

struct RunResult {
    std::vector<FetchLog> fetches;
    std::size_t first_program_fetch = 0;
    std::uint8_t result = 0;
    std::uint16_t final_pc = 0;
    std::uint8_t final_a = 0;
    std::uint8_t final_x = 0;
    std::uint8_t final_y = 0;
    std::uint8_t final_s = 0;
    std::uint8_t final_p = 0;
    std::uint64_t stopped_cycle = 0;
    bool stopped_at_brk = false;
};

static void load_program(ICpu& cpu)
{
    auto* m = cpu.memory();
    std::fill(m, m + 65536u, static_cast<std::uint8_t>(0xea)); // NOP-fill for easier debugging.

    constexpr std::uint16_t base = 0x0400;
    const std::uint8_t program[] = {
        0xa2, 0x03,       // LDX #$03
        0xa9, 0x00,       // LDA #$00
        0x18,             // CLC
        0x69, 0x01,       // ADC #$01
        0xca,             // DEX
        0xd0, 0xfb,       // BNE $0405
        0x8d, 0x00, 0x02, // STA $0200
        0x00              // BRK - stop before executing it
    };

    for (std::size_t i = 0; i < sizeof(program); ++i) {
        m[base + i] = program[i];
    }

    m[0xfffc] = static_cast<std::uint8_t>(base & 0xffu);
    m[0xfffd] = static_cast<std::uint8_t>(base >> 8u);
}


struct StepLog {
    std::uint64_t step = 0;
    std::uint16_t bus_address = 0;
    std::uint8_t bus_data = 0;
    bool is_write = false;
    bool is_fetch = false;
    std::uint16_t pc = 0;
    std::uint8_t a = 0;
    std::uint8_t x = 0;
    std::uint8_t y = 0;
    std::uint8_t s = 0;
    std::uint8_t p = 0;
};

static StepLog snapshot_step(ICpu& cpu, std::uint64_t step)
{
    StepLog log;
    log.step = step;
    log.bus_address = cpu.bus_address();
    log.bus_data = cpu.bus_data();
    log.is_write = cpu.is_write();
    log.is_fetch = cpu.is_opcode_fetch();
    log.pc = cpu.pc();
    log.a = cpu.a();
    log.x = cpu.x();
    log.y = cpu.y();
    log.s = cpu.s();
    log.p = cpu.p();
    return log;
}

static void print_step_log(const StepLog& f)
{
    std::printf("step=%3llu %s bus=%c %04X data=%02X PC=%04X A=%02X X=%02X Y=%02X S=%02X P=%02X\n",
                static_cast<unsigned long long>(f.step),
                f.is_fetch ? "FETCH" : "     ",
                f.is_write ? 'W' : 'R',
                static_cast<unsigned>(f.bus_address),
                static_cast<unsigned>(f.bus_data),
                static_cast<unsigned>(f.pc),
                static_cast<unsigned>(f.a),
                static_cast<unsigned>(f.x),
                static_cast<unsigned>(f.y),
                static_cast<unsigned>(f.s),
                static_cast<unsigned>(f.p));
}

static void dump_restart_steps(const char* name, std::unique_ptr<ICpu> cpu)
{
    load_program(*cpu);
    cpu->restart();

    std::printf("\n[%s restart step trace]\n", name);
    std::printf("Each line is the visible bus/register state before calling ICpu::step().\n");

    for (std::uint64_t step = 0; step < 16; ++step) {
        const auto log = snapshot_step(*cpu, step);
        print_step_log(log);
        cpu->step();
    }
}

static RunResult run_cpu(const char* name, std::unique_ptr<ICpu> cpu)
{
    load_program(*cpu);
    cpu->restart();

    RunResult out;
    out.first_program_fetch = static_cast<std::size_t>(-1);

    for (std::uint64_t cycle = 0; cycle < 1000; ++cycle) {
        if (cpu->is_opcode_fetch()) {
            FetchLog log;
            log.cycle = cycle;
            log.bus_address = cpu->bus_address();
            log.bus_data = cpu->bus_data();
            log.is_write = cpu->is_write();
            log.pc = cpu->pc();
            log.a = cpu->a();
            log.x = cpu->x();
            log.y = cpu->y();
            log.s = cpu->s();
            log.p = cpu->p();

            if (out.first_program_fetch == static_cast<std::size_t>(-1)
                && log.bus_address == 0x0400
                && log.bus_data == 0xa2) {
                out.first_program_fetch = out.fetches.size();
            }

            out.fetches.push_back(log);

            if (out.first_program_fetch != static_cast<std::size_t>(-1)
                && log.bus_address == 0x040d
                && log.bus_data == 0x00) {
                out.stopped_cycle = cycle;
                out.stopped_at_brk = true;
                break;
            }
        }

        cpu->step();
    }

    out.result = cpu->memory()[0x0200];
    out.final_pc = cpu->pc();
    out.final_a = cpu->a();
    out.final_x = cpu->x();
    out.final_y = cpu->y();
    out.final_s = cpu->s();
    out.final_p = cpu->p();

    std::printf("\n[%s raw fetch log]\n", name);
    for (const auto& f : out.fetches) {
        std::printf("cycle=%3llu fetch bus=%c %04X data=%02X PC=%04X A=%02X X=%02X Y=%02X S=%02X P=%02X\n",
                    static_cast<unsigned long long>(f.cycle),
                    f.is_write ? 'W' : 'R',
                    static_cast<unsigned>(f.bus_address),
                    static_cast<unsigned>(f.bus_data),
                    static_cast<unsigned>(f.pc),
                    static_cast<unsigned>(f.a),
                    static_cast<unsigned>(f.x),
                    static_cast<unsigned>(f.y),
                    static_cast<unsigned>(f.s),
                    static_cast<unsigned>(f.p));
    }
    std::printf("stopped=%s cycle=%llu result[$0200]=%02X final PC=%04X A=%02X X=%02X Y=%02X S=%02X P=%02X\n",
                out.stopped_at_brk ? "yes" : "no",
                static_cast<unsigned long long>(out.stopped_cycle),
                static_cast<unsigned>(out.result),
                static_cast<unsigned>(out.final_pc),
                static_cast<unsigned>(out.final_a),
                static_cast<unsigned>(out.final_x),
                static_cast<unsigned>(out.final_y),
                static_cast<unsigned>(out.final_s),
                static_cast<unsigned>(out.final_p));

    return out;
}

static bool same_program_fetch_bus(const FetchLog& a, const FetchLog& b,
                                   std::uint64_t a_base_cycle,
                                   std::uint64_t b_base_cycle)
{
    return (a.cycle - a_base_cycle) == (b.cycle - b_base_cycle)
        && a.bus_address == b.bus_address
        && a.bus_data == b.bus_data
        && a.is_write == b.is_write;
}

int main()
{
    dump_restart_steps("qe6502", cpu6502_bridge::make_qe6502_cpu());
    dump_restart_steps("perfect6502", cpu6502_bridge::make_perfect6502_cpu());

    auto qe = run_cpu("qe6502", cpu6502_bridge::make_qe6502_cpu());
    auto pf = run_cpu("perfect6502", cpu6502_bridge::make_perfect6502_cpu());

    bool ok = qe.stopped_at_brk && pf.stopped_at_brk;

    if (qe.first_program_fetch == static_cast<std::size_t>(-1)
        || pf.first_program_fetch == static_cast<std::size_t>(-1)) {
        std::printf("\nCould not find the first program fetch at $0400.\n");
        std::printf("COMPARE: FAIL\n");
        return 1;
    }

    const auto qe_begin = qe.fetches.begin() + static_cast<std::ptrdiff_t>(qe.first_program_fetch);
    const auto pf_begin = pf.fetches.begin() + static_cast<std::ptrdiff_t>(pf.first_program_fetch);
    const std::size_t qe_count = static_cast<std::size_t>(qe.fetches.end() - qe_begin);
    const std::size_t pf_count = static_cast<std::size_t>(pf.fetches.end() - pf_begin);
    const std::uint64_t qe_base_cycle = qe_begin->cycle;
    const std::uint64_t pf_base_cycle = pf_begin->cycle;

    std::printf("\n[normalized program fetch bus comparison]\n");
    std::printf("qe6502 first program fetch cycle=%llu; perfect6502 first program fetch cycle=%llu; offset=%lld\n",
                static_cast<unsigned long long>(qe_base_cycle),
                static_cast<unsigned long long>(pf_base_cycle),
                static_cast<long long>(pf_base_cycle) - static_cast<long long>(qe_base_cycle));

    if (qe_count != pf_count) {
        std::printf("FETCH COUNT MISMATCH after program start: qe6502=%zu perfect6502=%zu\n", qe_count, pf_count);
        ok = false;
    }

    const std::size_t n = std::min(qe_count, pf_count);
    for (std::size_t i = 0; i < n; ++i) {
        const auto& q = *(qe_begin + static_cast<std::ptrdiff_t>(i));
        const auto& p = *(pf_begin + static_cast<std::ptrdiff_t>(i));
        const bool same = same_program_fetch_bus(q, p, qe_base_cycle, pf_base_cycle);
        std::printf("#%02zu rel_cycle=%3llu bus=%c %04X data=%02X  %s\n",
                    i,
                    static_cast<unsigned long long>(q.cycle - qe_base_cycle),
                    q.is_write ? 'W' : 'R',
                    static_cast<unsigned>(q.bus_address),
                    static_cast<unsigned>(q.bus_data),
                    same ? "OK" : "MISMATCH");
        if (!same) {
            std::printf("    qe: abs_cycle=%llu bus=%c %04X data=%02X\n",
                        static_cast<unsigned long long>(q.cycle), q.is_write ? 'W' : 'R', q.bus_address, q.bus_data);
            std::printf("    pf: abs_cycle=%llu bus=%c %04X data=%02X\n",
                        static_cast<unsigned long long>(p.cycle), p.is_write ? 'W' : 'R', p.bus_address, p.bus_data);
            ok = false;
        }
    }

    if (qe.result != pf.result) {
        std::printf("\nRESULT MISMATCH: qe6502=%02X perfect6502=%02X\n", qe.result, pf.result);
        ok = false;
    }

    // perfect6502 reports bit 4 set in P here; mask it for the arithmetic result check.
    const bool same_final = qe.final_pc == pf.final_pc
        && qe.final_a == pf.final_a
        && qe.final_x == pf.final_x
        && qe.final_y == pf.final_y
        && qe.final_s == pf.final_s
        && (qe.final_p & 0xefu) == (pf.final_p & 0xefu);
    if (!same_final) {
        std::printf("\nFINAL REGISTER MISMATCH after masking P bit 4\n");
        ok = false;
    }

    std::printf("\nCOMPARE: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
