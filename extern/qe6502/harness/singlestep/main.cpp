#include <qe6502/cpu.hpp>

#include <nlohmann/json.hpp>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <exception>
#include <fstream>
#include <string>

namespace {

struct options {
    qe6502::model model = qe6502::model::nmos;
    std::string path;
    int opcode = -1;
    bool all = false;
    bool compare_cycles = true;
    std::uint64_t max_cases = 0;
};

struct counters {
    std::uint64_t files_run = 0;
    std::uint64_t files_skipped = 0;
    std::uint64_t cases_run = 0;
    std::uint64_t cases_failed = 0;
};

void print_usage(const char* argv0)
{
    std::fprintf(stderr,
        "usage: %s --model nmos|nes|wdc|rw|st --path <ProcessorTests dir> (--opcode xx|--all) [--max-cases n] [--no-cycles]\n",
        argv0);
}

bool parse_hex_byte(const std::string& text, int& out)
{
    if (text.size() != 2u) {
        return false;
    }
    char* end = nullptr;
    const long value = std::strtol(text.c_str(), &end, 16);
    if (end == nullptr || *end != '\0' || value < 0 || value > 0xff) {
        return false;
    }
    out = static_cast<int>(value);
    return true;
}

bool parse_args(int argc, char** argv, options& out)
{
    for (int i = 1; i < argc; i++) {
        const std::string arg = argv[i];
        if (arg == "--model" && i + 1 < argc) {
            const std::string value = argv[++i];
            if (value == "nmos" || value == "6502") {
                out.model = qe6502::model::nmos;
            } else if (value == "nes" || value == "nes6502") {
                out.model = qe6502::model::nes;
            } else if (value == "wdc" || value == "wdc65c02" || value == "65c02") {
                out.model = qe6502::model::wdc;
            } else if (value == "rw" || value == "rockwell" || value == "rockwell65c02") {
                out.model = qe6502::model::rw;
            } else if (value == "st" || value == "synertek" || value == "synertek65c02") {
                out.model = qe6502::model::st;
            } else {
                std::fprintf(stderr, "unknown model: %s\n", value.c_str());
                return false;
            }
        } else if (arg == "--path" && i + 1 < argc) {
            out.path = argv[++i];
        } else if (arg == "--opcode" && i + 1 < argc) {
            int opcode = -1;
            if (!parse_hex_byte(argv[++i], opcode)) {
                std::fprintf(stderr, "invalid opcode: %s\n", argv[i]);
                return false;
            }
            out.opcode = opcode;
        } else if (arg == "--all") {
            out.all = true;
        } else if (arg == "--no-cycles") {
            out.compare_cycles = false;
        } else if (arg == "--max-cases" && i + 1 < argc) {
            char* end = nullptr;
            const unsigned long long value = std::strtoull(argv[++i], &end, 10);
            if (end == nullptr || *end != '\0') {
                std::fprintf(stderr, "invalid max-cases: %s\n", argv[i]);
                return false;
            }
            out.max_cases = static_cast<std::uint64_t>(value);
        } else {
            std::fprintf(stderr, "unknown argument: %s\n", arg.c_str());
            return false;
        }
    }

    if (out.path.empty()) {
        std::fprintf(stderr, "missing --path\n");
        return false;
    }
    if ((out.opcode >= 0 && out.all) || (out.opcode < 0 && !out.all)) {
        std::fprintf(stderr, "use exactly one of --opcode or --all\n");
        return false;
    }
    return true;
}

std::string opcode_filename(std::uint8_t opcode)
{
    constexpr char digits[] = "0123456789abcdef";
    std::string out;
    out.push_back(digits[(opcode >> 4u) & 0x0fu]);
    out.push_back(digits[opcode & 0x0fu]);
    out += ".json";
    return out;
}

std::string join_path(const std::string& dir, const std::string& file)
{
    if (!dir.empty() && (dir.back() == '/' || dir.back() == '\\')) {
        return dir + file;
    }
    return dir + "/" + file;
}

std::uint8_t json_u8(const nlohmann::json& value)
{
    return static_cast<std::uint8_t>(value.get<std::uint32_t>() & 0xffu);
}

std::uint16_t json_u16(const nlohmann::json& value)
{
    return static_cast<std::uint16_t>(value.get<std::uint32_t>() & 0xffffu);
}

void load_ram(std::array<std::uint8_t, 0x10000>& memory, const nlohmann::json& ram)
{
    for (const auto& entry : ram) {
        memory[json_u16(entry.at(0u))] = json_u8(entry.at(1u));
    }
}

void set_initial_state(qe6502::cpu& cpu, const nlohmann::json& initial)
{
    cpu.pc(json_u16(initial.at("pc")));
    cpu.s(json_u8(initial.at("s")));
    cpu.a(json_u8(initial.at("a")));
    cpu.x(json_u8(initial.at("x")));
    cpu.y(json_u8(initial.at("y")));
    cpu.p(json_u8(initial.at("p")));
}

bool compare_u8(const char* name, std::uint8_t actual, std::uint8_t expected)
{
    if (actual != expected) {
        std::fprintf(stderr, "%s mismatch: got 0x%02X expected 0x%02X\n", name, static_cast<unsigned>(actual), static_cast<unsigned>(expected));
        return false;
    }
    return true;
}

bool compare_u16(const char* name, std::uint16_t actual, std::uint16_t expected)
{
    if (actual != expected) {
        std::fprintf(stderr, "%s mismatch: got 0x%04X expected 0x%04X\n", name, static_cast<unsigned>(actual), static_cast<unsigned>(expected));
        return false;
    }
    return true;
}


bool is_nmos_kil_opcode(qe6502::model model, std::uint8_t opcode)
{
    if (model != qe6502::model::nmos && model != qe6502::model::nes) {
        return false;
    }

    for (const std::uint8_t kil_opcode : {
             static_cast<std::uint8_t>(0x02u),
             static_cast<std::uint8_t>(0x12u),
             static_cast<std::uint8_t>(0x22u),
             static_cast<std::uint8_t>(0x32u),
             static_cast<std::uint8_t>(0x42u),
             static_cast<std::uint8_t>(0x52u),
             static_cast<std::uint8_t>(0x62u),
             static_cast<std::uint8_t>(0x72u),
             static_cast<std::uint8_t>(0x92u),
             static_cast<std::uint8_t>(0xB2u),
             static_cast<std::uint8_t>(0xD2u),
             static_cast<std::uint8_t>(0xF2u),
         }) {
        if (opcode == kil_opcode) {
            return true;
        }
    }
    return false;
}

bool compare_final_state(const qe6502::cpu& cpu, const std::array<std::uint8_t, 0x10000>& memory, const nlohmann::json& final)
{
    bool ok = true;
    ok = compare_u16("PC", cpu.pc(), json_u16(final.at("pc"))) && ok;
    ok = compare_u8("S", cpu.s(), json_u8(final.at("s"))) && ok;
    ok = compare_u8("A", cpu.a(), json_u8(final.at("a"))) && ok;
    ok = compare_u8("X", cpu.x(), json_u8(final.at("x"))) && ok;
    ok = compare_u8("Y", cpu.y(), json_u8(final.at("y"))) && ok;
    ok = compare_u8("P", cpu.p(), json_u8(final.at("p"))) && ok;

    for (const auto& entry : final.at("ram")) {
        const std::uint16_t address = json_u16(entry.at(0u));
        const std::uint8_t expected = json_u8(entry.at(1u));
        if (memory[address] != expected) {
            std::fprintf(stderr, "RAM[0x%04X] mismatch: got 0x%02X expected 0x%02X\n", static_cast<unsigned>(address), static_cast<unsigned>(memory[address]), static_cast<unsigned>(expected));
            ok = false;
        }
    }
    return ok;
}

bool run_case(const nlohmann::json& test_case, qe6502::model model, bool compare_cycles)
{
    std::array<std::uint8_t, 0x10000> memory{};
    qe6502::cpu cpu{model};
    const nlohmann::json& initial = test_case.at("initial");
    const nlohmann::json& final = test_case.at("final");
    const nlohmann::json& cycles = test_case.at("cycles");
    const std::string name = test_case.contains("name") ? test_case.at("name").get<std::string>() : std::string{"<unnamed>"};

    load_ram(memory, initial.at("ram"));
    set_initial_state(cpu, initial);
    cpu.jump_to(cpu.pc());

    constexpr std::size_t nmos_kil_cycles_to_compare = 2u;
    const std::uint16_t initial_pc = json_u16(initial.at("pc"));
    const bool nmos_kil_opcode = is_nmos_kil_opcode(model, memory[initial_pc]);
    std::size_t cycle_index = 0u;
    const std::size_t cycles_to_run = compare_cycles
        ? (nmos_kil_opcode && cycles.size() > nmos_kil_cycles_to_compare ? nmos_kil_cycles_to_compare : cycles.size())
        : static_cast<std::size_t>(0u);
    while (compare_cycles ? (cycle_index < cycles_to_run) : true) {
        if (compare_cycles) {
            const nlohmann::json& expected_cycle = cycles.at(cycle_index);
            const std::uint16_t expected_address = json_u16(expected_cycle.at(0u));
            const std::uint8_t expected_data = json_u8(expected_cycle.at(1u));
            const std::string expected_rw = expected_cycle.at(2u).get<std::string>();
            const bool expected_write = expected_rw == "write";

            if (cpu.bus_address() != expected_address) {
                std::fprintf(stderr, "%s: cycle %zu address mismatch: got 0x%04X expected 0x%04X\n", name.c_str(), cycle_index, static_cast<unsigned>(cpu.bus_address()), static_cast<unsigned>(expected_address));
                return false;
            }
            if (cpu.is_write() != expected_write) {
                std::fprintf(stderr, "%s: cycle %zu rw mismatch: got %s expected %s\n", name.c_str(), cycle_index, cpu.is_write() ? "write" : "read", expected_rw.c_str());
                return false;
            }
            const std::uint8_t actual_data = cpu.is_write() ? cpu.bus_data() : memory[cpu.bus_address()];
            if (actual_data != expected_data) {
                std::fprintf(stderr, "%s: cycle %zu data mismatch: got 0x%02X expected 0x%02X\n", name.c_str(), cycle_index, static_cast<unsigned>(actual_data), static_cast<unsigned>(expected_data));
                return false;
            }
        }

        const std::uint8_t bus = cpu.is_write() ? cpu.bus_data() : memory[cpu.bus_address()];
        if (cpu.is_write()) {
            memory[cpu.bus_address()] = bus;
        }
        cpu.tick(bus);
        cycle_index++;

        if (!compare_cycles && nmos_kil_opcode && cycle_index >= nmos_kil_cycles_to_compare) {
            break;
        }
        if (!compare_cycles && cpu.is_opcode_fetch()) {
            break;
        }
        if (!compare_cycles && cycle_index > 32u) {
            std::fprintf(stderr, "%s: instruction cycle limit exceeded\n", name.c_str());
            return false;
        }
    }

    if (!compare_final_state(cpu, memory, final)) {
        std::fprintf(stderr, "%s: final state mismatch\n", name.c_str());
        return false;
    }
    return true;
}

bool run_file(const options& opts, std::uint8_t opcode, counters& counts)
{
    const std::string filename = join_path(opts.path, opcode_filename(opcode));
    std::ifstream in{filename};
    if (!in) {
        std::fprintf(stderr, "failed to open %s\n", filename.c_str());
        return false;
    }

    if (in.peek() == std::ifstream::traits_type::eof()) {
        counts.files_run++;
        std::printf(
            "PASS %02X (0 cases)\n",
            static_cast<unsigned>(opcode));
        return true;
    }

    const nlohmann::json tests = nlohmann::json::parse(in);
    if (!tests.is_array()) {
        std::fprintf(stderr, "%s: root JSON value is not an array\n", filename.c_str());
        return false;
    }

    std::uint64_t local_cases = 0;
    for (const auto& test_case : tests) {
        if (opts.max_cases != 0u && local_cases >= opts.max_cases) {
            break;
        }
        if (!run_case(test_case, opts.model, opts.compare_cycles)) {
            counts.cases_failed++;
            std::fprintf(stderr,
                "FAIL opcode %02X case #%llu\n",
                static_cast<unsigned>(opcode),
                static_cast<unsigned long long>(local_cases));
            return false;
        }
        local_cases++;
        counts.cases_run++;
    }

    counts.files_run++;
    std::printf(
        "PASS %02X (%llu cases)\n",
        static_cast<unsigned>(opcode),
        static_cast<unsigned long long>(local_cases));
    return true;
}

bool run_requested(const options& opts, counters& counts)
{
    if (opts.opcode >= 0) {
        return run_file(opts, static_cast<std::uint8_t>(opts.opcode), counts);
    }

    for (unsigned opcode = 0u; opcode <= 0xffu; opcode++) {
        if (!run_file(opts, static_cast<std::uint8_t>(opcode), counts)) {
            return false;
        }
    }
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    options opts;
    if (!parse_args(argc, argv, opts)) {
        print_usage(argv[0]);
        return 2;
    }

    counters counts;
    try {
        if (!run_requested(opts, counts)) {
            return 1;
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }

    std::printf(
        "SUMMARY files=%llu skipped=%llu cases=%llu failed=%llu\n",
        static_cast<unsigned long long>(counts.files_run),
        static_cast<unsigned long long>(counts.files_skipped),
        static_cast<unsigned long long>(counts.cases_run),
        static_cast<unsigned long long>(counts.cases_failed));

    return counts.cases_failed == 0u ? 0 : 1;
}
