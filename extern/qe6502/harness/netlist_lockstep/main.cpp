#include <qe6502/cpu.hpp>

#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <vector>


extern "C" {
#include <types.h>
#include <perfect6502.h>
#include <netlist_sim.h>
}

namespace {

struct options {
    std::uint32_t seconds = 0;
    std::uint32_t seed = 0;
    bool seed_provided = false;
    std::uint8_t forced_opcode = 0;
    bool forced_opcode_provided = false;
    bool verbose = false;
};

struct problem_tracker {
    bool seen = false;
    int remaining_iterations = -1;
};

struct rng32 {
    std::uint32_t state = 0;

    explicit rng32(std::uint32_t seed) noexcept
        : state(seed)
    {
    }

    std::uint32_t next_u32() noexcept
    {
        state = state * UINT32_C(1664525) + UINT32_C(1013904223);
        return state;
    }

    std::uint8_t next_u8() noexcept
    {
        return static_cast<std::uint8_t>(next_u32() >> 24u);
    }
};

struct bus_request {
    bool write = false;
    std::uint16_t address = 0;
    std::uint8_t data = 0;
};

struct register_state {
    std::uint16_t pc = 0;
    std::uint8_t a = 0;
    std::uint8_t x = 0;
    std::uint8_t y = 0;
    std::uint8_t s = 0;
    std::uint8_t p = 0;
};

struct counters {
    std::uint64_t cycles = 0;
    std::uint64_t instructions = 0;
    std::uint8_t last_opcode = 0;
    bool have_opcode = false;
};

struct pending_register_compare {
    bool valid = false;
    register_state qe_regs{};
    std::uint64_t cycle = 0;
    std::uint64_t instruction = 0;
    std::uint8_t opcode = 0;
    bool have_opcode = false;
};

void print_usage(const char* argv0)
{
    std::fprintf(stderr,
        "usage: %s <seconds> [seed] [opcode] [--verbose]\n"
        "  seconds: positive integer runtime in wall-clock seconds\n"
        "  seed:    optional unsigned 32-bit integer, decimal or 0x-prefixed\n"
        "  opcode:  optional forced opcode byte, decimal or 0x-prefixed\n"
        "  --verbose, -v: dump each lockstep while-iteration and continue 8 iterations after the first problem\n",
        argv0);
}

bool parse_u32(const char* text, std::uint32_t& out)
{
    if (text == nullptr || *text == '\0') {
        return false;
    }

    char* end = nullptr;
    const unsigned long long value = std::strtoull(text, &end, 0);
    if (end == nullptr || *end != '\0' || value > std::numeric_limits<std::uint32_t>::max()) {
        return false;
    }

    out = static_cast<std::uint32_t>(value);
    return true;
}

bool is_verbose_arg(const char* text)
{
    return std::strcmp(text, "--verbose") == 0 || std::strcmp(text, "-v") == 0;
}

bool parse_args(int argc, char** argv, options& out)
{
    if (argc < 2 || argc > 5) {
        print_usage(argv[0]);
        return false;
    }

    if (!parse_u32(argv[1], out.seconds) || out.seconds == 0u) {
        std::fprintf(stderr, "invalid seconds: %s\n", argv[1]);
        print_usage(argv[0]);
        return false;
    }

    std::vector<const char*> positional{};
    for (int i = 2; i < argc; ++i) {
        if (is_verbose_arg(argv[i])) {
            out.verbose = true;
        } else {
            positional.push_back(argv[i]);
        }
    }

    if (positional.size() > 2u) {
        print_usage(argv[0]);
        return false;
    }

    if (!positional.empty()) {
        if (!parse_u32(positional[0], out.seed)) {
            std::fprintf(stderr, "invalid seed: %s\n", positional[0]);
            print_usage(argv[0]);
            return false;
        }
        out.seed_provided = true;
    }

    if (positional.size() == 2u) {
        std::uint32_t opcode = 0;
        if (!parse_u32(positional[1], opcode) || opcode > 0xFFu) {
            std::fprintf(stderr, "invalid opcode: %s\n", positional[1]);
            print_usage(argv[0]);
            return false;
        }
        out.forced_opcode = static_cast<std::uint8_t>(opcode);
        out.forced_opcode_provided = true;
    }

    return true;
}

std::uint32_t make_time_seed()
{
    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const auto steady = std::chrono::steady_clock::now().time_since_epoch().count();
    std::uint64_t mixed = static_cast<std::uint64_t>(now) ^ (static_cast<std::uint64_t>(steady) << 1u);
    mixed += UINT64_C(0x9e3779b97f4a7c15);
    mixed = (mixed ^ (mixed >> 30u)) * UINT64_C(0xbf58476d1ce4e5b9);
    mixed = (mixed ^ (mixed >> 27u)) * UINT64_C(0x94d049bb133111eb);
    mixed ^= mixed >> 31u;

    rng32 seed_rng{static_cast<std::uint32_t>(mixed ^ (mixed >> 32u))};
    return seed_rng.next_u32();
}

const std::vector<std::uint8_t> escaped_kil_jam_opcodes = {
    /* KIL/JAM opcodes stop the CPU and do not produce a useful fuzz stream. */
    0x02u, 0x12u, 0x22u, 0x32u, 0x42u, 0x52u,
    0x62u, 0x72u, 0x92u, 0xB2u, 0xD2u, 0xF2u,
};

const std::vector<std::uint8_t> escaped_unstable_opcodes = {
    /*
     * Undocumented/unstable opcodes whose final register state is implemented
     * in favor of SingleStepTests/ProcessorTests 6502 compatibility rather
     * than matching perfect6502 in this lockstep oracle:
     * https://github.com/SingleStepTests/ProcessorTests/tree/main/6502
     */
    0x0Bu, 0x2Bu, 0x4Bu, 0x6Bu, 0x8Bu, 0xABu,
    0xBBu,
};

bool contains_opcode(const std::vector<std::uint8_t>& opcodes, std::uint8_t opcode)
{
    return std::find(opcodes.begin(), opcodes.end(), opcode) != opcodes.end();
}

bool is_escaped_opcode(std::uint8_t opcode)
{
    return contains_opcode(escaped_kil_jam_opcodes, opcode)
        || contains_opcode(escaped_unstable_opcodes, opcode);
}

void print_opcode_list(const char* label, const std::vector<std::uint8_t>& opcodes)
{
    std::printf("netlist_lockstep: %s", label);
    for (const std::uint8_t opcode : opcodes) {
        std::printf(" %02X", static_cast<unsigned>(opcode));
    }
    std::printf("\n");
}

void print_test_info()
{
    std::printf("netlist_lockstep: lockstep fuzz test against perfect6502; read cycles get deterministic pseudo-random bytes and bus activity is compared cycle-by-cycle\n");
    print_opcode_list("escaped KIL/JAM opcodes:", escaped_kil_jam_opcodes);
    print_opcode_list("escaped unstable opcodes (qe6502 follows SingleStepTests/ProcessorTests 6502):", escaped_unstable_opcodes);
    std::printf("netlist_lockstep: SingleStepTests/ProcessorTests 6502: https://github.com/SingleStepTests/ProcessorTests/tree/main/6502\n");
}

std::uint8_t next_read_byte(rng32& rng, bool opcode_fetch, const options& opts)
{
    if (opcode_fetch && opts.forced_opcode_provided) {
        return opts.forced_opcode;
    }

    std::uint8_t byte = 0;
    do {
        byte = rng.next_u8();
    } while (opcode_fetch && is_escaped_opcode(byte));
    return byte;
}

bus_request qe_bus(const qe6502::cpu& cpu)
{
    return bus_request{cpu.is_write(), cpu.bus_address(), cpu.bus_data()};
}

bus_request perfect_bus(state_t* state)
{
    const bool reading = readRW(state) != 0u;
    return bus_request{!reading, readAddressBus(state), readDataBus(state)};
}

register_state qe_registers(const qe6502::cpu& cpu)
{
    return register_state{cpu.pc(), cpu.a(), cpu.x(), cpu.y(), cpu.s(), cpu.p()};
}

register_state perfect_registers(state_t* state)
{
    return register_state{
        readPC(state),
        readA(state),
        readX(state),
        readY(state),
        readSP(state),
        readP(state),
    };
}

constexpr nodenum_t perfect_clk0 = 1171;

bool qe_jammed(const qe6502::cpu& qe);
bool perfect_next_step_is_memory_half(state_t* perfect);

const char* rw_text(bool write)
{
    return write ? "W" : "R";
}

void print_bus(const char* name, const bus_request& bus)
{
    std::fprintf(stderr, "%-12s %s addr=0x%04X data=0x%02X\n",
        name,
        rw_text(bus.write),
        static_cast<unsigned>(bus.address),
        static_cast<unsigned>(bus.data));
}

void print_registers(const char* name, const register_state& regs)
{
    std::fprintf(stderr,
        "%-12s PC=0x%04X A=0x%02X X=0x%02X Y=0x%02X S=0x%02X P=0x%02X\n",
        name,
        static_cast<unsigned>(regs.pc),
        static_cast<unsigned>(regs.a),
        static_cast<unsigned>(regs.x),
        static_cast<unsigned>(regs.y),
        static_cast<unsigned>(regs.s),
        static_cast<unsigned>(regs.p));
}

void dump_qe_state(const char* label, const qe6502::cpu& qe)
{
    std::fprintf(stderr, "%s\n", label);
    print_bus("  qe bus:", qe_bus(qe));
    print_registers("  qe regs:", qe_registers(qe));
    std::fprintf(stderr, "  qe status: 0x%02X%s%s\n",
        static_cast<unsigned>(qe.bus_status()),
        qe.is_opcode_fetch() ? " fetch" : "",
        qe_jammed(qe) ? " jammed" : "");
}

void dump_perfect_state(const char* label, state_t* perfect)
{
    std::fprintf(stderr, "%s\n", label);
    std::fprintf(stderr, "  perfect clk0: %u next_step=%s\n",
        static_cast<unsigned>(isNodeHigh(perfect, perfect_clk0) != 0),
        perfect_next_step_is_memory_half(perfect) ? "memory-half" : "cpu-half");
    print_bus("  pf bus:", perfect_bus(perfect));
    print_registers("  pf regs:", perfect_registers(perfect));
}

void mark_first_problem(problem_tracker& problem, const counters& counts, const char* kind)
{
    if (!problem.seen) {
        problem.seen = true;
        problem.remaining_iterations = 8;
        std::fprintf(stderr,
            "\n!!!!!!!!!!!!!!!! FIRST PROBLEM SEEN: %s at cycle=%" PRIu64 " instruction=%" PRIu64 " !!!!!!!!!!!!!!!!\n",
            kind,
            counts.cycles,
            counts.instructions);
    } else {
        std::fprintf(stderr, "\n---- additional problem: %s at cycle=%" PRIu64 " ----\n", kind, counts.cycles);
    }
}

bool finish_verbose_iteration_after_problem(problem_tracker& problem)
{
    if (!problem.seen) {
        return false;
    }
    if (problem.remaining_iterations == 0) {
        return true;
    }
    --problem.remaining_iterations;
    return false;
}

bool report_mismatch(
    const char* field,
    std::uint32_t seed,
    const counters& counts,
    bool fetch_cycle,
    const bus_request& qe,
    const bus_request& perfect)
{
    std::fprintf(stderr, "netlist_lockstep: mismatch\n");
    std::fprintf(stderr, "seed:        0x%08" PRIX32 "\n", seed);
    std::fprintf(stderr, "cycle:       %" PRIu64 "\n", counts.cycles);
    std::fprintf(stderr, "instruction: %" PRIu64 "\n", counts.instructions);
    if (counts.have_opcode) {
        std::fprintf(stderr, "last_opcode: 0x%02X\n", static_cast<unsigned>(counts.last_opcode));
    } else {
        std::fprintf(stderr, "last_opcode: --\n");
    }
    std::fprintf(stderr, "fetch_cycle: %s\n", fetch_cycle ? "yes" : "no");
    std::fprintf(stderr, "field:       %s\n", field);
    print_bus("qe6502:", qe);
    print_bus("perfect6502:", perfect);
    return false;
}

bool report_register_mismatch(
    const char* field,
    std::uint32_t seed,
    const pending_register_compare& pending,
    const register_state& perfect)
{
    std::fprintf(stderr, "netlist_lockstep: register mismatch\n");
    std::fprintf(stderr, "seed:        0x%08" PRIX32 "\n", seed);
    std::fprintf(stderr, "cycle:       %" PRIu64 "\n", pending.cycle);
    std::fprintf(stderr, "instruction: %" PRIu64 "\n", pending.instruction);
    if (pending.have_opcode) {
        std::fprintf(stderr, "last_opcode: 0x%02X\n", static_cast<unsigned>(pending.opcode));
    } else {
        std::fprintf(stderr, "last_opcode: --\n");
    }
    std::fprintf(stderr, "compare_at:  after perfect6502 CPU-half of following fetch\n");
    std::fprintf(stderr, "field:       %s\n", field);
    print_registers("qe6502 saved:", pending.qe_regs);
    print_registers("perfect6502:", perfect);
    return false;
}

bool report_jammed(std::uint32_t seed, const counters& counts, bool fetch_cycle, const bus_request& qe, const bus_request& perfect)
{
    std::fprintf(stderr, "netlist_lockstep: qe6502 jammed\n");
    std::fprintf(stderr, "seed:        0x%08" PRIX32 "\n", seed);
    std::fprintf(stderr, "cycle:       %" PRIu64 "\n", counts.cycles);
    std::fprintf(stderr, "instruction: %" PRIu64 "\n", counts.instructions);
    if (counts.have_opcode) {
        std::fprintf(stderr, "last_opcode: 0x%02X\n", static_cast<unsigned>(counts.last_opcode));
    } else {
        std::fprintf(stderr, "last_opcode: --\n");
    }
    std::fprintf(stderr, "fetch_cycle: %s\n", fetch_cycle ? "yes" : "no");
    print_bus("qe6502:", qe);
    print_bus("perfect6502:", perfect);
    return false;
}

bool qe_jammed(const qe6502::cpu& qe)
{
    return (qe.bus_status() & qe6502_status_cpu_jammed) != 0u;
}

bool compare_bus_request(
    std::uint32_t seed,
    const counters& counts,
    bool fetch_cycle,
    const bus_request& qe,
    const bus_request& perfect)
{
    if (qe.write != perfect.write) {
        return report_mismatch("rw", seed, counts, fetch_cycle, qe, perfect);
    }
    if (qe.address != perfect.address) {
        return report_mismatch("address", seed, counts, fetch_cycle, qe, perfect);
    }
    return true;
}

bool compare_write_data_after_memory_half(
    std::uint32_t seed,
    const counters& counts,
    bool fetch_cycle,
    const bus_request& qe)
{
    const bus_request perfect_write{true, qe.address, memory[qe.address]};
    if (qe.data != perfect_write.data) {
        return report_mismatch("write_data", seed, counts, fetch_cycle, qe, perfect_write);
    }
    return true;
}

bool compare_pending_registers_after_fetch_cpu_half(
    std::uint32_t seed,
    const pending_register_compare& pending,
    state_t* perfect)
{
    if (!pending.valid || !pending.have_opcode) {
        return true;
    }

    const register_state perfect_regs = perfect_registers(perfect);

    /*
     * The compare is intentionally delayed: qe6502 reaches the next fetch and
     * exposes the completed architectural state one tick earlier than the
     * perfect6502 register readers do for some RMW+ALU opcodes. Compare the
     * saved qe6502 snapshot after perfect6502 has consumed the following fetch
     * opcode in its CPU half. PC is not compared here because perfect6502 has
     * already advanced into the next instruction's operand/decode phase.
     */
    if (pending.qe_regs.a != perfect_regs.a) {
        return report_register_mismatch("a", seed, pending, perfect_regs);
    }
    if (pending.qe_regs.x != perfect_regs.x) {
        return report_register_mismatch("x", seed, pending, perfect_regs);
    }
    if (pending.qe_regs.y != perfect_regs.y) {
        return report_register_mismatch("y", seed, pending, perfect_regs);
    }
    if (pending.qe_regs.s != perfect_regs.s) {
        return report_register_mismatch("s", seed, pending, perfect_regs);
    }
    /*
     * The 6502 B flag is not a real status flip-flop, and bit 5 is not backed
     * by a normal status latch either. Compare the real P bits and still print
     * the raw P values if any real status bit differs.
     */
    constexpr std::uint8_t real_p_mask = static_cast<std::uint8_t>(~(qe6502_flag_B | qe6502_flag_UN));
    if ((pending.qe_regs.p & real_p_mask) != (perfect_regs.p & real_p_mask)) {
        return report_register_mismatch("p", seed, pending, perfect_regs);
    }

    return true;
}

constexpr std::uint16_t reset_vector = 0xFFFCu;
constexpr std::uint16_t lockstep_start = 0x0400u;
constexpr std::uint32_t max_reset_half_cycles = 128u;
void init_perfect_memory_for_reset()
{
    std::memset(memory, 0, 65536u);
    memory[reset_vector] = static_cast<std::uint8_t>(lockstep_start & 0x00FFu);
    memory[static_cast<std::uint16_t>(reset_vector + 1u)] = static_cast<std::uint8_t>(lockstep_start >> 8u);
}

bool is_start_fetch_request(const bus_request& bus)
{
    return !bus.write && bus.address == lockstep_start;
}

bool perfect_next_step_is_memory_half(state_t* perfect)
{
    /*
     * perfect6502::step() calls handleMemory() only when the old clk0 value
     * is low. Therefore clk0 == 0 means the netlist has just completed a CPU
     * half-cycle and the next step will service the memory request currently
     * visible on the bus. That is the only phase where this harness can safely
     * inject a read byte through perfect6502::memory[address].
     */
    return isNodeHigh(perfect, perfect_clk0) == 0;
}

bool perfect_is_opcode_fetch_request(state_t* perfect)
{
    const bus_request bus = perfect_bus(perfect);
    const register_state regs = perfect_registers(perfect);
    return perfect_next_step_is_memory_half(perfect) && !bus.write && bus.address == regs.pc;
}

bool align_perfect_to_start_fetch(state_t* perfect)
{
    for (std::uint32_t half_cycle = 1u; half_cycle <= max_reset_half_cycles; ++half_cycle) {
        step(perfect);

        if (perfect_is_opcode_fetch_request(perfect) && is_start_fetch_request(perfect_bus(perfect))) {
            return true;
        }
    }

    std::fprintf(stderr,
        "netlist_lockstep: failed to align perfect6502 to reset fetch at 0x%04X\n",
        static_cast<unsigned>(lockstep_start));
    return false;
}

bool sync_qe_to_perfect_fetch(qe6502::cpu& qe, state_t* perfect)
{
    if (!perfect_is_opcode_fetch_request(perfect)) {
        std::fprintf(stderr, "netlist_lockstep: perfect6502 is not at a syncable opcode-fetch phase\n");
        std::fprintf(stderr, "clk0:        %u\n", static_cast<unsigned>(isNodeHigh(perfect, perfect_clk0) != 0));
        print_bus("perfect6502:", perfect_bus(perfect));
        print_registers("perfect6502:", perfect_registers(perfect));
        return false;
    }

    const register_state regs = perfect_registers(perfect);

    qe.jump_to(regs.pc);
    qe.a(regs.a);
    qe.x(regs.x);
    qe.y(regs.y);
    qe.s(regs.s);
    qe.p(regs.p);
    qe.pc(regs.pc);

    if (!qe.is_opcode_fetch()) {
        std::fprintf(stderr, "netlist_lockstep: qe6502 goto did not produce an opcode-fetch request\n");
        print_bus("qe6502:", qe_bus(qe));
        return false;
    }

    const bus_request qe_current = qe_bus(qe);
    const bus_request perfect_current = perfect_bus(perfect);
    if (qe_current.write != perfect_current.write || qe_current.address != perfect_current.address) {
        std::fprintf(stderr, "netlist_lockstep: post-sync fetch alignment mismatch\n");
        print_bus("qe6502:", qe_current);
        print_bus("perfect6502:", perfect_current);
        return false;
    }

    return true;
}

bool start_lockstep(qe6502::cpu& qe, state_t*& perfect)
{
    init_perfect_memory_for_reset();
    perfect = initAndResetChip();

    if (!align_perfect_to_start_fetch(perfect)) {
        return false;
    }

    return sync_qe_to_perfect_fetch(qe, perfect);
}

bool run_lockstep(const options& opts)
{
    const std::uint32_t seed = opts.seed_provided ? opts.seed : make_time_seed();
    print_test_info();
    if (opts.forced_opcode_provided) {
        std::printf("netlist_lockstep: seed=0x%08" PRIX32 " seconds=%" PRIu32 " opcode=0x%02X\n",
            seed,
            opts.seconds,
            static_cast<unsigned>(opts.forced_opcode));
    } else {
        std::printf("netlist_lockstep: seed=0x%08" PRIX32 " seconds=%" PRIu32 "\n", seed, opts.seconds);
    }
    std::fflush(stdout);

    rng32 rng{seed};
    qe6502::cpu qe{qe6502::model::nmos};
    state_t* perfect = nullptr;

    if (!start_lockstep(qe, perfect)) {
        if (perfect != nullptr) {
            destroyChip(perfect);
        }
        return false;
    }

    counters counts{};
    problem_tracker problem{};
    pending_register_compare pending_registers{};
    const auto end_time = std::chrono::steady_clock::now() + std::chrono::seconds(opts.seconds);

    while (std::chrono::steady_clock::now() < end_time) {
        if (opts.verbose) {
            std::fprintf(stderr,
                "\n--------------- while cycle=%" PRIu64 " instruction=%" PRIu64 " ---------------\n",
                counts.cycles,
                counts.instructions);
        }

        const bool fetch_cycle = qe.is_opcode_fetch();
        const bus_request qe_current = qe_bus(qe);
        const bus_request perfect_current = perfect_bus(perfect);

        if (qe_jammed(qe)) {
            if (opts.verbose) {
                mark_first_problem(problem, counts, "qe6502 jammed before memory activity");
                report_jammed(seed, counts, fetch_cycle, qe_current, perfect_current);
            } else {
                report_jammed(seed, counts, fetch_cycle, qe_current, perfect_current);
                destroyChip(perfect);
                return false;
            }
        }

        if (!compare_bus_request(seed, counts, fetch_cycle, qe_current, perfect_current)) {
            if (opts.verbose) {
                mark_first_problem(problem, counts, "bus request mismatch after CPU-half");
            } else {
                destroyChip(perfect);
                return false;
            }
        }

        std::uint8_t qe_input = qe_current.data;
        bool fetched_opcode = false;
        std::uint8_t fetched_opcode_value = 0;
        if (!qe_current.write) {
            qe_input = next_read_byte(rng, fetch_cycle, opts);
            memory[qe_current.address] = qe_input;
            if (fetch_cycle) {
                fetched_opcode = true;
                fetched_opcode_value = qe_input;
            }
        }

        const bool perfect_step0_is_memory = perfect_next_step_is_memory_half(perfect);
        step(perfect);
        if (opts.verbose) {
            std::fprintf(stderr,
                "after perfect6502 step: %s; serviced request=%s addr=0x%04X",
                perfect_step0_is_memory ? "memory-half" : "cpu-half",
                rw_text(qe_current.write),
                static_cast<unsigned>(qe_current.address));
            if (qe_current.write) {
                std::fprintf(stderr, " data=0x%02X", static_cast<unsigned>(qe_current.data));
            } else {
                std::fprintf(stderr, " feed=0x%02X", static_cast<unsigned>(qe_input));
            }
            std::fprintf(stderr, "\n");
            dump_perfect_state("perfect6502 dump after step", perfect);
        }

        if (qe_current.write
            && !compare_write_data_after_memory_half(seed, counts, fetch_cycle, qe_current)) {
            if (opts.verbose) {
                mark_first_problem(problem, counts, "write data mismatch after memory-half");
            } else {
                destroyChip(perfect);
                return false;
            }
        }


        if (fetched_opcode) {
            counts.last_opcode = fetched_opcode_value;
            counts.have_opcode = true;
            counts.instructions++;
        }

        if (opts.verbose) {
            std::fprintf(stderr,
                "before qe6502 tick: request=%s addr=0x%04X",
                rw_text(qe_current.write),
                static_cast<unsigned>(qe_current.address));
            if (qe_current.write) {
                std::fprintf(stderr, " data=0x%02X\n", static_cast<unsigned>(qe_current.data));
            } else {
                std::fprintf(stderr, " input=0x%02X%s\n",
                    static_cast<unsigned>(qe_input),
                    fetch_cycle ? " fetch" : "");
            }
        }
        qe.tick(qe_input);
        if (opts.verbose) {
            dump_qe_state("qe6502 dump after tick", qe);
        }

        if (qe.is_opcode_fetch()) {
            pending_registers.valid = true;
            pending_registers.qe_regs = qe_registers(qe);
            pending_registers.cycle = counts.cycles;
            pending_registers.instruction = counts.instructions;
            pending_registers.opcode = counts.last_opcode;
            pending_registers.have_opcode = counts.have_opcode;
        }

        const bool perfect_step1_is_memory = perfect_next_step_is_memory_half(perfect);
        step(perfect);
        if (opts.verbose) {
            std::fprintf(stderr,
                "after perfect6502 step: %s\n",
                perfect_step1_is_memory ? "memory-half" : "cpu-half");
            dump_perfect_state("perfect6502 dump after step", perfect);
        }

        if (fetch_cycle && !compare_pending_registers_after_fetch_cpu_half(seed, pending_registers, perfect)) {
            if (opts.verbose) {
                mark_first_problem(problem, counts, "register mismatch after following fetch CPU-half");
            } else {
                destroyChip(perfect);
                return false;
            }
        }
        if (fetch_cycle) {
            pending_registers.valid = false;
        }

        counts.cycles++;

        if (qe_jammed(qe)) {
            const bool next_fetch_cycle = qe.is_opcode_fetch();
            if (opts.verbose) {
                mark_first_problem(problem, counts, "qe6502 jammed after CPU-half");
                report_jammed(seed, counts, next_fetch_cycle, qe_bus(qe), perfect_bus(perfect));
            } else {
                report_jammed(seed, counts, next_fetch_cycle, qe_bus(qe), perfect_bus(perfect));
                destroyChip(perfect);
                return false;
            }
        }

        if (opts.verbose && finish_verbose_iteration_after_problem(problem)) {
            destroyChip(perfect);
            return false;
        }
    }

    destroyChip(perfect);
    if (opts.forced_opcode_provided) {
        std::printf("netlist_lockstep: ok seed=0x%08" PRIX32 " seconds=%" PRIu32 " opcode=0x%02X cycles=%" PRIu64 " instructions=%" PRIu64 "\n",
            seed,
            opts.seconds,
            static_cast<unsigned>(opts.forced_opcode),
            counts.cycles,
            counts.instructions);
    } else {
        std::printf("netlist_lockstep: ok seed=0x%08" PRIX32 " seconds=%" PRIu32 " cycles=%" PRIu64 " instructions=%" PRIu64 "\n",
            seed,
            opts.seconds,
            counts.cycles,
            counts.instructions);
    }
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    options opts{};
    if (!parse_args(argc, argv, opts)) {
        return EXIT_FAILURE;
    }

    return run_lockstep(opts) ? EXIT_SUCCESS : EXIT_FAILURE;
}
