#pragma once

/*
 * Small C++20 embedded assembler for building 6502 test programs.
 *
 * Basic usage:
 *
 *     asm6502::Asm6502 p;
 *
 *     p.begin()
 *      .reset_vector("boot")          // RESET vector at $FFFC/$FFFD; does not move PC
 *      .nmi_vector("nmi")             // NMI vector at $FFFA/$FFFB; does not move PC
 *      .brk_irq_vector("irq")         // shared BRK/IRQ vector at $FFFE/$FFFF; does not move PC
 *
 *      .set("zp_ptr", 0x0080)         // define constant symbol
 *      .set("table_end", "table", 3)  // define symbol as table + 3
 *
 *      .org(0x0400, "boot")           // set PC to $0400 and define label "boot"
 *          .lda(0x12)                 // default LDA mode is immediate
 *          .jmp("test")               // default JMP mode is absolute
 *
 *      .org(0x0500, "test")
 *          .label("loop")             // like "loop:" in assembly
 *          .lda(asm6502::izy, "zp_ptr")
 *          .bne("loop")               // signed relative branch to label
 *
 *      .org(0x0080)
 *          .dw("table")               // little-endian word; labels are allowed
 *
 *      .org(0x0600, "table")
 *          .db(0x34, 0x56, 0x78)
 *      .end();
 *
 *     auto mem = p.compile_to_mem_values();
 *
 * Directives:
 *   begin()                 starts a fluent source-building chain; currently a no-op.
 *   end()                   ends a fluent source-building chain; currently a no-op.
 *   org(address)            moves the program counter.
 *   org(address, label)     moves the program counter and defines a label there.
 *   label(name)             defines a label at the current program counter.
 *   set(name, value)        defines a constant symbol.
 *   set(name, base, disp)   defines a symbol relative to another symbol.
 *   db(...)                 emits byte values.
 *   dw(...)                 emits little-endian 16-bit values or label addresses.
 *
 * Vector helpers:
 *   nmi_vector(target)      writes target to $FFFA/$FFFB.
 *   reset_vector(target)    writes target to $FFFC/$FFFD.
 *   brk_irq_vector(target)  writes target to $FFFE/$FFFF.
 *   Vector helpers do not move the program counter.
 *
 * Address mode keywords:
 *   zp        zero page:              LDA $80
 *   zpx       zero page,X:            LDA $80,X
 *   zpy       zero page,Y:            LDX $80,Y
 *   absolute  absolute:               LDA $2200
 *   abx       absolute,X:             LDA $2200,X
 *   aby       absolute,Y:             LDA $2200,Y
 *   ind       absolute indirect:      JMP ($2200)
 *   izx       indexed indirect:       LDA ($80,X)
 *   izy       indirect indexed:       LDA ($80),Y
 *
 * Other helper keywords:
 *   sym(name, disp)         label reference with displacement, useful in dw(...):
 *                               .dw(asm6502::sym("handler", 2))
 *
 * Instruction defaults:
 *   lda(value), adc(value), cmp(value), etc. use immediate mode where valid.
 *   jmp(label/address) uses absolute mode.
 *   bcc/bcs/beq/bmi/bne/bpl/bvc/bvs(label, disp) use relative branch offsets.
 *   Implied instructions use no address-mode keyword, e.g. nop(), rts(), clc().
 *
 * Only org(), db(), dw(), and instruction bytes advance the program counter.
 * Vector helpers and set() do not move the program counter.
 *
 * Symbol resolution is iterative, so forward references and dependent symbols are supported.
 * Compilation writes to a checked address map; writing two bytes to the same address is an error.
 */

#include <cstdint>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace asm6502 {

/// One memory setup byte: {address, value}.
using mem_value = std::pair<std::uint16_t, std::uint8_t>;

/// Resolved assembler symbol table.
using symbol_table = std::map<std::string, std::uint16_t>;

/// Compiled memory modifications, sorted by address.
using memory_modifiers = std::map<std::uint16_t, std::uint8_t>;

/// Supported 6502 addressing-mode identifiers.
enum class address_mode_id
{
    acc,
    imm,
    zp,
    zpx,
    zpy,
    abs,
    abx,
    aby,
    ind,
    izx,
    izy,
    rel
};

/// Strong type used to select an addressing mode in instruction helpers.
struct address_mode
{
    address_mode_id id;
};

