#include <asm6502/asm6502.h>

#include <cstdint>
#include <initializer_list>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void expect_byte(const asm6502::memory_modifiers& out, std::uint16_t address, std::uint8_t expected)
{
    const auto it = out.find(address);
    if (it == out.end())
        throw std::runtime_error("missing assembled byte at expected address");

    if (it->second != expected)
        throw std::runtime_error("assembled byte mismatch");
}

void expect_sequence(const asm6502::memory_modifiers& out,
                     std::uint16_t& pc,
                     std::initializer_list<std::uint8_t> bytes)
{
    for (std::uint8_t expected : bytes)
        expect_byte(out, pc++, expected);
}

template<class F>
void expect_invalid_argument(F&& fn)
{
    try
    {
        fn();
    }
    catch (const std::invalid_argument&)
    {
        return;
    }

    throw std::runtime_error("expected std::invalid_argument");
}

void test_existing_official_api()
{
    using namespace asm6502;

    Asm6502 p = Asm6502::New()
    .begin()
        .reset_vector("boot")
        .nmi_vector(0x1234)
        .brk_irq_vector("irq")
        .set("zp_ptr", 0x0080)
        .set("out", 0x0002)
        .set("table_plus_two", "table", +2)
        .org(0x0400, "boot")
            .lda(0x24)
            .ldx(0xfd)
            .txs()
            .pha()
            .lda(0x45)
            .ldx(0x05)
            .ldy(0x07)
            .plp()
            .jmp("test")
        .org(0x0080)
            .dw("table")
        .org(0x0500, "test")
            .label("loop")
            .lda(izy, "zp_ptr")
            .sta(zp, "out")
            .bne("loop")
            .jmp("done")
        .org(0x0600, "table")
            .db(0x34, 0x56, 0x78)
            .dw(0x12ab, "table_plus_two", sym("done", +1), 0x88cf)
        .org(0x0700, "done")
            .label("irq")
            .nop()
    .end();

    const auto out = p.compile_to_map();

    expect_byte(out, 0xfffc, 0x00);
    expect_byte(out, 0xfffd, 0x04);
    expect_byte(out, 0xfffa, 0x34);
    expect_byte(out, 0xfffb, 0x12);
    expect_byte(out, 0xfffe, 0x00);
    expect_byte(out, 0xffff, 0x07);

    expect_byte(out, 0x0400, 0xa9);
    expect_byte(out, 0x0401, 0x24);
    expect_byte(out, 0x040d, 0x4c);
    expect_byte(out, 0x040e, 0x00);
    expect_byte(out, 0x040f, 0x05);

    expect_byte(out, 0x0080, 0x00);
    expect_byte(out, 0x0081, 0x06);

    expect_byte(out, 0x0500, 0xb1);
    expect_byte(out, 0x0501, 0x80);
    expect_byte(out, 0x0502, 0x85);
    expect_byte(out, 0x0503, 0x02);
    expect_byte(out, 0x0504, 0xd0);
    expect_byte(out, 0x0505, 0xfa); // 0x0500 - 0x0506 == -6

    expect_byte(out, 0x0603, 0xab);
    expect_byte(out, 0x0604, 0x12);
    expect_byte(out, 0x0605, 0x02);
    expect_byte(out, 0x0606, 0x06);
    expect_byte(out, 0x0607, 0x01);
    expect_byte(out, 0x0608, 0x07);
    expect_byte(out, 0x0609, 0xcf);
    expect_byte(out, 0x060a, 0x88);
}

