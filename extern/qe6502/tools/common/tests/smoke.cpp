#include <tools6502/common.hpp>
#include <tools6502/testcase_collections.hpp>

#include <cstdint>
#include <stdexcept>

namespace {

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void expect_byte(const tools6502::memory_setup& setup,
                 std::uint16_t address,
                 std::uint8_t expected)
{
    for (const auto& [actual_address, actual_value] : setup)
    {
        if (actual_address == address)
        {
            if (actual_value != expected)
                throw std::runtime_error("unexpected byte value");
            return;
        }
    }

    throw std::runtime_error("missing byte");
}

void test_default_testcase_shape()
{
    const tools6502::testcase test{};

    require(test.start_at == 0x0400u, "default start_at changed");
    require(test.vectors.reset == 0x0200u, "default reset vector changed");
    require(test.vectors.brk_irq == 0x9100u, "default brk/irq vector changed");
    require(test.vectors.nmi == 0x9000u, "default nmi vector changed");
    require(test.P == 0x24u, "default P changed");
    require(test.S == 0xfdu, "default S changed");
}

void test_bootstrap_uses_testcase_vectors()
{
    tools6502::testcase test{};
    test.start_at = 0x0456u;
    test.vectors.reset = 0x1234u;
    test.vectors.brk_irq = 0x5678u;
    test.vectors.nmi = 0x9abcu;
    test.A = 0x11u;
    test.X = 0x22u;
    test.Y = 0x33u;
    test.P = 0x24u;
    test.S = 0xfdu;

    const auto bootstrap = tools6502::make_bootstrap(test);

    expect_byte(bootstrap, 0xfffc, 0x34u);
    expect_byte(bootstrap, 0xfffd, 0x12u);
    expect_byte(bootstrap, 0xfffe, 0x78u);
    expect_byte(bootstrap, 0xffff, 0x56u);
    expect_byte(bootstrap, 0xfffa, 0xbcu);
    expect_byte(bootstrap, 0xfffb, 0x9au);
}

void test_nmos_opcode_testcase_collection_shape()
{
    const auto cases = tools6502::get_nmos6502_opcode_testcases();

    require(cases.size() == 256u, "NMOS opcode testcase collection must cover all opcodes");

    for (std::uint16_t opcode = 0; opcode <= 0xffu; ++opcode)
    {
        const auto it = cases.find(static_cast<std::uint8_t>(opcode));
        require(it != cases.end(), "missing opcode testcase bucket");
        require(!it->second.empty(), "empty opcode testcase bucket");

        for (const auto& test : it->second)
        {
            require(test.opcode == static_cast<std::uint8_t>(opcode), "opcode field/key mismatch");
            require(test.bytes != 0u, "testcase byte length must be present");
            require(test.start_at != 0u, "testcase start_at must be present");
            require(!test.program.empty(), "testcase program must be present");
            require(test.vectors.nmi == 0x0800u, "adapted NMOS testcase nmi vector changed");
            require(test.vectors.brk_irq == 0x0800u, "adapted NMOS testcase brk/irq vector changed");
        }
    }
}

} // namespace

int main()
{
    test_default_testcase_shape();
    test_bootstrap_uses_testcase_vectors();
    test_nmos_opcode_testcase_collection_shape();
    return 0;
}