/// Accumulator addressing mode, e.g. ASL A. Most APIs expose this as asl().
inline constexpr address_mode acc{address_mode_id::acc};
/// Immediate addressing mode, e.g. LDA #$12.
inline constexpr address_mode imm{address_mode_id::imm};
/// Zero-page addressing mode, e.g. LDA $80.
inline constexpr address_mode zp {address_mode_id::zp};
/// Zero-page,X addressing mode, e.g. LDA $80,X.
inline constexpr address_mode zpx{address_mode_id::zpx};
/// Zero-page,Y addressing mode, e.g. LDX $80,Y.
inline constexpr address_mode zpy{address_mode_id::zpy};
/// Absolute addressing mode, e.g. LDA $2200. Named `absolute` to avoid the C library `abs()` name.
inline constexpr address_mode absolute{address_mode_id::abs};
/// Absolute,X addressing mode, e.g. LDA $2200,X.
inline constexpr address_mode abx{address_mode_id::abx};
/// Absolute,Y addressing mode, e.g. LDA $2200,Y.
inline constexpr address_mode aby{address_mode_id::aby};
/// Absolute indirect addressing mode, e.g. JMP ($2200).
inline constexpr address_mode ind{address_mode_id::ind};
/// Indexed-indirect addressing mode, e.g. LDA ($40,X).
inline constexpr address_mode izx{address_mode_id::izx};
/// Indirect-indexed addressing mode, e.g. LDA ($80),Y.
inline constexpr address_mode izy{address_mode_id::izy};
/// Relative addressing mode. Branch helpers use this implicitly.
inline constexpr address_mode rel{address_mode_id::rel};

/// Symbol reference with an optional signed displacement.
struct symbol_ref
{
    std::string name;
    int displacement = 0;
};

/// Operand target: either a direct 16-bit value or a symbol reference.
using link_target = std::variant<std::uint16_t, symbol_ref>;

/// Create a symbol reference, useful for .dw(sym("label", +2)).
symbol_ref sym(std::string name, int displacement = 0);

/// Reserved text encodings for future string directives.
enum class text_encoding
{
    plain_ascii
};

/// Reserved target platform selector for future platform-specific directives.
enum class target_platform
{
    generic_6502
};

/// Fluent assembler for the official and undocumented NMOS 6502 instruction sets.
class Asm6502
{
public:
    /// Create an empty assembler program.
    Asm6502();

    static Asm6502 New();

    Asm6502& begin();
    Asm6502& end();

    /// Set the current output address. This is the assembler origin directive.
    Asm6502& org(std::uint16_t address);

    /// Set the current output address and define a label at that address.
    Asm6502& org(std::uint16_t address, std::string name);

    /// Set the current output address at that label address.
    Asm6502& org(std::string name);

    /// Define a label at the current PC, like `label:` in assembler source.
    Asm6502& label(std::string name);

    /// Define a symbol to an absolute 16-bit value without changing PC.
    Asm6502& set(std::string name, std::uint16_t value);

    /// Define a symbol as another symbol plus displacement, without changing PC.
    Asm6502& set(std::string name, std::string base, int displacement = 0);

    /// Write the NMI vector at $FFFA/$FFFB without changing PC.
    Asm6502& nmi_vector(std::uint16_t address);

    /// Write the NMI vector at $FFFA/$FFFB without changing PC.
    Asm6502& nmi_vector(std::string label, int displacement = 0);

    /// Write the reset vector at $FFFC/$FFFD without changing PC.
    Asm6502& reset_vector(std::uint16_t address);

    /// Write the reset vector at $FFFC/$FFFD without changing PC.
    Asm6502& reset_vector(std::string label, int displacement = 0);

    /// Write the shared BRK/IRQ vector at $FFFE/$FFFF without changing PC.
    Asm6502& brk_irq_vector(std::uint16_t address);

    /// Write the shared BRK/IRQ vector at $FFFE/$FFFF without changing PC.
    Asm6502& brk_irq_vector(std::string label, int displacement = 0);

    /// Emit one or more bytes at the current PC.
    template<class... T>
    Asm6502& db(T&&... values)
    {
        (add_db_argument(std::forward<T>(values)), ...);
        return *this;
    }

    /// Emit one or more little-endian words or label addresses at the current PC.
    template<class... T>
    Asm6502& dw(T&&... values)
    {
        (add_dw_argument(std::forward<T>(values)), ...);
        return *this;
    }

    /* Undocumented/illegal NMOS 6502 helpers implemented by qe6502's NMOS control store.
     *
     * Methods use the common mnemonics from qe6502/cpu/src/control_store/nmos_block.inc.
     * For duplicated opcodes with the same mnemonic/addressing mode, the normal mnemonic
     * helper emits the first opcode in the control store; *_opcode helpers let callers
     * select an exact encoding. Exact selectors exist for all qe6502 duplicate
     * encodings: NOP, ANC, SBC #imm ($E9/$EB), and KIL/JAM.
     */

    /// Emit AHX using (zp),Y or abs,Y addressing. Unstable NMOS store instruction.
    Asm6502& ahx(address_mode mode, std::uint16_t operand);

    /// Emit AHX using (zp),Y or abs,Y addressing and a label operand.
    Asm6502& ahx(address_mode mode, std::string label, int displacement = 0);

    /// Emit ALR #imm.
    template<class T>
    Asm6502& alr(T value)
    {
        return alr_impl(imm, direct_byte_target(value, "ALR immediate"));
    }

