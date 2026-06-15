#include <tools6502/testcase_collections.hpp>

#include <asm6502/asm6502.h>

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace tools6502 {



/*
 * Return NMOS 6502 opcode timing/scenario testcases for every byte value 0x00-0xFF,
 * including the undocumented/illegal opcodes implemented by the qe6502 NMOS control
 * store.  Each map key is an opcode and each value contains one or more scenarios for
 * that opcode.
 *
 * The scenarios are meant to drive a cycle/timing observer, not to be a full arithmetic
 * truth table for every possible input value.  For fixed-cycle opcodes there is one
 * representative straight-line case.  For opcodes whose execution length can vary, all
 * timing-distinct paths are represented: conditional branches have not-taken, taken on
 * the same page, and taken with page crossing; indexed read/NOP modes that take an
 * extra page-cross cycle have both no-cross and cross cases; fixed-cycle indexed
 * store/read-modify-write modes also include no-cross and cross addressing cases
 * where the bus/addressing path differs even when the cycle count does not.  Control-flow opcodes place
 * the final trap at the address reached by the opcode (BRK vector, JMP/JSR target,
 * RTS/RTI return address).  The NMOS JMP ($xxFF) indirect high-byte wrap is included as
 * an explicit addressing corner case.
 *
 * Every generated mini-program contains the opcode under test, emitted at
 * testcase::start_at with the requested operands, and a terminal self-jump trap
 * (`JMP trap`) at the address where execution is expected to continue after that
 * opcode. Some not-taken branch cases also define a non-fallthrough branch target
 * label so the encoded relative operand is meaningful even though control falls
 * through to the trap.  Any other bytes in testcase::program are data only: vectors,
 * pointer tables, stack return bytes, or memory operands needed by the instruction.
 */
std::map<std::uint8_t, std::vector<testcase>> get_nmos6502_opcode_testcases()
{
    using namespace asm6502;
    return{
        // 0x00
        {
            0x00,
            {
                testcase{
                    .opcode = 0x00,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .brk_irq_vector("trap")
                        .org(0x0400)
                            .brk()
                        .org(0x0800, "trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "BRK vectors through BRK/IRQ vector",
                },
            }
        },
        // 0x01
        {
            0x01,
            {
                testcase{
                    .opcode = 0x01,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_ptr_base")
                        .org(0x0008, "zp_ptr")
                            .dw("value_addr")
                        .org(0x2134, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .ora(izx, "zp_ptr_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ORA (zp,X) indexed indirect with zero-page pointer-base wraparound",
                },
            }
        },
        // 0x02
        {
            0x02,
            {
                testcase{
                    .opcode = 0x02,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .kil_opcode(0x02)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "KIL/JAM enters JAM/KIL bus loop",
                },
            }
        },
        // 0x03
        {
            0x03,
            {
                testcase{
                    .opcode = 0x03,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_ptr_base")
                        .org(0x0008, "zp_ptr")
                            .dw("value_addr")
                        .org(0x2134, "value_addr")
                            .db(0x41)
                        .org(0x0400)
                            .slo(izx, "zp_ptr_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SLO (zp,X) indexed indirect with zero-page pointer-base wraparound",
                },
            }
        },
        // 0x04
        {
            0x04,
            {
                testcase{
                    .opcode = 0x04,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x87)
                        .org(0x0400)
                            .nop_opcode(0x04, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP zp zero page",
                },
            }
        },
        // 0x05
        {
            0x05,
            {
                testcase{
                    .opcode = 0x05,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x87)
                        .org(0x0400)
                            .ora(zp, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ORA zp zero page",
                },
            }
        },
        // 0x06
        {
            0x06,
            {
                testcase{
                    .opcode = 0x06,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x81, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x41)
                        .org(0x0400)
                            .asl(zp, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ASL zp zero page",
                },
            }
        },
        // 0x07
        {
            0x07,
            {
                testcase{
                    .opcode = 0x07,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x41)
                        .org(0x0400)
                            .slo(zp, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SLO zp zero page",
                },
            }
        },
        // 0x08
        {
            0x08,
            {
                testcase{
                    .opcode = 0x08,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0xe5, .S = 0xfe,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .php()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "PHP implied",
                },
            }
        },
        // 0x09
        {
            0x09,
            {
                testcase{
                    .opcode = 0x09,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .ora(0x18)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ORA #imm immediate",
                },
            }
        },
        // 0x0A
        {
            0x0A,
            {
                testcase{
                    .opcode = 0x0A,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x81, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .asl()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ASL A accumulator",
                },
            }
        },
        // 0x0B
        {
            0x0B,
            {
                testcase{
                    .opcode = 0x0B,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .anc(0x7f)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ANC #imm immediate",
                },
            }
        },
        // 0x0C
        {
            0x0C,
            {
                testcase{
                    .opcode = 0x0C,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .nop_opcode(0x0C, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0x87)
                    .end().compile(),
                    .description = "NOP abs absolute",
                },
            }
        },
        // 0x0D
        {
            0x0D,
            {
                testcase{
                    .opcode = 0x0D,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .ora(absolute, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0x87)
                    .end().compile(),
                    .description = "ORA abs absolute",
                },
            }
        },
        // 0x0E
        {
            0x0E,
            {
                testcase{
                    .opcode = 0x0E,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x81, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .asl(absolute, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0x41)
                    .end().compile(),
                    .description = "ASL abs absolute",
                },
            }
        },
        // 0x0F
        {
            0x0F,
            {
                testcase{
                    .opcode = 0x0F,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .slo(absolute, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0x41)
                    .end().compile(),
                    .description = "SLO abs absolute",
                },
            }
        },
        // 0x10
        {
            0x10,
            {
                testcase{
                    .opcode = 0x10,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0xa4, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .bpl("branch_target")
                        .label("trap")
                            .jmp("trap")
                        .org(0x0420, "branch_target")
                    .end().compile(),
                    .description = "BPL rel not taken with a non-fallthrough encoded target",
                },
                testcase{
                    .opcode = 0x10,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .bpl("trap")
                        .org(0x0420, "trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "BPL rel taken without page cross to a non-fallthrough target",
                },
                testcase{
                    .opcode = 0x10,
                    .bytes = 2,
                    .start_at = 0x04f0,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x04f0)
                            .bpl("trap")
                        .org(0x0505, "trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "BPL rel taken with page cross",
                },
            }
        },
        // 0x11
        {
            0x11,
            {
                testcase{
                    .opcode = 0x11,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .ora(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ORA (zp),Y indirect indexed without page cross",
                },
                testcase{
                    .opcode = 0x11,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .ora(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ORA (zp),Y indirect indexed with page cross",
                },
            }
        },
        // 0x12
        {
            0x12,
            {
                testcase{
                    .opcode = 0x12,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .kil_opcode(0x12)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "KIL/JAM enters JAM/KIL bus loop",
                },
            }
        },
        // 0x13
        {
            0x13,
            {
                testcase{
                    .opcode = 0x13,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x41)
                        .org(0x0400)
                            .slo(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SLO (zp),Y indirect indexed",
                },
                testcase{
                    .opcode = 0x13,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x41)
                        .org(0x0400)
                            .slo(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SLO (zp),Y indirect indexed with page cross",
                },
            }
        },
        // 0x14
        {
            0x14,
            {
                testcase{
                    .opcode = 0x14,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x87)
                        .org(0x0400)
                            .nop_opcode(0x14, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP zp,X zero page,X with wraparound",
                },
            }
        },
        // 0x15
        {
            0x15,
            {
                testcase{
                    .opcode = 0x15,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x87)
                        .org(0x0400)
                            .ora(zpx, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ORA zp,X zero page,X with wraparound",
                },
            }
        },
        // 0x16
        {
            0x16,
            {
                testcase{
                    .opcode = 0x16,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x81, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x41)
                        .org(0x0400)
                            .asl(zpx, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ASL zp,X zero page,X with wraparound",
                },
            }
        },
        // 0x17
        {
            0x17,
            {
                testcase{
                    .opcode = 0x17,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x41)
                        .org(0x0400)
                            .slo(zpx, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SLO zp,X zero page,X with wraparound",
                },
            }
        },
        // 0x18
        {
            0x18,
            {
                testcase{
                    .opcode = 0x18,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .clc()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "CLC implied",
                },
            }
        },
        // 0x19
        {
            0x19,
            {
                testcase{
                    .opcode = 0x19,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .ora(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ORA abs,Y absolute,Y without page cross",
                },
                testcase{
                    .opcode = 0x19,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .ora(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ORA abs,Y absolute,Y with page cross",
                },
            }
        },
        // 0x1A
        {
            0x1A,
            {
                testcase{
                    .opcode = 0x1A,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .nop_opcode(0x1A)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP implied",
                },
            }
        },
        // 0x1B
        {
            0x1B,
            {
                testcase{
                    .opcode = 0x1B,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x41)
                        .org(0x0400)
                            .slo(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SLO abs,Y absolute,Y",
                },
                testcase{
                    .opcode = 0x1B,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x41)
                        .org(0x0400)
                            .slo(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SLO abs,Y absolute,Y with page cross",
                },
            }
        },
        // 0x1C
        {
            0x1C,
            {
                testcase{
                    .opcode = 0x1C,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .nop_opcode(0x1C, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP abs,X absolute,X without page cross",
                },
                testcase{
                    .opcode = 0x1C,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x20, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .nop_opcode(0x1C, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP abs,X absolute,X with page cross",
                },
            }
        },
        // 0x1D
        {
            0x1D,
            {
                testcase{
                    .opcode = 0x1D,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .ora(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ORA abs,X absolute,X without page cross",
                },
                testcase{
                    .opcode = 0x1D,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x20, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .ora(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ORA abs,X absolute,X with page cross",
                },
            }
        },
        // 0x1E
        {
            0x1E,
            {
                testcase{
                    .opcode = 0x1E,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x81, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x41)
                        .org(0x0400)
                            .asl(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ASL abs,X absolute,X",
                },
                testcase{
                    .opcode = 0x1E,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x81, .X = 0x20, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x41)
                        .org(0x0400)
                            .asl(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ASL abs,X absolute,X with page cross",
                },
            }
        },
        // 0x1F
        {
            0x1F,
            {
                testcase{
                    .opcode = 0x1F,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x41)
                        .org(0x0400)
                            .slo(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SLO abs,X absolute,X",
                },
                testcase{
                    .opcode = 0x1F,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x20, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x41)
                        .org(0x0400)
                            .slo(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SLO abs,X absolute,X with page cross",
                },
            }
        },
        // 0x20
        {
            0x20,
            {
                testcase{
                    .opcode = 0x20,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .jsr("trap")
                        .org(0x0800, "trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "JSR abs jumps to subroutine target",
                },
            }
        },
        // 0x21
        {
            0x21,
            {
                testcase{
                    .opcode = 0x21,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_ptr_base")
                        .org(0x0008, "zp_ptr")
                            .dw("value_addr")
                        .org(0x2134, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .and_(izx, "zp_ptr_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "AND (zp,X) indexed indirect with zero-page pointer-base wraparound",
                },
            }
        },
        // 0x22
        {
            0x22,
            {
                testcase{
                    .opcode = 0x22,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .kil_opcode(0x22)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "KIL/JAM enters JAM/KIL bus loop",
                },
            }
        },
        // 0x23
        {
            0x23,
            {
                testcase{
                    .opcode = 0x23,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_ptr_base")
                        .org(0x0008, "zp_ptr")
                            .dw("value_addr")
                        .org(0x2134, "value_addr")
                            .db(0x81)
                        .org(0x0400)
                            .rla(izx, "zp_ptr_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "RLA (zp,X) indexed indirect with zero-page pointer-base wraparound",
                },
            }
        },
        // 0x24
        {
            0x24,
            {
                testcase{
                    .opcode = 0x24,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0xc0)
                        .org(0x0400)
                            .bit(zp, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "BIT zp zero page",
                },
            }
        },
        // 0x25
        {
            0x25,
            {
                testcase{
                    .opcode = 0x25,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x87)
                        .org(0x0400)
                            .and_(zp, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "AND zp zero page",
                },
            }
        },
        // 0x26
        {
            0x26,
            {
                testcase{
                    .opcode = 0x26,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x81, .X = 0x00, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x81)
                        .org(0x0400)
                            .rol(zp, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ROL zp zero page",
                },
            }
        },
        // 0x27
        {
            0x27,
            {
                testcase{
                    .opcode = 0x27,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x81)
                        .org(0x0400)
                            .rla(zp, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "RLA zp zero page",
                },
            }
        },
        // 0x28
        {
            0x28,
            {
                testcase{
                    .opcode = 0x28,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x01fe, "stack_value")
                            .db(0xa5)
                        .org(0x0400)
                            .plp()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "PLP pulls value from stack",
                },
            }
        },
        // 0x29
        {
            0x29,
            {
                testcase{
                    .opcode = 0x29,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .and_(0x3c)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "AND #imm immediate",
                },
            }
        },
        // 0x2A
        {
            0x2A,
            {
                testcase{
                    .opcode = 0x2A,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x81, .X = 0x00, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .rol()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ROL A accumulator",
                },
            }
        },
        // 0x2B
        {
            0x2B,
            {
                testcase{
                    .opcode = 0x2B,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .anc_opcode(0x2B, 0x7f)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ANC #imm immediate",
                },
            }
        },
        // 0x2C
        {
            0x2C,
            {
                testcase{
                    .opcode = 0x2C,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .bit(absolute, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0xc0)
                    .end().compile(),
                    .description = "BIT abs absolute",
                },
            }
        },
        // 0x2D
        {
            0x2D,
            {
                testcase{
                    .opcode = 0x2D,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .and_(absolute, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0x87)
                    .end().compile(),
                    .description = "AND abs absolute",
                },
            }
        },
        // 0x2E
        {
            0x2E,
            {
                testcase{
                    .opcode = 0x2E,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x81, .X = 0x00, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .rol(absolute, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0x81)
                    .end().compile(),
                    .description = "ROL abs absolute",
                },
            }
        },
        // 0x2F
        {
            0x2F,
            {
                testcase{
                    .opcode = 0x2F,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .rla(absolute, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0x81)
                    .end().compile(),
                    .description = "RLA abs absolute",
                },
            }
        },
        // 0x30
        {
            0x30,
            {
                testcase{
                    .opcode = 0x30,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .bmi("branch_target")
                        .label("trap")
                            .jmp("trap")
                        .org(0x0420, "branch_target")
                    .end().compile(),
                    .description = "BMI rel not taken with a non-fallthrough encoded target",
                },
                testcase{
                    .opcode = 0x30,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0xa4, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .bmi("trap")
                        .org(0x0420, "trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "BMI rel taken without page cross to a non-fallthrough target",
                },
                testcase{
                    .opcode = 0x30,
                    .bytes = 2,
                    .start_at = 0x04f0,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0xa4, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x04f0)
                            .bmi("trap")
                        .org(0x0505, "trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "BMI rel taken with page cross",
                },
            }
        },
        // 0x31
        {
            0x31,
            {
                testcase{
                    .opcode = 0x31,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .and_(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "AND (zp),Y indirect indexed without page cross",
                },
                testcase{
                    .opcode = 0x31,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .and_(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "AND (zp),Y indirect indexed with page cross",
                },
            }
        },
        // 0x32
        {
            0x32,
            {
                testcase{
                    .opcode = 0x32,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .kil_opcode(0x32)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "KIL/JAM enters JAM/KIL bus loop",
                },
            }
        },
        // 0x33
        {
            0x33,
            {
                testcase{
                    .opcode = 0x33,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x10, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x81)
                        .org(0x0400)
                            .rla(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "RLA (zp),Y indirect indexed",
                },
                testcase{
                    .opcode = 0x33,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x41)
                        .org(0x0400)
                            .rla(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "RLA (zp),Y indirect indexed with page cross",
                },
            }
        },
        // 0x34
        {
            0x34,
            {
                testcase{
                    .opcode = 0x34,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x87)
                        .org(0x0400)
                            .nop_opcode(0x34, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP zp,X zero page,X with wraparound",
                },
            }
        },
        // 0x35
        {
            0x35,
            {
                testcase{
                    .opcode = 0x35,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x87)
                        .org(0x0400)
                            .and_(zpx, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "AND zp,X zero page,X with wraparound",
                },
            }
        },
        // 0x36
        {
            0x36,
            {
                testcase{
                    .opcode = 0x36,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x81, .X = 0x10, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x81)
                        .org(0x0400)
                            .rol(zpx, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ROL zp,X zero page,X with wraparound",
                },
            }
        },
        // 0x37
        {
            0x37,
            {
                testcase{
                    .opcode = 0x37,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x81)
                        .org(0x0400)
                            .rla(zpx, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "RLA zp,X zero page,X with wraparound",
                },
            }
        },
        // 0x38
        {
            0x38,
            {
                testcase{
                    .opcode = 0x38,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .sec()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SEC implied",
                },
            }
        },
        // 0x39
        {
            0x39,
            {
                testcase{
                    .opcode = 0x39,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .and_(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "AND abs,Y absolute,Y without page cross",
                },
                testcase{
                    .opcode = 0x39,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .and_(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "AND abs,Y absolute,Y with page cross",
                },
            }
        },
        // 0x3A
        {
            0x3A,
            {
                testcase{
                    .opcode = 0x3A,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .nop_opcode(0x3A)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP implied",
                },
            }
        },
        // 0x3B
        {
            0x3B,
            {
                testcase{
                    .opcode = 0x3B,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x10, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x81)
                        .org(0x0400)
                            .rla(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "RLA abs,Y absolute,Y",
                },
                testcase{
                    .opcode = 0x3B,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x41)
                        .org(0x0400)
                            .rla(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "RLA abs,Y absolute,Y with page cross",
                },
            }
        },
        // 0x3C
        {
            0x3C,
            {
                testcase{
                    .opcode = 0x3C,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .nop_opcode(0x3C, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP abs,X absolute,X without page cross",
                },
                testcase{
                    .opcode = 0x3C,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x20, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .nop_opcode(0x3C, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP abs,X absolute,X with page cross",
                },
            }
        },
        // 0x3D
        {
            0x3D,
            {
                testcase{
                    .opcode = 0x3D,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .and_(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "AND abs,X absolute,X without page cross",
                },
                testcase{
                    .opcode = 0x3D,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x20, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .and_(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "AND abs,X absolute,X with page cross",
                },
            }
        },
        // 0x3E
        {
            0x3E,
            {
                testcase{
                    .opcode = 0x3E,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x81, .X = 0x10, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x81)
                        .org(0x0400)
                            .rol(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ROL abs,X absolute,X",
                },
                testcase{
                    .opcode = 0x3E,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x81, .X = 0x20, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x41)
                        .org(0x0400)
                            .rol(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ROL abs,X absolute,X with page cross",
                },
            }
        },
        // 0x3F
        {
            0x3F,
            {
                testcase{
                    .opcode = 0x3F,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x81)
                        .org(0x0400)
                            .rla(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "RLA abs,X absolute,X",
                },
                testcase{
                    .opcode = 0x3F,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x20, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x41)
                        .org(0x0400)
                            .rla(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "RLA abs,X absolute,X with page cross",
                },
            }
        },
        // 0x40
        {
            0x40,
            {
                testcase{
                    .opcode = 0x40,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfc,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x01fd, "rti_stack_frame")
                            .db(0x24)
                            .dw("trap")
                        .org(0x0400)
                            .rti()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "RTI pulls P and PC from stack",
                },
            }
        },
        // 0x41
        {
            0x41,
            {
                testcase{
                    .opcode = 0x41,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_ptr_base")
                        .org(0x0008, "zp_ptr")
                            .dw("value_addr")
                        .org(0x2134, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .eor(izx, "zp_ptr_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "EOR (zp,X) indexed indirect with zero-page pointer-base wraparound",
                },
            }
        },
        // 0x42
        {
            0x42,
            {
                testcase{
                    .opcode = 0x42,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .kil_opcode(0x42)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "KIL/JAM enters JAM/KIL bus loop",
                },
            }
        },
        // 0x43
        {
            0x43,
            {
                testcase{
                    .opcode = 0x43,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_ptr_base")
                        .org(0x0008, "zp_ptr")
                            .dw("value_addr")
                        .org(0x2134, "value_addr")
                            .db(0x82)
                        .org(0x0400)
                            .sre(izx, "zp_ptr_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SRE (zp,X) indexed indirect with zero-page pointer-base wraparound",
                },
            }
        },
        // 0x44
        {
            0x44,
            {
                testcase{
                    .opcode = 0x44,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x87)
                        .org(0x0400)
                            .nop_opcode(0x44, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP zp zero page",
                },
            }
        },
        // 0x45
        {
            0x45,
            {
                testcase{
                    .opcode = 0x45,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x87)
                        .org(0x0400)
                            .eor(zp, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "EOR zp zero page",
                },
            }
        },
        // 0x46
        {
            0x46,
            {
                testcase{
                    .opcode = 0x46,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x81, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x82)
                        .org(0x0400)
                            .lsr(zp, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LSR zp zero page",
                },
            }
        },
        // 0x47
        {
            0x47,
            {
                testcase{
                    .opcode = 0x47,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x82)
                        .org(0x0400)
                            .sre(zp, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SRE zp zero page",
                },
            }
        },
        // 0x48
        {
            0x48,
            {
                testcase{
                    .opcode = 0x48,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x8e, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .pha()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "PHA implied",
                },
            }
        },
        // 0x49
        {
            0x49,
            {
                testcase{
                    .opcode = 0x49,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .eor(0xff)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "EOR #imm immediate",
                },
            }
        },
        // 0x4A
        {
            0x4A,
            {
                testcase{
                    .opcode = 0x4A,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x81, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .lsr()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LSR A accumulator",
                },
            }
        },
        // 0x4B
        {
            0x4B,
            {
                testcase{
                    .opcode = 0x4B,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .alr(0xf0)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ALR #imm immediate",
                },
            }
        },
        // 0x4C
        {
            0x4C,
            {
                testcase{
                    .opcode = 0x4C,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .jmp("trap")
                        .org(0x0800, "trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "JMP abs absolute jump",
                },
            }
        },
        // 0x4D
        {
            0x4D,
            {
                testcase{
                    .opcode = 0x4D,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .eor(absolute, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0x87)
                    .end().compile(),
                    .description = "EOR abs absolute",
                },
            }
        },
        // 0x4E
        {
            0x4E,
            {
                testcase{
                    .opcode = 0x4E,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x81, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .lsr(absolute, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0x82)
                    .end().compile(),
                    .description = "LSR abs absolute",
                },
            }
        },
        // 0x4F
        {
            0x4F,
            {
                testcase{
                    .opcode = 0x4F,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .sre(absolute, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0x82)
                    .end().compile(),
                    .description = "SRE abs absolute",
                },
            }
        },
        // 0x50
        {
            0x50,
            {
                testcase{
                    .opcode = 0x50,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x64, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .bvc("branch_target")
                        .label("trap")
                            .jmp("trap")
                        .org(0x0420, "branch_target")
                    .end().compile(),
                    .description = "BVC rel not taken with a non-fallthrough encoded target",
                },
                testcase{
                    .opcode = 0x50,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .bvc("trap")
                        .org(0x0420, "trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "BVC rel taken without page cross to a non-fallthrough target",
                },
                testcase{
                    .opcode = 0x50,
                    .bytes = 2,
                    .start_at = 0x04f0,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x04f0)
                            .bvc("trap")
                        .org(0x0505, "trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "BVC rel taken with page cross",
                },
            }
        },
        // 0x51
        {
            0x51,
            {
                testcase{
                    .opcode = 0x51,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .eor(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "EOR (zp),Y indirect indexed without page cross",
                },
                testcase{
                    .opcode = 0x51,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .eor(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "EOR (zp),Y indirect indexed with page cross",
                },
            }
        },
        // 0x52
        {
            0x52,
            {
                testcase{
                    .opcode = 0x52,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .kil_opcode(0x52)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "KIL/JAM enters JAM/KIL bus loop",
                },
            }
        },
        // 0x53
        {
            0x53,
            {
                testcase{
                    .opcode = 0x53,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x82)
                        .org(0x0400)
                            .sre(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SRE (zp),Y indirect indexed",
                },
                testcase{
                    .opcode = 0x53,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x82)
                        .org(0x0400)
                            .sre(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SRE (zp),Y indirect indexed with page cross",
                },
            }
        },
        // 0x54
        {
            0x54,
            {
                testcase{
                    .opcode = 0x54,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x87)
                        .org(0x0400)
                            .nop_opcode(0x54, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP zp,X zero page,X with wraparound",
                },
            }
        },
        // 0x55
        {
            0x55,
            {
                testcase{
                    .opcode = 0x55,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x87)
                        .org(0x0400)
                            .eor(zpx, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "EOR zp,X zero page,X with wraparound",
                },
            }
        },
        // 0x56
        {
            0x56,
            {
                testcase{
                    .opcode = 0x56,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x81, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x82)
                        .org(0x0400)
                            .lsr(zpx, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LSR zp,X zero page,X with wraparound",
                },
            }
        },
        // 0x57
        {
            0x57,
            {
                testcase{
                    .opcode = 0x57,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x82)
                        .org(0x0400)
                            .sre(zpx, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SRE zp,X zero page,X with wraparound",
                },
            }
        },
        // 0x58
        {
            0x58,
            {
                testcase{
                    .opcode = 0x58,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .cli()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "CLI implied",
                },
            }
        },
        // 0x59
        {
            0x59,
            {
                testcase{
                    .opcode = 0x59,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .eor(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "EOR abs,Y absolute,Y without page cross",
                },
                testcase{
                    .opcode = 0x59,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .eor(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "EOR abs,Y absolute,Y with page cross",
                },
            }
        },
        // 0x5A
        {
            0x5A,
            {
                testcase{
                    .opcode = 0x5A,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .nop_opcode(0x5A)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP implied",
                },
            }
        },
        // 0x5B
        {
            0x5B,
            {
                testcase{
                    .opcode = 0x5B,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x82)
                        .org(0x0400)
                            .sre(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SRE abs,Y absolute,Y",
                },
                testcase{
                    .opcode = 0x5B,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x82)
                        .org(0x0400)
                            .sre(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SRE abs,Y absolute,Y with page cross",
                },
            }
        },
        // 0x5C
        {
            0x5C,
            {
                testcase{
                    .opcode = 0x5C,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .nop_opcode(0x5C, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP abs,X absolute,X without page cross",
                },
                testcase{
                    .opcode = 0x5C,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x20, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .nop_opcode(0x5C, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP abs,X absolute,X with page cross",
                },
            }
        },
        // 0x5D
        {
            0x5D,
            {
                testcase{
                    .opcode = 0x5D,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .eor(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "EOR abs,X absolute,X without page cross",
                },
                testcase{
                    .opcode = 0x5D,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x20, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .eor(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "EOR abs,X absolute,X with page cross",
                },
            }
        },
        // 0x5E
        {
            0x5E,
            {
                testcase{
                    .opcode = 0x5E,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x81, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x82)
                        .org(0x0400)
                            .lsr(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LSR abs,X absolute,X",
                },
                testcase{
                    .opcode = 0x5E,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x81, .X = 0x20, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x82)
                        .org(0x0400)
                            .lsr(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LSR abs,X absolute,X with page cross",
                },
            }
        },
        // 0x5F
        {
            0x5F,
            {
                testcase{
                    .opcode = 0x5F,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x82)
                        .org(0x0400)
                            .sre(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SRE abs,X absolute,X",
                },
                testcase{
                    .opcode = 0x5F,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x20, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x82)
                        .org(0x0400)
                            .sre(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SRE abs,X absolute,X with page cross",
                },
            }
        },
        // 0x60
        {
            0x60,
            {
                testcase{
                    .opcode = 0x60,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x01fe, "rts_return_address")
                            .dw(sym("trap", -1))
                        .org(0x0400)
                            .rts()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "RTS pulls return address from stack",
                },
            }
        },
        // 0x61
        {
            0x61,
            {
                testcase{
                    .opcode = 0x61,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x10, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_ptr_base")
                        .org(0x0008, "zp_ptr")
                            .dw("value_addr")
                        .org(0x2134, "value_addr")
                            .db(0x15)
                        .org(0x0400)
                            .adc(izx, "zp_ptr_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ADC (zp,X) indexed indirect with zero-page pointer-base wraparound",
                },
            }
        },
        // 0x62
        {
            0x62,
            {
                testcase{
                    .opcode = 0x62,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .kil_opcode(0x62)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "KIL/JAM enters JAM/KIL bus loop",
                },
            }
        },
        // 0x63
        {
            0x63,
            {
                testcase{
                    .opcode = 0x63,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_ptr_base")
                        .org(0x0008, "zp_ptr")
                            .dw("value_addr")
                        .org(0x2134, "value_addr")
                            .db(0x81)
                        .org(0x0400)
                            .rra(izx, "zp_ptr_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "RRA (zp,X) indexed indirect with zero-page pointer-base wraparound",
                },
            }
        },
        // 0x64
        {
            0x64,
            {
                testcase{
                    .opcode = 0x64,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x87)
                        .org(0x0400)
                            .nop_opcode(0x64, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP zp zero page",
                },
            }
        },
        // 0x65
        {
            0x65,
            {
                testcase{
                    .opcode = 0x65,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x00, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x15)
                        .org(0x0400)
                            .adc(zp, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ADC zp zero page",
                },
            }
        },
        // 0x66
        {
            0x66,
            {
                testcase{
                    .opcode = 0x66,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x81, .X = 0x00, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x81)
                        .org(0x0400)
                            .ror(zp, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ROR zp zero page",
                },
            }
        },
        // 0x67
        {
            0x67,
            {
                testcase{
                    .opcode = 0x67,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x81)
                        .org(0x0400)
                            .rra(zp, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "RRA zp zero page",
                },
            }
        },
        // 0x68
        {
            0x68,
            {
                testcase{
                    .opcode = 0x68,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x01fe, "stack_value")
                            .db(0x5a)
                        .org(0x0400)
                            .pla()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "PLA pulls value from stack",
                },
            }
        },
        // 0x69
        {
            0x69,
            {
                testcase{
                    .opcode = 0x69,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x00, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .adc(0x15)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ADC #imm immediate",
                },
                testcase{
                    .opcode = 0x69,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x00, .Y = 0x00, .P = 0x2d, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .adc(0x38)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ADC #imm decimal mode immediate",
                },
            }
        },
        // 0x6A
        {
            0x6A,
            {
                testcase{
                    .opcode = 0x6A,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x81, .X = 0x00, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .ror()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ROR A accumulator",
                },
            }
        },
        // 0x6B
        {
            0x6B,
            {
                testcase{
                    .opcode = 0x6B,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .arr(0x6e)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ARR #imm immediate",
                },
            }
        },
        // 0x6C
        {
            0x6C,
            {
                testcase{
                    .opcode = 0x6C,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0200, "jump_ptr")
                            .dw("trap")
                        .org(0x0400)
                            .jmp(ind, "jump_ptr")
                        .org(0x0800, "trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "JMP (abs) indirect jump",
                },
                testcase{
                    .opcode = 0x6C,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0200, "jump_ptr_high_byte")
                            .db(0x08)
                        .org(0x02ff, "jump_ptr")
                            .db(0x34)
                        .org(0x0400)
                            .jmp(ind, "jump_ptr")
                        .org(0x0834, "trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "JMP (abs) NMOS indirect pointer high-byte wraps at $xxFF",
                },
            }
        },
        // 0x6D
        {
            0x6D,
            {
                testcase{
                    .opcode = 0x6D,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x00, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .adc(absolute, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0x15)
                    .end().compile(),
                    .description = "ADC abs absolute",
                },
            }
        },
        // 0x6E
        {
            0x6E,
            {
                testcase{
                    .opcode = 0x6E,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x81, .X = 0x00, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .ror(absolute, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0x81)
                    .end().compile(),
                    .description = "ROR abs absolute",
                },
            }
        },
        // 0x6F
        {
            0x6F,
            {
                testcase{
                    .opcode = 0x6F,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .rra(absolute, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0x81)
                    .end().compile(),
                    .description = "RRA abs absolute",
                },
            }
        },
        // 0x70
        {
            0x70,
            {
                testcase{
                    .opcode = 0x70,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .bvs("branch_target")
                        .label("trap")
                            .jmp("trap")
                        .org(0x0420, "branch_target")
                    .end().compile(),
                    .description = "BVS rel not taken with a non-fallthrough encoded target",
                },
                testcase{
                    .opcode = 0x70,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x64, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .bvs("trap")
                        .org(0x0420, "trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "BVS rel taken without page cross to a non-fallthrough target",
                },
                testcase{
                    .opcode = 0x70,
                    .bytes = 2,
                    .start_at = 0x04f0,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x64, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x04f0)
                            .bvs("trap")
                        .org(0x0505, "trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "BVS rel taken with page cross",
                },
            }
        },
        // 0x71
        {
            0x71,
            {
                testcase{
                    .opcode = 0x71,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x00, .Y = 0x10, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x15)
                        .org(0x0400)
                            .adc(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ADC (zp),Y indirect indexed without page cross",
                },
                testcase{
                    .opcode = 0x71,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x00, .Y = 0x20, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x15)
                        .org(0x0400)
                            .adc(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ADC (zp),Y indirect indexed with page cross",
                },
            }
        },
        // 0x72
        {
            0x72,
            {
                testcase{
                    .opcode = 0x72,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .kil_opcode(0x72)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "KIL/JAM enters JAM/KIL bus loop",
                },
            }
        },
        // 0x73
        {
            0x73,
            {
                testcase{
                    .opcode = 0x73,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x10, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x81)
                        .org(0x0400)
                            .rra(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "RRA (zp),Y indirect indexed",
                },
                testcase{
                    .opcode = 0x73,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x7f)
                        .org(0x0400)
                            .rra(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "RRA (zp),Y indirect indexed with page cross",
                },
            }
        },
        // 0x74
        {
            0x74,
            {
                testcase{
                    .opcode = 0x74,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x87)
                        .org(0x0400)
                            .nop_opcode(0x74, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP zp,X zero page,X with wraparound",
                },
            }
        },
        // 0x75
        {
            0x75,
            {
                testcase{
                    .opcode = 0x75,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x10, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x15)
                        .org(0x0400)
                            .adc(zpx, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ADC zp,X zero page,X with wraparound",
                },
            }
        },
        // 0x76
        {
            0x76,
            {
                testcase{
                    .opcode = 0x76,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x81, .X = 0x10, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x81)
                        .org(0x0400)
                            .ror(zpx, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ROR zp,X zero page,X with wraparound",
                },
            }
        },
        // 0x77
        {
            0x77,
            {
                testcase{
                    .opcode = 0x77,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x81)
                        .org(0x0400)
                            .rra(zpx, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "RRA zp,X zero page,X with wraparound",
                },
            }
        },
        // 0x78
        {
            0x78,
            {
                testcase{
                    .opcode = 0x78,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .sei()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SEI implied",
                },
            }
        },
        // 0x79
        {
            0x79,
            {
                testcase{
                    .opcode = 0x79,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x00, .Y = 0x10, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x15)
                        .org(0x0400)
                            .adc(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ADC abs,Y absolute,Y without page cross",
                },
                testcase{
                    .opcode = 0x79,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x00, .Y = 0x20, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x15)
                        .org(0x0400)
                            .adc(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ADC abs,Y absolute,Y with page cross",
                },
            }
        },
        // 0x7A
        {
            0x7A,
            {
                testcase{
                    .opcode = 0x7A,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .nop_opcode(0x7A)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP implied",
                },
            }
        },
        // 0x7B
        {
            0x7B,
            {
                testcase{
                    .opcode = 0x7B,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x10, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x81)
                        .org(0x0400)
                            .rra(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "RRA abs,Y absolute,Y",
                },
                testcase{
                    .opcode = 0x7B,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x7f)
                        .org(0x0400)
                            .rra(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "RRA abs,Y absolute,Y with page cross",
                },
            }
        },
        // 0x7C
        {
            0x7C,
            {
                testcase{
                    .opcode = 0x7C,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .nop_opcode(0x7C, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP abs,X absolute,X without page cross",
                },
                testcase{
                    .opcode = 0x7C,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x20, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .nop_opcode(0x7C, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP abs,X absolute,X with page cross",
                },
            }
        },
        // 0x7D
        {
            0x7D,
            {
                testcase{
                    .opcode = 0x7D,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x10, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x15)
                        .org(0x0400)
                            .adc(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ADC abs,X absolute,X without page cross",
                },
                testcase{
                    .opcode = 0x7D,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x20, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x15)
                        .org(0x0400)
                            .adc(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ADC abs,X absolute,X with page cross",
                },
            }
        },
        // 0x7E
        {
            0x7E,
            {
                testcase{
                    .opcode = 0x7E,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x81, .X = 0x10, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x81)
                        .org(0x0400)
                            .ror(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ROR abs,X absolute,X",
                },
                testcase{
                    .opcode = 0x7E,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x81, .X = 0x20, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x7f)
                        .org(0x0400)
                            .ror(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ROR abs,X absolute,X with page cross",
                },
            }
        },
        // 0x7F
        {
            0x7F,
            {
                testcase{
                    .opcode = 0x7F,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x81)
                        .org(0x0400)
                            .rra(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "RRA abs,X absolute,X",
                },
                testcase{
                    .opcode = 0x7F,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x20, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x7f)
                        .org(0x0400)
                            .rra(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "RRA abs,X absolute,X with page cross",
                },
            }
        },
        // 0x80
        {
            0x80,
            {
                testcase{
                    .opcode = 0x80,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .nop_opcode(0x80, 0x7f)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP #imm immediate",
                },
            }
        },
        // 0x81
        {
            0x81,
            {
                testcase{
                    .opcode = 0x81,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x5a, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_ptr_base")
                        .org(0x0008, "zp_ptr")
                            .dw("store_addr")
                        .org(0x2134, "store_addr")
                        .org(0x0400)
                            .sta(izx, "zp_ptr_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "STA (zp,X) indexed indirect with zero-page pointer-base wraparound",
                },
            }
        },
        // 0x82
        {
            0x82,
            {
                testcase{
                    .opcode = 0x82,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .nop_opcode(0x82, 0x7f)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP #imm immediate",
                },
            }
        },
        // 0x83
        {
            0x83,
            {
                testcase{
                    .opcode = 0x83,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xf3, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0x9a,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_ptr_base")
                        .org(0x0008, "zp_ptr")
                            .dw("store_addr")
                        .org(0x2134, "store_addr")
                        .org(0x0400)
                            .sax(izx, "zp_ptr_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SAX (zp,X) indexed indirect with zero-page pointer-base wraparound",
                },
            }
        },
        // 0x84
        {
            0x84,
            {
                testcase{
                    .opcode = 0x84,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x7e, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "store_zp")
                        .org(0x0400)
                            .sty(zp, "store_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "STY zp zero page",
                },
            }
        },
        // 0x85
        {
            0x85,
            {
                testcase{
                    .opcode = 0x85,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x5a, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "store_zp")
                        .org(0x0400)
                            .sta(zp, "store_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "STA zp zero page",
                },
            }
        },
        // 0x86
        {
            0x86,
            {
                testcase{
                    .opcode = 0x86,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x3c, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "store_zp")
                        .org(0x0400)
                            .stx(zp, "store_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "STX zp zero page",
                },
            }
        },
        // 0x87
        {
            0x87,
            {
                testcase{
                    .opcode = 0x87,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xf3, .X = 0xcc, .Y = 0x00, .P = 0x24, .S = 0x9a,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "store_zp")
                        .org(0x0400)
                            .sax(zp, "store_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SAX zp zero page",
                },
            }
        },
        // 0x88
        {
            0x88,
            {
                testcase{
                    .opcode = 0x88,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .dey()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "DEY implied",
                },
            }
        },
        // 0x89
        {
            0x89,
            {
                testcase{
                    .opcode = 0x89,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .nop_opcode(0x89, 0x7f)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP #imm immediate",
                },
            }
        },
        // 0x8A
        {
            0x8A,
            {
                testcase{
                    .opcode = 0x8A,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x7c, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .txa()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "TXA implied",
                },
            }
        },
        // 0x8B
        {
            0x8B,
            {
                testcase{
                    .opcode = 0x8B,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .xaa(0xee)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "XAA #imm immediate",
                },
            }
        },
        // 0x8C
        {
            0x8C,
            {
                testcase{
                    .opcode = 0x8C,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x7e, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .sty(absolute, "store_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "store_addr")
                    .end().compile(),
                    .description = "STY abs absolute",
                },
            }
        },
        // 0x8D
        {
            0x8D,
            {
                testcase{
                    .opcode = 0x8D,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x5a, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .sta(absolute, "store_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "store_addr")
                    .end().compile(),
                    .description = "STA abs absolute",
                },
            }
        },
        // 0x8E
        {
            0x8E,
            {
                testcase{
                    .opcode = 0x8E,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x3c, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .stx(absolute, "store_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "store_addr")
                    .end().compile(),
                    .description = "STX abs absolute",
                },
            }
        },
        // 0x8F
        {
            0x8F,
            {
                testcase{
                    .opcode = 0x8F,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xf3, .X = 0xcc, .Y = 0x00, .P = 0x24, .S = 0x9a,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .sax(absolute, "store_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "store_addr")
                    .end().compile(),
                    .description = "SAX abs absolute",
                },
            }
        },
        // 0x90
        {
            0x90,
            {
                testcase{
                    .opcode = 0x90,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .bcc("branch_target")
                        .label("trap")
                            .jmp("trap")
                        .org(0x0420, "branch_target")
                    .end().compile(),
                    .description = "BCC rel not taken with a non-fallthrough encoded target",
                },
                testcase{
                    .opcode = 0x90,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .bcc("trap")
                        .org(0x0420, "trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "BCC rel taken without page cross to a non-fallthrough target",
                },
                testcase{
                    .opcode = 0x90,
                    .bytes = 2,
                    .start_at = 0x04f0,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x04f0)
                            .bcc("trap")
                        .org(0x0505, "trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "BCC rel taken with page cross",
                },
            }
        },
        // 0x91
        {
            0x91,
            {
                testcase{
                    .opcode = 0x91,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x5a, .X = 0x00, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x2100, "value_base")
                        .org(0x2110, "store_addr")
                        .org(0x0400)
                            .sta(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "STA (zp),Y indirect indexed",
                },
                testcase{
                    .opcode = 0x91,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x5a, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x21f0, "value_base")
                        .org(0x2210, "store_addr")
                        .org(0x0400)
                            .sta(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "STA (zp),Y indirect indexed with page cross",
                },
            }
        },
        // 0x92
        {
            0x92,
            {
                testcase{
                    .opcode = 0x92,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .kil_opcode(0x92)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "KIL/JAM enters JAM/KIL bus loop",
                },
            }
        },
        // 0x93
        {
            0x93,
            {
                testcase{
                    .opcode = 0x93,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xf3, .X = 0xcc, .Y = 0x10, .P = 0x24, .S = 0x9a,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x21f0, "value_base")
                        .org(0x2210, "store_addr")
                        .org(0x0400)
                            .ahx(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "AHX (zp),Y unstable indirect indexed store with page cross",
                },
                testcase{
                    .opcode = 0x93,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xf3, .X = 0xcc, .Y = 0x10, .P = 0x24, .S = 0x9a,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x2100, "value_base")
                        .org(0x2110, "store_addr")
                        .org(0x0400)
                            .ahx(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "AHX (zp),Y unstable indirect indexed without page cross",
                },
            }
        },
        // 0x94
        {
            0x94,
            {
                testcase{
                    .opcode = 0x94,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x10, .Y = 0x7e, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "store_zp")
                        .org(0x0400)
                            .sty(zpx, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "STY zp,X zero page,X with wraparound",
                },
            }
        },
        // 0x95
        {
            0x95,
            {
                testcase{
                    .opcode = 0x95,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x5a, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "store_zp")
                        .org(0x0400)
                            .sta(zpx, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "STA zp,X zero page,X with wraparound",
                },
            }
        },
        // 0x96
        {
            0x96,
            {
                testcase{
                    .opcode = 0x96,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x3c, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "store_zp")
                        .org(0x0400)
                            .stx(zpy, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "STX zp,Y zero page,Y with wraparound",
                },
            }
        },
        // 0x97
        {
            0x97,
            {
                testcase{
                    .opcode = 0x97,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xf3, .X = 0xcc, .Y = 0x10, .P = 0x24, .S = 0x9a,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "store_zp")
                        .org(0x0400)
                            .sax(zpy, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SAX zp,Y zero page,Y with wraparound",
                },
            }
        },
        // 0x98
        {
            0x98,
            {
                testcase{
                    .opcode = 0x98,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x6d, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .tya()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "TYA implied",
                },
            }
        },
        // 0x99
        {
            0x99,
            {
                testcase{
                    .opcode = 0x99,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x5a, .X = 0x00, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "store_addr")
                        .org(0x0400)
                            .sta(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "STA abs,Y absolute,Y",
                },
                testcase{
                    .opcode = 0x99,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x5a, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "store_addr")
                        .org(0x0400)
                            .sta(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "STA abs,Y absolute,Y with page cross",
                },
            }
        },
        // 0x9A
        {
            0x9A,
            {
                testcase{
                    .opcode = 0x9A,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x7c, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .txs()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "TXS implied",
                },
            }
        },
        // 0x9B
        {
            0x9B,
            {
                testcase{
                    .opcode = 0x9B,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xf3, .X = 0xcc, .Y = 0x10, .P = 0x24, .S = 0x9a,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "store_addr")
                        .org(0x0400)
                            .tas(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "TAS abs,Y unstable absolute,Y store with page cross",
                },
                testcase{
                    .opcode = 0x9B,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xf3, .X = 0xcc, .Y = 0x10, .P = 0x24, .S = 0x9a,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "store_addr")
                        .org(0x0400)
                            .tas(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "TAS abs,Y unstable absolute,Y store without page cross",
                },
            }
        },
        // 0x9C
        {
            0x9C,
            {
                testcase{
                    .opcode = 0x9C,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x10, .Y = 0xcf, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "store_addr")
                        .org(0x0400)
                            .shy(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SHY abs,X unstable absolute,X store with page cross",
                },
                testcase{
                    .opcode = 0x9C,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x10, .Y = 0xcf, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "store_addr")
                        .org(0x0400)
                            .shy(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SHY abs,X unstable absolute,X store without page cross",
                },
            }
        },
        // 0x9D
        {
            0x9D,
            {
                testcase{
                    .opcode = 0x9D,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x5a, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "store_addr")
                        .org(0x0400)
                            .sta(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "STA abs,X absolute,X",
                },
                testcase{
                    .opcode = 0x9D,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x5a, .X = 0x20, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "store_addr")
                        .org(0x0400)
                            .sta(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "STA abs,X absolute,X with page cross",
                },
            }
        },
        // 0x9E
        {
            0x9E,
            {
                testcase{
                    .opcode = 0x9E,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0xcf, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "store_addr")
                        .org(0x0400)
                            .shx(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SHX abs,Y unstable absolute,Y store with page cross",
                },
                testcase{
                    .opcode = 0x9E,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0xcf, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "store_addr")
                        .org(0x0400)
                            .shx(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SHX abs,Y unstable absolute,Y store without page cross",
                },
            }
        },
        // 0x9F
        {
            0x9F,
            {
                testcase{
                    .opcode = 0x9F,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xf3, .X = 0xcc, .Y = 0x10, .P = 0x24, .S = 0x9a,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "store_addr")
                        .org(0x0400)
                            .ahx(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "AHX abs,Y unstable absolute,Y store with page cross",
                },
                testcase{
                    .opcode = 0x9F,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xf3, .X = 0xcc, .Y = 0x10, .P = 0x24, .S = 0x9a,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "store_addr")
                        .org(0x0400)
                            .ahx(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "AHX abs,Y unstable absolute,Y store without page cross",
                },
            }
        },
        // 0xA0
        {
            0xA0,
            {
                testcase{
                    .opcode = 0xA0,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .ldy(0x6d)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LDY #imm immediate",
                },
            }
        },
        // 0xA1
        {
            0xA1,
            {
                testcase{
                    .opcode = 0xA1,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_ptr_base")
                        .org(0x0008, "zp_ptr")
                            .dw("value_addr")
                        .org(0x2134, "value_addr")
                            .db(0x8e)
                        .org(0x0400)
                            .lda(izx, "zp_ptr_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LDA (zp,X) indexed indirect with zero-page pointer-base wraparound",
                },
            }
        },
        // 0xA2
        {
            0xA2,
            {
                testcase{
                    .opcode = 0xA2,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .ldx(0x7c)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LDX #imm immediate",
                },
            }
        },
        // 0xA3
        {
            0xA3,
            {
                testcase{
                    .opcode = 0xA3,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_ptr_base")
                        .org(0x0008, "zp_ptr")
                            .dw("value_addr")
                        .org(0x2134, "value_addr")
                            .db(0x8e)
                        .org(0x0400)
                            .lax(izx, "zp_ptr_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LAX (zp,X) indexed indirect with zero-page pointer-base wraparound",
                },
            }
        },
        // 0xA4
        {
            0xA4,
            {
                testcase{
                    .opcode = 0xA4,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x8e)
                        .org(0x0400)
                            .ldy(zp, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LDY zp zero page",
                },
            }
        },
        // 0xA5
        {
            0xA5,
            {
                testcase{
                    .opcode = 0xA5,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x8e)
                        .org(0x0400)
                            .lda(zp, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LDA zp zero page",
                },
            }
        },
        // 0xA6
        {
            0xA6,
            {
                testcase{
                    .opcode = 0xA6,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x8e)
                        .org(0x0400)
                            .ldx(zp, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LDX zp zero page",
                },
            }
        },
        // 0xA7
        {
            0xA7,
            {
                testcase{
                    .opcode = 0xA7,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x8e)
                        .org(0x0400)
                            .lax(zp, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LAX zp zero page",
                },
            }
        },
        // 0xA8
        {
            0xA8,
            {
                testcase{
                    .opcode = 0xA8,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x8e, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .tay()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "TAY implied",
                },
            }
        },
        // 0xA9
        {
            0xA9,
            {
                testcase{
                    .opcode = 0xA9,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .lda(0x8e)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LDA #imm immediate",
                },
            }
        },
        // 0xAA
        {
            0xAA,
            {
                testcase{
                    .opcode = 0xAA,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x8e, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .tax()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "TAX implied",
                },
            }
        },
        // 0xAB
        {
            0xAB,
            {
                testcase{
                    .opcode = 0xAB,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .lxa(0xf3)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LXA #imm immediate",
                },
            }
        },
        // 0xAC
        {
            0xAC,
            {
                testcase{
                    .opcode = 0xAC,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .ldy(absolute, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0x8e)
                    .end().compile(),
                    .description = "LDY abs absolute",
                },
            }
        },
        // 0xAD
        {
            0xAD,
            {
                testcase{
                    .opcode = 0xAD,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .lda(absolute, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0x8e)
                    .end().compile(),
                    .description = "LDA abs absolute",
                },
            }
        },
        // 0xAE
        {
            0xAE,
            {
                testcase{
                    .opcode = 0xAE,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .ldx(absolute, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0x8e)
                    .end().compile(),
                    .description = "LDX abs absolute",
                },
            }
        },
        // 0xAF
        {
            0xAF,
            {
                testcase{
                    .opcode = 0xAF,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .lax(absolute, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0x8e)
                    .end().compile(),
                    .description = "LAX abs absolute",
                },
            }
        },
        // 0xB0
        {
            0xB0,
            {
                testcase{
                    .opcode = 0xB0,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .bcs("branch_target")
                        .label("trap")
                            .jmp("trap")
                        .org(0x0420, "branch_target")
                    .end().compile(),
                    .description = "BCS rel not taken with a non-fallthrough encoded target",
                },
                testcase{
                    .opcode = 0xB0,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .bcs("trap")
                        .org(0x0420, "trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "BCS rel taken without page cross to a non-fallthrough target",
                },
                testcase{
                    .opcode = 0xB0,
                    .bytes = 2,
                    .start_at = 0x04f0,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x04f0)
                            .bcs("trap")
                        .org(0x0505, "trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "BCS rel taken with page cross",
                },
            }
        },
        // 0xB1
        {
            0xB1,
            {
                testcase{
                    .opcode = 0xB1,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x8e)
                        .org(0x0400)
                            .lda(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LDA (zp),Y indirect indexed without page cross",
                },
                testcase{
                    .opcode = 0xB1,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x8e)
                        .org(0x0400)
                            .lda(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LDA (zp),Y indirect indexed with page cross",
                },
            }
        },
        // 0xB2
        {
            0xB2,
            {
                testcase{
                    .opcode = 0xB2,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .kil_opcode(0xB2)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "KIL/JAM enters JAM/KIL bus loop",
                },
            }
        },
        // 0xB3
        {
            0xB3,
            {
                testcase{
                    .opcode = 0xB3,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x8e)
                        .org(0x0400)
                            .lax(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LAX (zp),Y indirect indexed without page cross",
                },
                testcase{
                    .opcode = 0xB3,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x8e)
                        .org(0x0400)
                            .lax(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LAX (zp),Y indirect indexed with page cross",
                },
            }
        },
        // 0xB4
        {
            0xB4,
            {
                testcase{
                    .opcode = 0xB4,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x8e)
                        .org(0x0400)
                            .ldy(zpx, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LDY zp,X zero page,X with wraparound",
                },
            }
        },
        // 0xB5
        {
            0xB5,
            {
                testcase{
                    .opcode = 0xB5,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x8e)
                        .org(0x0400)
                            .lda(zpx, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LDA zp,X zero page,X with wraparound",
                },
            }
        },
        // 0xB6
        {
            0xB6,
            {
                testcase{
                    .opcode = 0xB6,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x8e)
                        .org(0x0400)
                            .ldx(zpy, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LDX zp,Y zero page,Y with wraparound",
                },
            }
        },
        // 0xB7
        {
            0xB7,
            {
                testcase{
                    .opcode = 0xB7,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x8e)
                        .org(0x0400)
                            .lax(zpy, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LAX zp,Y zero page,Y with wraparound",
                },
            }
        },
        // 0xB8
        {
            0xB8,
            {
                testcase{
                    .opcode = 0xB8,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .clv()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "CLV implied",
                },
            }
        },
        // 0xB9
        {
            0xB9,
            {
                testcase{
                    .opcode = 0xB9,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x8e)
                        .org(0x0400)
                            .lda(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LDA abs,Y absolute,Y without page cross",
                },
                testcase{
                    .opcode = 0xB9,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x8e)
                        .org(0x0400)
                            .lda(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LDA abs,Y absolute,Y with page cross",
                },
            }
        },
        // 0xBA
        {
            0xBA,
            {
                testcase{
                    .opcode = 0xBA,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0x7b,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .tsx()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "TSX implied",
                },
            }
        },
        // 0xBB
        {
            0xBB,
            {
                testcase{
                    .opcode = 0xBB,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x10, .P = 0x24, .S = 0xf3,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x8e)
                        .org(0x0400)
                            .las(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LAS abs,Y absolute,Y without page cross",
                },
                testcase{
                    .opcode = 0xBB,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xf3,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x8e)
                        .org(0x0400)
                            .las(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LAS abs,Y absolute,Y with page cross",
                },
            }
        },
        // 0xBC
        {
            0xBC,
            {
                testcase{
                    .opcode = 0xBC,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x8e)
                        .org(0x0400)
                            .ldy(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LDY abs,X absolute,X without page cross",
                },
                testcase{
                    .opcode = 0xBC,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x20, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x8e)
                        .org(0x0400)
                            .ldy(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LDY abs,X absolute,X with page cross",
                },
            }
        },
        // 0xBD
        {
            0xBD,
            {
                testcase{
                    .opcode = 0xBD,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x8e)
                        .org(0x0400)
                            .lda(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LDA abs,X absolute,X without page cross",
                },
                testcase{
                    .opcode = 0xBD,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x20, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x8e)
                        .org(0x0400)
                            .lda(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LDA abs,X absolute,X with page cross",
                },
            }
        },
        // 0xBE
        {
            0xBE,
            {
                testcase{
                    .opcode = 0xBE,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x8e)
                        .org(0x0400)
                            .ldx(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LDX abs,Y absolute,Y without page cross",
                },
                testcase{
                    .opcode = 0xBE,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x8e)
                        .org(0x0400)
                            .ldx(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LDX abs,Y absolute,Y with page cross",
                },
            }
        },
        // 0xBF
        {
            0xBF,
            {
                testcase{
                    .opcode = 0xBF,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x8e)
                        .org(0x0400)
                            .lax(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LAX abs,Y absolute,Y without page cross",
                },
                testcase{
                    .opcode = 0xBF,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x8e)
                        .org(0x0400)
                            .lax(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "LAX abs,Y absolute,Y with page cross",
                },
            }
        },
        // 0xC0
        {
            0xC0,
            {
                testcase{
                    .opcode = 0xC0,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .cpy(0x30)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "CPY #imm immediate",
                },
            }
        },
        // 0xC1
        {
            0xC1,
            {
                testcase{
                    .opcode = 0xC1,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_ptr_base")
                        .org(0x0008, "zp_ptr")
                            .dw("value_addr")
                        .org(0x2134, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .cmp(izx, "zp_ptr_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "CMP (zp,X) indexed indirect with zero-page pointer-base wraparound",
                },
            }
        },
        // 0xC2
        {
            0xC2,
            {
                testcase{
                    .opcode = 0xC2,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .nop_opcode(0xC2, 0x7f)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP #imm immediate",
                },
            }
        },
        // 0xC3
        {
            0xC3,
            {
                testcase{
                    .opcode = 0xC3,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_ptr_base")
                        .org(0x0008, "zp_ptr")
                            .dw("value_addr")
                        .org(0x2134, "value_addr")
                            .db(0x80)
                        .org(0x0400)
                            .dcp(izx, "zp_ptr_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "DCP (zp,X) indexed indirect with zero-page pointer-base wraparound",
                },
            }
        },
        // 0xC4
        {
            0xC4,
            {
                testcase{
                    .opcode = 0xC4,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x87)
                        .org(0x0400)
                            .cpy(zp, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "CPY zp zero page",
                },
            }
        },
        // 0xC5
        {
            0xC5,
            {
                testcase{
                    .opcode = 0xC5,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x87)
                        .org(0x0400)
                            .cmp(zp, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "CMP zp zero page",
                },
            }
        },
        // 0xC6
        {
            0xC6,
            {
                testcase{
                    .opcode = 0xC6,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x80)
                        .org(0x0400)
                            .dec(zp, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "DEC zp zero page",
                },
            }
        },
        // 0xC7
        {
            0xC7,
            {
                testcase{
                    .opcode = 0xC7,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x80)
                        .org(0x0400)
                            .dcp(zp, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "DCP zp zero page",
                },
            }
        },
        // 0xC8
        {
            0xC8,
            {
                testcase{
                    .opcode = 0xC8,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .iny()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "INY implied",
                },
            }
        },
        // 0xC9
        {
            0xC9,
            {
                testcase{
                    .opcode = 0xC9,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .cmp(0x40)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "CMP #imm immediate",
                },
            }
        },
        // 0xCA
        {
            0xCA,
            {
                testcase{
                    .opcode = 0xCA,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .dex()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "DEX implied",
                },
            }
        },
        // 0xCB
        {
            0xCB,
            {
                testcase{
                    .opcode = 0xCB,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .axs(0x33)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "AXS #imm immediate",
                },
            }
        },
        // 0xCC
        {
            0xCC,
            {
                testcase{
                    .opcode = 0xCC,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .cpy(absolute, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0x87)
                    .end().compile(),
                    .description = "CPY abs absolute",
                },
            }
        },
        // 0xCD
        {
            0xCD,
            {
                testcase{
                    .opcode = 0xCD,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .cmp(absolute, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0x87)
                    .end().compile(),
                    .description = "CMP abs absolute",
                },
            }
        },
        // 0xCE
        {
            0xCE,
            {
                testcase{
                    .opcode = 0xCE,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .dec(absolute, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0x80)
                    .end().compile(),
                    .description = "DEC abs absolute",
                },
            }
        },
        // 0xCF
        {
            0xCF,
            {
                testcase{
                    .opcode = 0xCF,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .dcp(absolute, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0x80)
                    .end().compile(),
                    .description = "DCP abs absolute",
                },
            }
        },
        // 0xD0
        {
            0xD0,
            {
                testcase{
                    .opcode = 0xD0,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x26, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .bne("branch_target")
                        .label("trap")
                            .jmp("trap")
                        .org(0x0420, "branch_target")
                    .end().compile(),
                    .description = "BNE rel not taken with a non-fallthrough encoded target",
                },
                testcase{
                    .opcode = 0xD0,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .bne("trap")
                        .org(0x0420, "trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "BNE rel taken without page cross to a non-fallthrough target",
                },
                testcase{
                    .opcode = 0xD0,
                    .bytes = 2,
                    .start_at = 0x04f0,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x04f0)
                            .bne("trap")
                        .org(0x0505, "trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "BNE rel taken with page cross",
                },
            }
        },
        // 0xD1
        {
            0xD1,
            {
                testcase{
                    .opcode = 0xD1,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .cmp(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "CMP (zp),Y indirect indexed without page cross",
                },
                testcase{
                    .opcode = 0xD1,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .cmp(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "CMP (zp),Y indirect indexed with page cross",
                },
            }
        },
        // 0xD2
        {
            0xD2,
            {
                testcase{
                    .opcode = 0xD2,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .kil_opcode(0xD2)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "KIL/JAM enters JAM/KIL bus loop",
                },
            }
        },
        // 0xD3
        {
            0xD3,
            {
                testcase{
                    .opcode = 0xD3,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x80)
                        .org(0x0400)
                            .dcp(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "DCP (zp),Y indirect indexed",
                },
                testcase{
                    .opcode = 0xD3,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x80)
                        .org(0x0400)
                            .dcp(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "DCP (zp),Y indirect indexed with page cross",
                },
            }
        },
        // 0xD4
        {
            0xD4,
            {
                testcase{
                    .opcode = 0xD4,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x87)
                        .org(0x0400)
                            .nop_opcode(0xD4, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP zp,X zero page,X with wraparound",
                },
            }
        },
        // 0xD5
        {
            0xD5,
            {
                testcase{
                    .opcode = 0xD5,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x87)
                        .org(0x0400)
                            .cmp(zpx, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "CMP zp,X zero page,X with wraparound",
                },
            }
        },
        // 0xD6
        {
            0xD6,
            {
                testcase{
                    .opcode = 0xD6,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x80)
                        .org(0x0400)
                            .dec(zpx, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "DEC zp,X zero page,X with wraparound",
                },
            }
        },
        // 0xD7
        {
            0xD7,
            {
                testcase{
                    .opcode = 0xD7,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x80)
                        .org(0x0400)
                            .dcp(zpx, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "DCP zp,X zero page,X with wraparound",
                },
            }
        },
        // 0xD8
        {
            0xD8,
            {
                testcase{
                    .opcode = 0xD8,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .cld()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "CLD implied",
                },
            }
        },
        // 0xD9
        {
            0xD9,
            {
                testcase{
                    .opcode = 0xD9,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .cmp(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "CMP abs,Y absolute,Y without page cross",
                },
                testcase{
                    .opcode = 0xD9,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .cmp(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "CMP abs,Y absolute,Y with page cross",
                },
            }
        },
        // 0xDA
        {
            0xDA,
            {
                testcase{
                    .opcode = 0xDA,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .nop_opcode(0xDA)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP implied",
                },
            }
        },
        // 0xDB
        {
            0xDB,
            {
                testcase{
                    .opcode = 0xDB,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x10, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x80)
                        .org(0x0400)
                            .dcp(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "DCP abs,Y absolute,Y",
                },
                testcase{
                    .opcode = 0xDB,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x80)
                        .org(0x0400)
                            .dcp(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "DCP abs,Y absolute,Y with page cross",
                },
            }
        },
        // 0xDC
        {
            0xDC,
            {
                testcase{
                    .opcode = 0xDC,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .nop_opcode(0xDC, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP abs,X absolute,X without page cross",
                },
                testcase{
                    .opcode = 0xDC,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x20, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .nop_opcode(0xDC, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP abs,X absolute,X with page cross",
                },
            }
        },
        // 0xDD
        {
            0xDD,
            {
                testcase{
                    .opcode = 0xDD,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .cmp(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "CMP abs,X absolute,X without page cross",
                },
                testcase{
                    .opcode = 0xDD,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x20, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .cmp(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "CMP abs,X absolute,X with page cross",
                },
            }
        },
        // 0xDE
        {
            0xDE,
            {
                testcase{
                    .opcode = 0xDE,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x80)
                        .org(0x0400)
                            .dec(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "DEC abs,X absolute,X",
                },
                testcase{
                    .opcode = 0xDE,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x20, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x80)
                        .org(0x0400)
                            .dec(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "DEC abs,X absolute,X with page cross",
                },
            }
        },
        // 0xDF
        {
            0xDF,
            {
                testcase{
                    .opcode = 0xDF,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x80)
                        .org(0x0400)
                            .dcp(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "DCP abs,X absolute,X",
                },
                testcase{
                    .opcode = 0xDF,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x20, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x80)
                        .org(0x0400)
                            .dcp(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "DCP abs,X absolute,X with page cross",
                },
            }
        },
        // 0xE0
        {
            0xE0,
            {
                testcase{
                    .opcode = 0xE0,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .cpx(0x30)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "CPX #imm immediate",
                },
            }
        },
        // 0xE1
        {
            0xE1,
            {
                testcase{
                    .opcode = 0xE1,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x10, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_ptr_base")
                        .org(0x0008, "zp_ptr")
                            .dw("value_addr")
                        .org(0x2134, "value_addr")
                            .db(0x15)
                        .org(0x0400)
                            .sbc(izx, "zp_ptr_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SBC (zp,X) indexed indirect with zero-page pointer-base wraparound",
                },
            }
        },
        // 0xE2
        {
            0xE2,
            {
                testcase{
                    .opcode = 0xE2,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .nop_opcode(0xE2, 0x7f)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP #imm immediate",
                },
            }
        },
        // 0xE3
        {
            0xE3,
            {
                testcase{
                    .opcode = 0xE3,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_ptr_base")
                        .org(0x0008, "zp_ptr")
                            .dw("value_addr")
                        .org(0x2134, "value_addr")
                            .db(0x7f)
                        .org(0x0400)
                            .isc(izx, "zp_ptr_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ISC (zp,X) indexed indirect with zero-page pointer-base wraparound",
                },
            }
        },
        // 0xE4
        {
            0xE4,
            {
                testcase{
                    .opcode = 0xE4,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x87)
                        .org(0x0400)
                            .cpx(zp, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "CPX zp zero page",
                },
            }
        },
        // 0xE5
        {
            0xE5,
            {
                testcase{
                    .opcode = 0xE5,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x00, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x15)
                        .org(0x0400)
                            .sbc(zp, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SBC zp zero page",
                },
            }
        },
        // 0xE6
        {
            0xE6,
            {
                testcase{
                    .opcode = 0xE6,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x7f)
                        .org(0x0400)
                            .inc(zp, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "INC zp zero page",
                },
            }
        },
        // 0xE7
        {
            0xE7,
            {
                testcase{
                    .opcode = 0xE7,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0080, "value_zp")
                            .db(0x7f)
                        .org(0x0400)
                            .isc(zp, "value_zp")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ISC zp zero page",
                },
            }
        },
        // 0xE8
        {
            0xE8,
            {
                testcase{
                    .opcode = 0xE8,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .inx()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "INX implied",
                },
            }
        },
        // 0xE9
        {
            0xE9,
            {
                testcase{
                    .opcode = 0xE9,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x00, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .sbc(0x12)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SBC #imm immediate",
                },
                testcase{
                    .opcode = 0xE9,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x00, .Y = 0x00, .P = 0x2d, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .sbc(0x12)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SBC #imm decimal mode immediate",
                },
            }
        },
        // 0xEA
        {
            0xEA,
            {
                testcase{
                    .opcode = 0xEA,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .nop()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP implied",
                },
            }
        },
        // 0xEB
        {
            0xEB,
            {
                testcase{
                    .opcode = 0xEB,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x00, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .sbc_opcode(0xEB, 0x12)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SBC #imm immediate",
                },
                testcase{
                    .opcode = 0xEB,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x00, .Y = 0x00, .P = 0x2d, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .sbc_opcode(0xEB, 0x12)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SBC #imm decimal mode immediate",
                },
            }
        },
        // 0xEC
        {
            0xEC,
            {
                testcase{
                    .opcode = 0xEC,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .cpx(absolute, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0x87)
                    .end().compile(),
                    .description = "CPX abs absolute",
                },
            }
        },
        // 0xED
        {
            0xED,
            {
                testcase{
                    .opcode = 0xED,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x00, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .sbc(absolute, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0x15)
                    .end().compile(),
                    .description = "SBC abs absolute",
                },
            }
        },
        // 0xEE
        {
            0xEE,
            {
                testcase{
                    .opcode = 0xEE,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .inc(absolute, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0x7f)
                    .end().compile(),
                    .description = "INC abs absolute",
                },
            }
        },
        // 0xEF
        {
            0xEF,
            {
                testcase{
                    .opcode = 0xEF,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .isc(absolute, "value_addr")
                        .label("trap")
                            .jmp("trap")
                        .org(0x2134, "value_addr")
                            .db(0x7f)
                    .end().compile(),
                    .description = "ISC abs absolute",
                },
            }
        },
        // 0xF0
        {
            0xF0,
            {
                testcase{
                    .opcode = 0xF0,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .beq("branch_target")
                        .label("trap")
                            .jmp("trap")
                        .org(0x0420, "branch_target")
                    .end().compile(),
                    .description = "BEQ rel not taken with a non-fallthrough encoded target",
                },
                testcase{
                    .opcode = 0xF0,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x26, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .beq("trap")
                        .org(0x0420, "trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "BEQ rel taken without page cross to a non-fallthrough target",
                },
                testcase{
                    .opcode = 0xF0,
                    .bytes = 2,
                    .start_at = 0x04f0,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x26, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x04f0)
                            .beq("trap")
                        .org(0x0505, "trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "BEQ rel taken with page cross",
                },
            }
        },
        // 0xF1
        {
            0xF1,
            {
                testcase{
                    .opcode = 0xF1,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x00, .Y = 0x10, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x15)
                        .org(0x0400)
                            .sbc(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SBC (zp),Y indirect indexed without page cross",
                },
                testcase{
                    .opcode = 0xF1,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x00, .Y = 0x20, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x15)
                        .org(0x0400)
                            .sbc(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SBC (zp),Y indirect indexed with page cross",
                },
            }
        },
        // 0xF2
        {
            0xF2,
            {
                testcase{
                    .opcode = 0xF2,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .kil_opcode(0xF2)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "KIL/JAM enters JAM/KIL bus loop",
                },
            }
        },
        // 0xF3
        {
            0xF3,
            {
                testcase{
                    .opcode = 0xF3,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x10, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x7f)
                        .org(0x0400)
                            .isc(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ISC (zp),Y indirect indexed",
                },
                testcase{
                    .opcode = 0xF3,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0020, "zp_ptr")
                            .dw("value_base")
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x7f)
                        .org(0x0400)
                            .isc(izy, "zp_ptr")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ISC (zp),Y indirect indexed with page cross",
                },
            }
        },
        // 0xF4
        {
            0xF4,
            {
                testcase{
                    .opcode = 0xF4,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x87)
                        .org(0x0400)
                            .nop_opcode(0xF4, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP zp,X zero page,X with wraparound",
                },
            }
        },
        // 0xF5
        {
            0xF5,
            {
                testcase{
                    .opcode = 0xF5,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x10, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x15)
                        .org(0x0400)
                            .sbc(zpx, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SBC zp,X zero page,X with wraparound",
                },
            }
        },
        // 0xF6
        {
            0xF6,
            {
                testcase{
                    .opcode = 0xF6,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x7f)
                        .org(0x0400)
                            .inc(zpx, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "INC zp,X zero page,X with wraparound",
                },
            }
        },
        // 0xF7
        {
            0xF7,
            {
                testcase{
                    .opcode = 0xF7,
                    .bytes = 2,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x00f8, "zp_base")
                        .org(0x0008, "value_zp")
                            .db(0x7f)
                        .org(0x0400)
                            .isc(zpx, "zp_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ISC zp,X zero page,X with wraparound",
                },
            }
        },
        // 0xF8
        {
            0xF8,
            {
                testcase{
                    .opcode = 0xF8,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .sed()
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SED implied",
                },
            }
        },
        // 0xF9
        {
            0xF9,
            {
                testcase{
                    .opcode = 0xF9,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x00, .Y = 0x10, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x15)
                        .org(0x0400)
                            .sbc(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SBC abs,Y absolute,Y without page cross",
                },
                testcase{
                    .opcode = 0xF9,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x00, .Y = 0x20, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x15)
                        .org(0x0400)
                            .sbc(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SBC abs,Y absolute,Y with page cross",
                },
            }
        },
        // 0xFA
        {
            0xFA,
            {
                testcase{
                    .opcode = 0xFA,
                    .bytes = 1,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x00, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x0400)
                            .nop_opcode(0xFA)
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP implied",
                },
            }
        },
        // 0xFB
        {
            0xFB,
            {
                testcase{
                    .opcode = 0xFB,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x10, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x7f)
                        .org(0x0400)
                            .isc(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ISC abs,Y absolute,Y",
                },
                testcase{
                    .opcode = 0xFB,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x00, .Y = 0x20, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x7f)
                        .org(0x0400)
                            .isc(aby, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ISC abs,Y absolute,Y with page cross",
                },
            }
        },
        // 0xFC
        {
            0xFC,
            {
                testcase{
                    .opcode = 0xFC,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .nop_opcode(0xFC, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP abs,X absolute,X without page cross",
                },
                testcase{
                    .opcode = 0xFC,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x20, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x87)
                        .org(0x0400)
                            .nop_opcode(0xFC, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "NOP abs,X absolute,X with page cross",
                },
            }
        },
        // 0xFD
        {
            0xFD,
            {
                testcase{
                    .opcode = 0xFD,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x10, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x15)
                        .org(0x0400)
                            .sbc(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SBC abs,X absolute,X without page cross",
                },
                testcase{
                    .opcode = 0xFD,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x45, .X = 0x20, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x15)
                        .org(0x0400)
                            .sbc(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "SBC abs,X absolute,X with page cross",
                },
            }
        },
        // 0xFE
        {
            0xFE,
            {
                testcase{
                    .opcode = 0xFE,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x10, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x7f)
                        .org(0x0400)
                            .inc(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "INC abs,X absolute,X",
                },
                testcase{
                    .opcode = 0xFE,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0x42, .X = 0x20, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x7f)
                        .org(0x0400)
                            .inc(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "INC abs,X absolute,X with page cross",
                },
            }
        },
        // 0xFF
        {
            0xFF,
            {
                testcase{
                    .opcode = 0xFF,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x10, .Y = 0x00, .P = 0x25, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x2100, "value_base")
                        .org(0x2110, "value_addr")
                            .db(0x7f)
                        .org(0x0400)
                            .isc(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ISC abs,X absolute,X",
                },
                testcase{
                    .opcode = 0xFF,
                    .bytes = 3,
                    .start_at = 0x0400,
                    .vectors = {.reset = 0x0200, .brk_irq = 0x0800, .nmi = 0x0800},
                    .A = 0xa5, .X = 0x20, .Y = 0x00, .P = 0x24, .S = 0xfd,
                    .program = Asm6502::New()
                    .begin()
                        .org(0x21f0, "value_base")
                        .org(0x2210, "value_addr")
                            .db(0x7f)
                        .org(0x0400)
                            .isc(abx, "value_base")
                        .label("trap")
                            .jmp("trap")
                    .end().compile(),
                    .description = "ISC abs,X absolute,X with page cross",
                },
            }
        },
    };
}

} // namespace tools6502