void test_undocumented_nmos_api()
{
    using namespace asm6502;

    Asm6502 p = Asm6502::New()
    .begin()
        .set("zp_symbol", 0x0080)
        .set("abs_symbol", 0x3456)
        .org(0x0800)
            .kil()
            .kil(0x12)
            .kil(0x22)
            .kil(0x32)
            .kil(0x42)
            .kil(0x52)
            .kil(0x62)
            .kil(0x72)
            .jam(0x92)
            .jam(0xb2)
            .jam(0xd2)
            .jam(0xf2)

            .nop_opcode(0x1a)
            .nop_opcode(0x3a)
            .nop_opcode(0x5a)
            .nop_opcode(0x7a)
            .nop_opcode(0xda)
            .nop_opcode(0xfa)
            .nop(imm, 0x10)
            .nop(zp, 0x11)
            .nop(zpx, 0x12)
            .nop(absolute, 0x1234)
            .nop(abx, 0x1256)
            .nop_opcode(0x82, 0x13)
            .nop_opcode(0x89, 0x14)
            .nop_opcode(0xc2, 0x15)
            .nop_opcode(0xe2, 0x16)
            .nop_opcode(0x44, 0x17)
            .nop_opcode(0x64, 0x18)
            .nop_opcode(0x34, 0x19)
            .nop_opcode(0x54, 0x1a)
            .nop_opcode(0x74, 0x1b)
            .nop_opcode(0xd4, 0x1c)
            .nop_opcode(0xf4, 0x1d)
            .nop_opcode(0x3c, 0x2222)
            .nop_opcode(0x5c, 0x3333)
            .nop_opcode(0x7c, 0x4444)
            .nop_opcode(0xdc, 0x5555)
            .nop_opcode(0xfc, 0x6666)

            .anc(0x20)
            .anc_opcode(0x2b, 0x21)
            .alr(0x22)
            .arr(0x23)
            .xaa(0x24)
            .lxa(0x25)
            .axs(0x26)
            .sbc_unofficial(0x27)
            .sbc_opcode(0xe9, 0x28)
            .sbc_opcode(0xeb, 0x29)
            .sbc_opcode(0xeb, imm, "zp_symbol")
            .kil_opcode(0x02)
            .jam_opcode(0x12)

            .slo(izx, 0x30)
            .slo(zp, 0x31)
            .slo(absolute, 0x3233)
            .slo(izy, 0x34)
            .slo(zpx, 0x35)
            .slo(aby, 0x3637)
            .slo(abx, 0x3839)

            .rla(izx, 0x40)
            .rla(zp, 0x41)
            .rla(absolute, 0x4243)
            .rla(izy, 0x44)
            .rla(zpx, 0x45)
            .rla(aby, 0x4647)
            .rla(abx, 0x4849)

            .sre(izx, 0x50)
            .sre(zp, 0x51)
            .sre(absolute, 0x5253)
            .sre(izy, 0x54)
            .sre(zpx, 0x55)
            .sre(aby, 0x5657)
            .sre(abx, 0x5859)

            .rra(izx, 0x60)
            .rra(zp, 0x61)
            .rra(absolute, 0x6263)
            .rra(izy, 0x64)
            .rra(zpx, 0x65)
            .rra(aby, 0x6667)
            .rra(abx, 0x6869)

            .dcp(izx, 0x70)
            .dcp(zp, 0x71)
            .dcp(absolute, 0x7273)
            .dcp(izy, 0x74)
            .dcp(zpx, 0x75)
            .dcp(aby, 0x7677)
            .dcp(abx, 0x7879)

            .isc(izx, 0x80)
            .isc(zp, 0x81)
            .isc(absolute, 0x8283)
            .isc(izy, 0x84)
            .isc(zpx, 0x85)
            .isc(aby, 0x8687)
            .isc(abx, 0x8889)

            .sax(izx, 0x90)
            .sax(zp, 0x91)
            .sax(absolute, 0x9293)
            .sax(zpy, 0x94)

            .ahx(izy, 0xa0)
            .ahx(aby, 0xa1a2)
            .tas(aby, 0xa3a4)
            .shy(abx, 0xa5a6)
            .shx(aby, 0xa7a8)

            .lax(izx, 0xb0)
            .lax(zp, 0xb1)
            .lax(absolute, 0xb2b3)
            .lax(izy, 0xb4)
            .lax(zpy, 0xb5)
            .lax(aby, 0xb6b7)
            .las(aby, 0xb8b9)

            .lax(zp, "zp_symbol")
            .slo(absolute, "abs_symbol")
            .nop_opcode(0x0c, "abs_symbol")
            .anc(imm, "zp_symbol")
            .ahx(aby, "abs_symbol")
    .end();

    const auto out = p.compile_to_map();
    std::uint16_t pc = 0x0800;

    expect_sequence(out, pc, {
        0x02, 0x12, 0x22, 0x32, 0x42, 0x52, 0x62, 0x72, 0x92, 0xb2, 0xd2, 0xf2,

        0x1a, 0x3a, 0x5a, 0x7a, 0xda, 0xfa,
        0x80, 0x10,
        0x04, 0x11,
        0x14, 0x12,
        0x0c, 0x34, 0x12,
        0x1c, 0x56, 0x12,
        0x82, 0x13,
        0x89, 0x14,
        0xc2, 0x15,
        0xe2, 0x16,
        0x44, 0x17,
        0x64, 0x18,
        0x34, 0x19,
        0x54, 0x1a,
        0x74, 0x1b,
        0xd4, 0x1c,
        0xf4, 0x1d,
        0x3c, 0x22, 0x22,
        0x5c, 0x33, 0x33,
        0x7c, 0x44, 0x44,
        0xdc, 0x55, 0x55,
        0xfc, 0x66, 0x66,

        0x0b, 0x20,
        0x2b, 0x21,
        0x4b, 0x22,
        0x6b, 0x23,
        0x8b, 0x24,
        0xab, 0x25,
        0xcb, 0x26,
        0xeb, 0x27,
        0xe9, 0x28,
        0xeb, 0x29,
        0xeb, 0x80,
        0x02,
        0x12,

        0x03, 0x30,
        0x07, 0x31,
        0x0f, 0x33, 0x32,
        0x13, 0x34,
        0x17, 0x35,
        0x1b, 0x37, 0x36,
        0x1f, 0x39, 0x38,

        0x23, 0x40,
        0x27, 0x41,
        0x2f, 0x43, 0x42,
        0x33, 0x44,
        0x37, 0x45,
        0x3b, 0x47, 0x46,
        0x3f, 0x49, 0x48,

        0x43, 0x50,
        0x47, 0x51,
        0x4f, 0x53, 0x52,
        0x53, 0x54,
        0x57, 0x55,
        0x5b, 0x57, 0x56,
        0x5f, 0x59, 0x58,

        0x63, 0x60,
        0x67, 0x61,
        0x6f, 0x63, 0x62,
        0x73, 0x64,
        0x77, 0x65,
        0x7b, 0x67, 0x66,
        0x7f, 0x69, 0x68,

        0xc3, 0x70,
        0xc7, 0x71,
        0xcf, 0x73, 0x72,
        0xd3, 0x74,
        0xd7, 0x75,
        0xdb, 0x77, 0x76,
        0xdf, 0x79, 0x78,

        0xe3, 0x80,
        0xe7, 0x81,
        0xef, 0x83, 0x82,
        0xf3, 0x84,
        0xf7, 0x85,
        0xfb, 0x87, 0x86,
        0xff, 0x89, 0x88,

        0x83, 0x90,
        0x87, 0x91,
        0x8f, 0x93, 0x92,
        0x97, 0x94,

        0x93, 0xa0,
        0x9f, 0xa2, 0xa1,
        0x9b, 0xa4, 0xa3,
        0x9c, 0xa6, 0xa5,
        0x9e, 0xa8, 0xa7,

        0xa3, 0xb0,
        0xa7, 0xb1,
        0xaf, 0xb3, 0xb2,
        0xb3, 0xb4,
        0xb7, 0xb5,
        0xbf, 0xb7, 0xb6,
        0xbb, 0xb9, 0xb8,

        0xa7, 0x80,
        0x0f, 0x56, 0x34,
        0x0c, 0x56, 0x34,
        0x0b, 0x80,
        0x9f, 0x56, 0x34
    });

    require(pc == static_cast<std::uint16_t>(0x0800 + out.size()), "unexpected assembled illegal program size");

    expect_invalid_argument([] { Asm6502::New().begin().slo(imm, 0x12); });
    expect_invalid_argument([] { Asm6502::New().begin().kil(0xea); });
    expect_invalid_argument([] { Asm6502::New().begin().nop_opcode(0x04); });
    expect_invalid_argument([] { Asm6502::New().begin().nop_opcode(0x02, 0x12); });
    expect_invalid_argument([] { Asm6502::New().begin().anc_opcode(0x6b, 0x12); });
    expect_invalid_argument([] { Asm6502::New().begin().sbc_opcode(0xe5, 0x12); });
    expect_invalid_argument([] { Asm6502::New().begin().sbc_opcode(0xeb, zp, 0x12); });
    expect_invalid_argument([] { Asm6502::New().begin().kil_opcode(0xea); });
    expect_invalid_argument([] { Asm6502::New().begin().jam_opcode(0xea); });
}

} // namespace

int main()
{
    test_existing_official_api();
    test_undocumented_nmos_api();
    return 0;
}