    /// Emit ALR using immediate addressing.
    Asm6502& alr(address_mode mode, std::uint16_t operand);

    /// Emit ALR using immediate addressing and a label operand.
    Asm6502& alr(address_mode mode, std::string label, int displacement = 0);

    /// Emit ANC #imm using opcode $0B. Use anc_opcode($2B, ...) for the alternate encoding.
    template<class T>
    Asm6502& anc(T value)
    {
        return anc_impl(0x0bu, imm, direct_byte_target(value, "ANC immediate"));
    }

    /// Emit ANC using immediate addressing and opcode $0B.
    Asm6502& anc(address_mode mode, std::uint16_t operand);

    /// Emit ANC using immediate addressing, opcode $0B, and a label operand.
    Asm6502& anc(address_mode mode, std::string label, int displacement = 0);

    /// Emit ANC #imm with an exact opcode: $0B or $2B.
    template<class T>
    Asm6502& anc_opcode(std::uint8_t opcode, T value)
    {
        return anc_impl(opcode, imm, direct_byte_target(value, "ANC immediate"));
    }

    /// Emit ANC with an exact opcode: $0B or $2B.
    Asm6502& anc_opcode(std::uint8_t opcode, address_mode mode, std::uint16_t operand);

    /// Emit ANC with an exact opcode and label operand: $0B or $2B.
    Asm6502& anc_opcode(std::uint8_t opcode, address_mode mode, std::string label, int displacement = 0);

    /// Emit ARR #imm.
    template<class T>
    Asm6502& arr(T value)
    {
        return arr_impl(imm, direct_byte_target(value, "ARR immediate"));
    }

    /// Emit ARR using immediate addressing.
    Asm6502& arr(address_mode mode, std::uint16_t operand);

    /// Emit ARR using immediate addressing and a label operand.
    Asm6502& arr(address_mode mode, std::string label, int displacement = 0);

    /// Emit AXS #imm.
    template<class T>
    Asm6502& axs(T value)
    {
        return axs_impl(imm, direct_byte_target(value, "AXS immediate"));
    }

    /// Emit AXS using immediate addressing.
    Asm6502& axs(address_mode mode, std::uint16_t operand);

    /// Emit AXS using immediate addressing and a label operand.
    Asm6502& axs(address_mode mode, std::string label, int displacement = 0);

    /// Emit DCP using an explicit memory addressing mode.
    Asm6502& dcp(address_mode mode, std::uint16_t operand);

    /// Emit DCP using an explicit memory addressing mode and a label operand.
    Asm6502& dcp(address_mode mode, std::string label, int displacement = 0);

    /// Emit ISC using an explicit memory addressing mode.
    Asm6502& isc(address_mode mode, std::uint16_t operand);

    /// Emit ISC using an explicit memory addressing mode and a label operand.
    Asm6502& isc(address_mode mode, std::string label, int displacement = 0);

    /// Emit JAM/KIL. Defaults to opcode $02; accepts every qe6502 NMOS KIL/JAM opcode.
    Asm6502& jam(std::uint8_t opcode = 0x02u);

    /// Emit JAM/KIL with an exact opcode. Synonym for jam(opcode), provided for selector symmetry.
    Asm6502& jam_opcode(std::uint8_t opcode);

    /// Emit KIL/JAM. Defaults to opcode $02; accepts every qe6502 NMOS KIL/JAM opcode.
    Asm6502& kil(std::uint8_t opcode = 0x02u);

    /// Emit KIL/JAM with an exact opcode. Synonym for kil(opcode), provided for selector symmetry.
    Asm6502& kil_opcode(std::uint8_t opcode);

    /// Emit LAS abs,Y.
    Asm6502& las(address_mode mode, std::uint16_t operand);

    /// Emit LAS abs,Y with a label operand.
    Asm6502& las(address_mode mode, std::string label, int displacement = 0);

    /// Emit LAX using an explicit memory addressing mode.
    Asm6502& lax(address_mode mode, std::uint16_t operand);

    /// Emit LAX using an explicit memory addressing mode and a label operand.
    Asm6502& lax(address_mode mode, std::string label, int displacement = 0);

    /// Emit LXA #imm.
    template<class T>
    Asm6502& lxa(T value)
    {
        return lxa_impl(imm, direct_byte_target(value, "LXA immediate"));
    }

    /// Emit LXA using immediate addressing.
    Asm6502& lxa(address_mode mode, std::uint16_t operand);

    /// Emit LXA using immediate addressing and a label operand.
    Asm6502& lxa(address_mode mode, std::string label, int displacement = 0);

    /// Emit an undocumented NOP variant using the canonical opcode for the requested mode.
    Asm6502& nop(address_mode mode, std::uint16_t operand);

    /// Emit an undocumented NOP variant using the canonical opcode for the requested mode and a label operand.
    Asm6502& nop(address_mode mode, std::string label, int displacement = 0);

    /// Emit an exact implied NOP opcode, including undocumented implied NOPs.
    Asm6502& nop_opcode(std::uint8_t opcode);

    /// Emit an exact operand-taking NOP opcode, including undocumented NOPs.
    Asm6502& nop_opcode(std::uint8_t opcode, std::uint16_t operand);

    /// Emit an exact operand-taking NOP opcode with a label operand.
    Asm6502& nop_opcode(std::uint8_t opcode, std::string label, int displacement = 0);

    /// Emit RLA using an explicit memory addressing mode.
    Asm6502& rla(address_mode mode, std::uint16_t operand);

    /// Emit RLA using an explicit memory addressing mode and a label operand.
    Asm6502& rla(address_mode mode, std::string label, int displacement = 0);

    /// Emit RRA using an explicit memory addressing mode.
    Asm6502& rra(address_mode mode, std::uint16_t operand);

    /// Emit RRA using an explicit memory addressing mode and a label operand.
    Asm6502& rra(address_mode mode, std::string label, int displacement = 0);

    /// Emit SAX using an explicit memory addressing mode.
    Asm6502& sax(address_mode mode, std::uint16_t operand);

    /// Emit SAX using an explicit memory addressing mode and a label operand.
    Asm6502& sax(address_mode mode, std::string label, int displacement = 0);

    /// Emit SBC #imm with an exact immediate opcode: official $E9 or undocumented duplicate $EB.
    template<class T>
    Asm6502& sbc_opcode(std::uint8_t opcode, T value)
    {
        return sbc_opcode_impl(opcode, imm, direct_byte_target(value, "SBC immediate"));
    }

    /// Emit SBC using immediate addressing and an exact opcode: official $E9 or undocumented duplicate $EB.
    Asm6502& sbc_opcode(std::uint8_t opcode, address_mode mode, std::uint16_t operand);

    /// Emit SBC using immediate addressing, an exact opcode, and a label operand: $E9 or $EB.
    Asm6502& sbc_opcode(std::uint8_t opcode, address_mode mode, std::string label, int displacement = 0);

    /// Emit undocumented SBC #imm using opcode $EB.
    template<class T>
    Asm6502& sbc_unofficial(T value)
    {
        return sbc_opcode_impl(0xebu, imm, direct_byte_target(value, "SBC unofficial immediate"));
    }

    /// Emit undocumented SBC using immediate addressing and opcode $EB.
    Asm6502& sbc_unofficial(address_mode mode, std::uint16_t operand);

    /// Emit undocumented SBC using immediate addressing, opcode $EB, and a label operand.
    Asm6502& sbc_unofficial(address_mode mode, std::string label, int displacement = 0);

    /// Emit SHX abs,Y. Unstable NMOS store instruction.
    Asm6502& shx(address_mode mode, std::uint16_t operand);

    /// Emit SHX abs,Y with a label operand.
    Asm6502& shx(address_mode mode, std::string label, int displacement = 0);

    /// Emit SHY abs,X. Unstable NMOS store instruction.
    Asm6502& shy(address_mode mode, std::uint16_t operand);

    /// Emit SHY abs,X with a label operand.
    Asm6502& shy(address_mode mode, std::string label, int displacement = 0);

    /// Emit SLO using an explicit memory addressing mode.
    Asm6502& slo(address_mode mode, std::uint16_t operand);

    /// Emit SLO using an explicit memory addressing mode and a label operand.
    Asm6502& slo(address_mode mode, std::string label, int displacement = 0);

    /// Emit SRE using an explicit memory addressing mode.
    Asm6502& sre(address_mode mode, std::uint16_t operand);

    /// Emit SRE using an explicit memory addressing mode and a label operand.
    Asm6502& sre(address_mode mode, std::string label, int displacement = 0);

    /// Emit TAS abs,Y. Unstable NMOS store instruction.
    Asm6502& tas(address_mode mode, std::uint16_t operand);

    /// Emit TAS abs,Y with a label operand.
    Asm6502& tas(address_mode mode, std::string label, int displacement = 0);

    /// Emit XAA #imm.
    template<class T>
    Asm6502& xaa(T value)
    {
        return xaa_impl(imm, direct_byte_target(value, "XAA immediate"));
    }

    /// Emit XAA using immediate addressing.
    Asm6502& xaa(address_mode mode, std::uint16_t operand);

    /// Emit XAA using immediate addressing and a label operand.
    Asm6502& xaa(address_mode mode, std::string label, int displacement = 0);

    /// Emit ADC #imm.
    template<class T>
    Asm6502& adc(T value)
    {
        return adc_impl(imm, direct_byte_target(value, "ADC immediate"));
    }

    /// Emit ADC using an explicit addressing mode.
    Asm6502& adc(address_mode mode, std::uint16_t operand);

    /// Emit ADC using an explicit addressing mode and label operand.
    Asm6502& adc(address_mode mode, std::string label, int displacement = 0);

    /// Emit AND #imm.
    template<class T>
    Asm6502& and_(T value)
    {
        return and_impl(imm, direct_byte_target(value, "AND immediate"));
    }

    /// Emit AND using an explicit addressing mode.
    Asm6502& and_(address_mode mode, std::uint16_t operand);

    /// Emit AND using an explicit addressing mode and label operand.
    Asm6502& and_(address_mode mode, std::string label, int displacement = 0);

    /// Emit ASL A.
    Asm6502& asl();

    /// Emit ASL using an explicit memory addressing mode.
    Asm6502& asl(address_mode mode, std::uint16_t operand);

    /// Emit ASL using an explicit memory addressing mode and label operand.
    Asm6502& asl(address_mode mode, std::string label, int displacement = 0);

    /// Emit BCC to a label plus optional displacement.
    Asm6502& bcc(std::string label, int displacement = 0);

    /// Emit BCS to a label plus optional displacement.
    Asm6502& bcs(std::string label, int displacement = 0);

    /// Emit BEQ to a label plus optional displacement.
    Asm6502& beq(std::string label, int displacement = 0);

    /// Emit BIT using zero-page or absolute addressing.
    Asm6502& bit(address_mode mode, std::uint16_t operand);

    /// Emit BIT using zero-page or absolute addressing and a label operand.
    Asm6502& bit(address_mode mode, std::string label, int displacement = 0);

    /// Emit BMI to a label plus optional displacement.
    Asm6502& bmi(std::string label, int displacement = 0);

    /// Emit BNE to a label plus optional displacement.
    Asm6502& bne(std::string label, int displacement = 0);

    /// Emit BPL to a label plus optional displacement.
    Asm6502& bpl(std::string label, int displacement = 0);

    /// Emit BRK.
    Asm6502& brk();

    /// Emit BVC to a label plus optional displacement.
    Asm6502& bvc(std::string label, int displacement = 0);

    /// Emit BVS to a label plus optional displacement.
    Asm6502& bvs(std::string label, int displacement = 0);

    /// Emit CLC.
    Asm6502& clc();

    /// Emit CLD.
    Asm6502& cld();

    /// Emit CLI.
    Asm6502& cli();

    /// Emit CLV.
    Asm6502& clv();

    /// Emit CMP #imm.
    template<class T>
    Asm6502& cmp(T value)
    {
        return cmp_impl(imm, direct_byte_target(value, "CMP immediate"));
    }

    /// Emit CMP using an explicit addressing mode.
    Asm6502& cmp(address_mode mode, std::uint16_t operand);

    /// Emit CMP using an explicit addressing mode and label operand.
    Asm6502& cmp(address_mode mode, std::string label, int displacement = 0);

    /// Emit CPX #imm.
    template<class T>
    Asm6502& cpx(T value)
    {
        return cpx_impl(imm, direct_byte_target(value, "CPX immediate"));
    }

    /// Emit CPX using zero-page or absolute addressing.
    Asm6502& cpx(address_mode mode, std::uint16_t operand);

    /// Emit CPX using zero-page or absolute addressing and a label operand.
    Asm6502& cpx(address_mode mode, std::string label, int displacement = 0);

    /// Emit CPY #imm.
    template<class T>
    Asm6502& cpy(T value)
    {
        return cpy_impl(imm, direct_byte_target(value, "CPY immediate"));
    }

    /// Emit CPY using zero-page or absolute addressing.
    Asm6502& cpy(address_mode mode, std::uint16_t operand);

    /// Emit CPY using zero-page or absolute addressing and a label operand.
    Asm6502& cpy(address_mode mode, std::string label, int displacement = 0);

    /// Emit DEC using an explicit memory addressing mode.
    Asm6502& dec(address_mode mode, std::uint16_t operand);

    /// Emit DEC using an explicit memory addressing mode and label operand.
    Asm6502& dec(address_mode mode, std::string label, int displacement = 0);

    /// Emit DEX.
    Asm6502& dex();

    /// Emit DEY.
    Asm6502& dey();

    /// Emit EOR #imm.
    template<class T>
    Asm6502& eor(T value)
    {
        return eor_impl(imm, direct_byte_target(value, "EOR immediate"));
    }

    /// Emit EOR using an explicit addressing mode.
    Asm6502& eor(address_mode mode, std::uint16_t operand);

    /// Emit EOR using an explicit addressing mode and label operand.
    Asm6502& eor(address_mode mode, std::string label, int displacement = 0);

    /// Emit INC using an explicit memory addressing mode.
    Asm6502& inc(address_mode mode, std::uint16_t operand);

    /// Emit INC using an explicit memory addressing mode and label operand.
    Asm6502& inc(address_mode mode, std::string label, int displacement = 0);

    /// Emit INX.
    Asm6502& inx();

    /// Emit INY.
    Asm6502& iny();

    /// Emit JMP absolute.
    Asm6502& jmp(std::uint16_t address);

    /// Emit JMP absolute to a label plus optional displacement.
    Asm6502& jmp(std::string label, int displacement = 0);

    /// Emit JMP using absolute or indirect addressing.
    Asm6502& jmp(address_mode mode, std::uint16_t address);

    /// Emit JMP using absolute or indirect addressing and a label operand.
    Asm6502& jmp(address_mode mode, std::string label, int displacement = 0);

    /// Emit JSR absolute.
    Asm6502& jsr(std::uint16_t address);

    /// Emit JSR absolute to a label plus optional displacement.
    Asm6502& jsr(std::string label, int displacement = 0);

    /// Emit LDA #imm.
    template<class T>
    Asm6502& lda(T value)
    {
        return lda_impl(imm, direct_byte_target(value, "LDA immediate"));
    }

    /// Emit LDA using an explicit addressing mode.
    Asm6502& lda(address_mode mode, std::uint16_t operand);

    /// Emit LDA using an explicit addressing mode and label operand.
    Asm6502& lda(address_mode mode, std::string label, int displacement = 0);

    /// Emit LDX #imm.
    template<class T>
    Asm6502& ldx(T value)
    {
        return ldx_impl(imm, direct_byte_target(value, "LDX immediate"));
    }

    /// Emit LDX using an explicit addressing mode.
    Asm6502& ldx(address_mode mode, std::uint16_t operand);

    /// Emit LDX using an explicit addressing mode and label operand.
    Asm6502& ldx(address_mode mode, std::string label, int displacement = 0);

    /// Emit LDY #imm.
    template<class T>
    Asm6502& ldy(T value)
    {
        return ldy_impl(imm, direct_byte_target(value, "LDY immediate"));
    }

    /// Emit LDY using an explicit addressing mode.
    Asm6502& ldy(address_mode mode, std::uint16_t operand);

    /// Emit LDY using an explicit addressing mode and label operand.
    Asm6502& ldy(address_mode mode, std::string label, int displacement = 0);

    /// Emit LSR A.
    Asm6502& lsr();

    /// Emit LSR using an explicit memory addressing mode.
    Asm6502& lsr(address_mode mode, std::uint16_t operand);

    /// Emit LSR using an explicit memory addressing mode and label operand.
    Asm6502& lsr(address_mode mode, std::string label, int displacement = 0);

    /// Emit NOP.
    Asm6502& nop();

    /// Emit ORA #imm.
    template<class T>
    Asm6502& ora(T value)
    {
        return ora_impl(imm, direct_byte_target(value, "ORA immediate"));
    }

    /// Emit ORA using an explicit addressing mode.
    Asm6502& ora(address_mode mode, std::uint16_t operand);

    /// Emit ORA using an explicit addressing mode and label operand.
    Asm6502& ora(address_mode mode, std::string label, int displacement = 0);

    /// Emit PHA.
    Asm6502& pha();

    /// Emit PHP.
    Asm6502& php();

    /// Emit PLA.
    Asm6502& pla();

    /// Emit PLP.
    Asm6502& plp();

    /// Emit ROL A.
    Asm6502& rol();

    /// Emit ROL using an explicit memory addressing mode.
    Asm6502& rol(address_mode mode, std::uint16_t operand);

    /// Emit ROL using an explicit memory addressing mode and label operand.
    Asm6502& rol(address_mode mode, std::string label, int displacement = 0);

    /// Emit ROR A.
    Asm6502& ror();

    /// Emit ROR using an explicit memory addressing mode.
    Asm6502& ror(address_mode mode, std::uint16_t operand);

    /// Emit ROR using an explicit memory addressing mode and label operand.
    Asm6502& ror(address_mode mode, std::string label, int displacement = 0);

    /// Emit RTI.
    Asm6502& rti();

    /// Emit RTS.
    Asm6502& rts();

    /// Emit SBC #imm.
    template<class T>
    Asm6502& sbc(T value)
    {
        return sbc_impl(imm, direct_byte_target(value, "SBC immediate"));
    }

    /// Emit SBC using an explicit addressing mode.
    Asm6502& sbc(address_mode mode, std::uint16_t operand);

    /// Emit SBC using an explicit addressing mode and label operand.
    Asm6502& sbc(address_mode mode, std::string label, int displacement = 0);

    /// Emit SEC.
    Asm6502& sec();

    /// Emit SED.
    Asm6502& sed();

    /// Emit SEI.
    Asm6502& sei();

    /// Emit STA using an explicit addressing mode.
    Asm6502& sta(address_mode mode, std::uint16_t operand);

    /// Emit STA using an explicit addressing mode and label operand.
    Asm6502& sta(address_mode mode, std::string label, int displacement = 0);

    /// Emit STX using zero-page, zero-page,Y, or absolute addressing.
    Asm6502& stx(address_mode mode, std::uint16_t operand);

    /// Emit STX using zero-page, zero-page,Y, or absolute addressing and a label operand.
    Asm6502& stx(address_mode mode, std::string label, int displacement = 0);

    /// Emit STY using zero-page, zero-page,X, or absolute addressing.
    Asm6502& sty(address_mode mode, std::uint16_t operand);

    /// Emit STY using zero-page, zero-page,X, or absolute addressing and a label operand.
    Asm6502& sty(address_mode mode, std::string label, int displacement = 0);

    /// Emit TAX.
    Asm6502& tax();

    /// Emit TAY.
    Asm6502& tay();

    /// Emit TSX.
    Asm6502& tsx();

    /// Emit TXA.
    Asm6502& txa();

    /// Emit TXS.
    Asm6502& txs();

    /// Emit TYA.
    Asm6502& tya();

    /// Compile to a sorted map of memory modifications. Throws on overwrite or undefined symbols.
    memory_modifiers compile_to_map() const;

    /// Compile to the testcase-friendly vector form: {address, byte} pairs.
    std::vector<mem_value> compile() const;

    /// Compile and apply all memory modifications to a caller-owned 64 KiB memory buffer.
    static void apply(const std::vector<mem_value>&, std::uint8_t* memory);

    /// Compile and apply all memory modifications to a caller-owned 64 KiB memory buffer.
    static void apply(const memory_modifiers&, std::uint8_t* memory);

private:
    struct asm_context
    {
        symbol_table symbols;
        std::uint16_t pc = 0;
        memory_modifiers* output = nullptr;
        text_encoding encoding = text_encoding::plain_ascii;
        target_platform platform = target_platform::generic_6502;

        void put(std::uint16_t address, std::uint8_t value);
    };

    struct asm_command
    {
        std::function<bool(asm_context&)> resolve;
        std::function<void(asm_context&)> place;
        std::function<void(asm_context&)> update_pc;
    };

    struct mode_opcode
    {
        address_mode_id mode;
        std::uint8_t opcode;
        bool word_operand;
    };

    std::vector<asm_command> commands_;

    static bool define_symbol(symbol_table& symbols, const std::string& name, std::uint16_t value);
    static std::uint16_t resolve_target(const asm_context& ctx, const link_target& target);
    static std::string hex16(std::uint16_t value);
    static std::string hex8(std::uint8_t value);

    template<class T>
    static std::uint8_t checked_u8(T value, const char* what)
    {
        using U = std::remove_cv_t<std::remove_reference_t<T>>;
        static_assert(std::is_integral_v<U>, "expected an integral byte value");

        if constexpr (std::is_signed_v<U>)
        {
            if (value < 0)
                throw std::out_of_range(std::string(what) + " is negative");
        }

        const auto unsigned_value = static_cast<unsigned long long>(value);
        if (unsigned_value > 0xffull)
            throw std::out_of_range(std::string(what) + " does not fit in 8 bits");

        return static_cast<std::uint8_t>(unsigned_value);
    }

    template<class T>
    static std::uint16_t checked_u16(T value, const char* what)
    {
        using U = std::remove_cv_t<std::remove_reference_t<T>>;
        static_assert(std::is_integral_v<U>, "expected an integral word value");

        if constexpr (std::is_signed_v<U>)
        {
            if (value < 0)
                throw std::out_of_range(std::string(what) + " is negative");
        }

        const auto unsigned_value = static_cast<unsigned long long>(value);
        if (unsigned_value > 0xffffull)
            throw std::out_of_range(std::string(what) + " does not fit in 16 bits");

        return static_cast<std::uint16_t>(unsigned_value);
    }

    template<class T>
    static link_target direct_byte_target(T value, const char* what)
    {
        return link_target{static_cast<std::uint16_t>(checked_u8(value, what))};
    }

    template<class T>
    void add_db_argument(T&& value)
    {
        using U = std::remove_cv_t<std::remove_reference_t<T>>;
        if constexpr (std::is_integral_v<U>)
        {
            add_byte(checked_u8(value, "db() argument"));
        }
        else
        {
            static_assert(std::is_integral_v<U>, "db() accepts only integral byte values");
        }
    }

    template<class T>
    void add_dw_argument(T&& value)
    {
        using U = std::remove_cv_t<std::remove_reference_t<T>>;
        using D = std::decay_t<T>;

        if constexpr (std::is_integral_v<U>)
        {
            add_word(link_target{checked_u16(value, "dw() argument")});
        }
        else if constexpr (std::is_same_v<U, symbol_ref>)
        {
            add_word(link_target{std::forward<T>(value)});
        }
        else if constexpr (std::is_convertible_v<D, std::string>)
        {
            add_word(link_target{symbol_ref{std::string(std::forward<T>(value)), 0}});
        }
        else
        {
            static_assert(std::is_integral_v<U>, "dw() accepts integral words, strings, or symbol_ref");
        }
    }

    Asm6502& add_byte(std::uint8_t value);
    Asm6502& add_low(link_target target);
    Asm6502& add_word(link_target target);
    Asm6502& add_relative(link_target target);
    Asm6502& add_fixed_word(std::uint16_t address, link_target target);
    Asm6502& emit_implied(std::uint8_t opcode);
    Asm6502& emit_branch(std::uint8_t opcode, std::string label, int displacement);
    Asm6502& emit_addressed(const char* mnemonic, address_mode mode, link_target target,
                            const mode_opcode* table, std::size_t table_size);

    static bool is_kil_jam_opcode(std::uint8_t opcode);
    static bool is_nop_implied_opcode(std::uint8_t opcode);
    static bool is_nop_byte_operand_opcode(std::uint8_t opcode);
    static bool is_nop_word_operand_opcode(std::uint8_t opcode);

    Asm6502& ahx_impl(address_mode mode, link_target target);
    Asm6502& alr_impl(address_mode mode, link_target target);
    Asm6502& anc_impl(std::uint8_t opcode, address_mode mode, link_target target);
    Asm6502& arr_impl(address_mode mode, link_target target);
    Asm6502& axs_impl(address_mode mode, link_target target);
    Asm6502& dcp_impl(address_mode mode, link_target target);
    Asm6502& illegal_rmw_impl(const char* mnemonic, address_mode mode, link_target target,
                              std::uint8_t izx_opcode, std::uint8_t zp_opcode,
                              std::uint8_t zpx_opcode, std::uint8_t abs_opcode,
                              std::uint8_t abx_opcode, std::uint8_t aby_opcode,
                              std::uint8_t izy_opcode);
    Asm6502& isc_impl(address_mode mode, link_target target);
    Asm6502& las_impl(address_mode mode, link_target target);
    Asm6502& lax_impl(address_mode mode, link_target target);
    Asm6502& lxa_impl(address_mode mode, link_target target);
    Asm6502& nop_impl(address_mode mode, link_target target);
    Asm6502& nop_opcode_impl(std::uint8_t opcode, link_target target);
    Asm6502& rla_impl(address_mode mode, link_target target);
    Asm6502& rra_impl(address_mode mode, link_target target);
    Asm6502& sax_impl(address_mode mode, link_target target);
    Asm6502& sbc_opcode_impl(std::uint8_t opcode, address_mode mode, link_target target);
    Asm6502& sbc_unofficial_impl(address_mode mode, link_target target);
    Asm6502& shx_impl(address_mode mode, link_target target);
    Asm6502& shy_impl(address_mode mode, link_target target);
    Asm6502& slo_impl(address_mode mode, link_target target);
    Asm6502& sre_impl(address_mode mode, link_target target);
    Asm6502& tas_impl(address_mode mode, link_target target);
    Asm6502& xaa_impl(address_mode mode, link_target target);

    Asm6502& adc_impl(address_mode mode, link_target target);
    Asm6502& and_impl(address_mode mode, link_target target);
    Asm6502& bit_impl(address_mode mode, link_target target);
    Asm6502& cmp_impl(address_mode mode, link_target target);
    Asm6502& cpx_impl(address_mode mode, link_target target);
    Asm6502& cpy_impl(address_mode mode, link_target target);
    Asm6502& dec_impl(address_mode mode, link_target target);
    Asm6502& eor_impl(address_mode mode, link_target target);
    Asm6502& inc_impl(address_mode mode, link_target target);
    Asm6502& jmp_impl(address_mode mode, link_target target);
    Asm6502& lda_impl(address_mode mode, link_target target);
    Asm6502& ldx_impl(address_mode mode, link_target target);
    Asm6502& ldy_impl(address_mode mode, link_target target);
    Asm6502& ora_impl(address_mode mode, link_target target);
    Asm6502& rmw_impl(const char* mnemonic, address_mode mode, link_target target,
                      std::uint8_t zp_opcode, std::uint8_t zpx_opcode,
                      std::uint8_t abs_opcode, std::uint8_t abx_opcode);
    Asm6502& sbc_impl(address_mode mode, link_target target);
    Asm6502& sta_impl(address_mode mode, link_target target);
    Asm6502& stx_impl(address_mode mode, link_target target);
    Asm6502& sty_impl(address_mode mode, link_target target);
};

/// Build a reset bootstrap that sets A/X/Y/P/S and jumps to start_at.
std::vector<mem_value> bootstrap_program(std::uint8_t A,
                                         std::uint8_t X,
                                         std::uint8_t Y,
                                         std::uint8_t P,
                                         std::uint8_t S,
                                         std::uint16_t start_at,
                                         std::uint16_t reset_vector,
                                         std::uint16_t brk_irq_vector,
                                         std::uint16_t nmi_vector);

} // namespace asm6502
