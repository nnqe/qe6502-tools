#include <qe6502/qe6502.h>
#include <qe6502/qe6502_abi.h>
#include <stdbool.h>

static const uint8_t flag_C  = qe6502_flag_C ;
static const uint8_t flag_Z  = qe6502_flag_Z ;
static const uint8_t flag_I  = qe6502_flag_I ;
static const uint8_t flag_D  = qe6502_flag_D ;
static const uint8_t flag_B  = qe6502_flag_B ;
static const uint8_t flag_UN = qe6502_flag_UN;
static const uint8_t flag_V  = qe6502_flag_V ;
static const uint8_t flag_N  = qe6502_flag_N ;

static inline uint8_t flag_C_if(bool cond) { return cond ? flag_C : 0; }
static inline uint8_t flag_Z_if(bool cond) { return cond ? flag_Z : 0; }
static inline uint8_t flag_V_if(bool cond) { return cond ? flag_V : 0; }

static inline uint8_t stack_status(uint8_t status, uint8_t break_flag)
{
    return (uint8_t)((status | flag_UN | break_flag) & (uint8_t)(~flag_B | break_flag));
}

static inline uint8_t u16_get_byte(uint16_t x, unsigned byte_index)
{
    return (uint8_t)(x >> (byte_index * 8));
}

static inline uint16_t u16_set_byte(uint16_t x, unsigned byte_index, uint8_t value)
{
    unsigned shift = byte_index * 8;
    uint16_t mask = (uint16_t)((uint16_t)0xFFu << shift);
    return (uint16_t)((x & (uint16_t)~mask) | ((uint16_t)value << shift));
}

typedef enum service_slot
{
    service_slot_microcode_hijacked = 256,
    service_slot_illegal_ext = 257,
    service_slot_internal_reset = 258,
    service_slot_reset_0 = 259,
    service_slot_reset_1 = 260,
    service_slot_goto = 261,
    service_slot_nmi = 262,
    service_slot_irq = 263,

    service_slot_count_used
} service_slot_t;

#define IDX(model, slot, cycle) QE6502_IDX(model, slot, cycle)
#define SERVICE_SLOT_IDX(model, service, cycle) QE6502_SERVICE_SLOT_IDX(model, service, cycle)

static inline void enter_service_slot(qe6502_t* cpu, service_slot_t slot, uint8_t cycle)
{
    cpu->microcode = (uint16_t)SERVICE_SLOT_IDX(cpu->model, slot, cycle);
}

static inline void next_enter_service_slot(qe6502_t* cpu, service_slot_t slot, uint8_t cycle)
{
    enter_service_slot(cpu, slot, cycle);
    cpu->microcode--;
}

static inline void loop_here(qe6502_t* cpu)
{
    cpu->microcode--;
}

static inline uint8_t current_opcode_slot(const qe6502_t* cpu)
{
    return (uint8_t)((cpu->microcode >> 3) & 0x00ffu);
}

static inline void set_latch_addr0(qe6502_t* cpu, uint8_t value)
{
    cpu->latch_addr = u16_set_byte(cpu->latch_addr, 0, value);
}

static inline void set_latch_addr1(qe6502_t* cpu, uint8_t value)
{
    cpu->latch_addr = u16_set_byte(cpu->latch_addr, 1, value);
}

static inline qe6502_tick_t read(const qe6502_t* cpu, uint16_t address)
{
    (void)cpu;

    return (qe6502_tick_t){
        .address = address,
        .status = (uint8_t)(0)
    };
}

static inline qe6502_tick_t write(const qe6502_t* cpu, uint16_t address, uint8_t data)
{
    (void)cpu;

    return (qe6502_tick_t){
        .address = address,
        .bus = data,
        .status = (uint8_t)(qe6502_status_writing)
    };
}

static inline qe6502_tick_t fetch(qe6502_t* cpu)
{
    qe6502_tick_t tick = read(cpu, cpu->PC);
    tick.status = (uint8_t)(tick.status | qe6502_status_opcode_fetch);
    return tick;
}

static inline qe6502_tick_t fake_fetch(qe6502_t* cpu)
{
    return fetch(cpu);
}

static inline qe6502_tick_t stack_write(qe6502_t* cpu, uint8_t data)
{
    uint16_t address = (uint16_t)(0x0100 | cpu->S);
    cpu->S--;
    return write(cpu, address, data);
}

static inline qe6502_tick_t stack_read(qe6502_t* cpu)
{
    cpu->S++;
    return read(cpu, (uint16_t)(0x0100u | cpu->S));
}

static inline void update_flags_nz(qe6502_t* cpu, uint8_t value)
{
    const uint8_t mask = (uint8_t)(flag_Z | flag_N);
    const uint8_t flags = (uint8_t)(flag_Z_if(value == 0u) | (value & flag_N));

    cpu->P = (uint8_t)((cpu->P & (uint8_t)(~mask)) | flags);
}

static inline void update_flags_nzc(qe6502_t* cpu, uint8_t value, uint8_t carry)
{
    const uint8_t mask = (uint8_t)(flag_C | flag_Z | flag_N);
    const uint8_t flags = (uint8_t)(
        (carry & flag_C) |
        flag_Z_if(value == 0u) |
        (value & flag_N)
        );

    cpu->P = (uint8_t)((cpu->P & (uint8_t)(~mask)) | flags);
}

static inline void compare_register_with_value(qe6502_t* cpu, uint8_t reg, uint8_t value)
{
    const uint8_t result = (uint8_t)(reg - value);
    update_flags_nzc(cpu, result, flag_C_if(reg >= value));
}

static inline uint8_t asl_value(qe6502_t* cpu, uint8_t value)
{
    const uint8_t result = (uint8_t)(value << 1);
    update_flags_nzc(cpu, result, flag_C_if((value & flag_N) != 0u));
    return result;
}

static inline uint8_t lsr_value(qe6502_t* cpu, uint8_t value)
{
    const uint8_t result = (uint8_t)(value >> 1);
    update_flags_nzc(cpu, result, flag_C_if((value & 1u) != 0u));
    return result;
}

static inline uint8_t rol_value(qe6502_t* cpu, uint8_t value)
{
    const uint8_t old_carry = (uint8_t)(cpu->P & flag_C);
    const uint8_t result = (uint8_t)((value << 1) | old_carry);
    update_flags_nzc(cpu, result, flag_C_if((value & flag_N) != 0u));
    return result;
}

static inline uint8_t ror_value(qe6502_t* cpu, uint8_t value)
{
    const uint8_t old_carry = (uint8_t)(cpu->P & flag_C);
    const uint8_t result = (uint8_t)((value >> 1) | (uint8_t)(old_carry << 7));
    update_flags_nzc(cpu, result, flag_C_if((value & 1u) != 0u));
    return result;
}

static inline void update_flags_cvzn(qe6502_t* cpu, uint8_t flags)
{
    const uint8_t mask = (uint8_t)(flag_C | flag_Z | flag_V | flag_N);
    cpu->P = (uint8_t)((cpu->P & (uint8_t)(~mask)) | (flags & mask));
}

static inline uint8_t arr_binary_value(qe6502_t* cpu, uint8_t value)
{
    const uint8_t old_carry = (uint8_t)(cpu->P & flag_C);
    const uint8_t result = (uint8_t)((value >> 1) | (uint8_t)(old_carry << 7));
    const uint8_t bit5 = (uint8_t)(result & 0x20u);
    const uint8_t bit6 = (uint8_t)(result & 0x40u);
    const uint8_t flags = (uint8_t)(
        flag_C_if(bit6 != 0u) |
        flag_Z_if(result == 0u) |
        (result & flag_N) |
        flag_V_if(((bit6 >> 1u) ^ bit5) != 0u)
        );

    update_flags_cvzn(cpu, flags);
    return result;
}

static inline uint8_t arr_decimal_nmos_value(qe6502_t* cpu, uint8_t value)
{
    const uint8_t result = arr_binary_value(cpu, value);
    uint8_t adjusted = result;

    if (((value & 0x0fu) + (value & 0x01u)) > 5u)
    {
        const uint8_t adjusted_low = (uint8_t)((adjusted + 0x06u) & 0x0fu);
        adjusted = (uint8_t)((adjusted & 0xf0u) | adjusted_low);
    }

    if (((value & 0xf0u) + (value & 0x10u)) > 0x50u)
    {
        adjusted = (uint8_t)(adjusted + 0x60u);
        cpu->P = (uint8_t)(cpu->P | flag_C);
    }
    else
    {
        cpu->P = (uint8_t)(cpu->P & (uint8_t)(~flag_C));
    }

    return adjusted;
}

static inline void adc_binary(qe6502_t* cpu, uint8_t value)
{
    const uint8_t carry = ((cpu->P & flag_C) != 0u) ? 1u : 0u;
    const uint16_t result16 = (uint16_t)((uint16_t)cpu->A + (uint16_t)value + (uint16_t)carry);
    const uint8_t result = (uint8_t)result16;

    const uint8_t flags = (uint8_t)(
        flag_C_if(result16 > 0xffu) |
        flag_Z_if(result == 0u) |
        (result & flag_N) |
        flag_V_if((((uint8_t)(~(cpu->A ^ value)) & (cpu->A ^ result)) & flag_N) != 0u)
        );

    update_flags_cvzn(cpu, flags);
    cpu->A = result;
}

static inline void adc_decimal_nmos(qe6502_t* cpu, uint8_t value)
{
    uint8_t flags = 0;

    const uint8_t carry = ((cpu->P & flag_C) != 0u) ? 1u : 0u;
    const uint8_t bin_result = (uint8_t)(cpu->A + value + carry);
    flags |= flag_Z_if(bin_result == 0u);

    uint8_t low = (uint8_t)((cpu->A & 0x0fu) + (value & 0x0fu) + carry);
    uint8_t high = (uint8_t)((cpu->A >> 4) + (value >> 4));
    if (low > 9u)
    {
        low = (uint8_t)(low - 10u);
        high++;
    }

    uint8_t result = (uint8_t)(((unsigned int)(uint8_t)high << 4u) |
                               ((unsigned int)(uint8_t)low & 0x0fu));
    flags |= (uint8_t)(result & flag_N);
    if ((((uint8_t)(~(cpu->A ^ value)) & (cpu->A ^ result)) & flag_N) != 0u)
    {
        flags |= flag_V;
    }

    if (high > 9u)
    {
        result = (uint8_t)(result - (10u * 16u));
        flags |= flag_C;
    }

    update_flags_cvzn(cpu, flags);
    cpu->A = result;
}

static inline void adc_value(qe6502_t* cpu, uint8_t value)
{
    if ((cpu->P & flag_D) == 0u)
    {
        adc_binary(cpu, value);
    }
    else
    {
        adc_decimal_nmos(cpu, value);
    }
}

static inline void sbc_decimal_nmos(qe6502_t* cpu, uint8_t value)
{
    uint8_t flags = flag_C;

    const uint8_t carry = ((cpu->P & flag_C) != 0u) ? 1u : 0u;
    const uint8_t bin_result = (uint8_t)(cpu->A + (uint8_t)(value ^ 0xffu) + carry);
    flags |= flag_Z_if(bin_result == 0u);

    const int8_t carry_inv = (carry == 0u) ? 1 : 0;
    int8_t low = (int8_t)((int8_t)(cpu->A & 0x0fu) - (int8_t)(value & 0x0fu) - carry_inv);
    int8_t high = (int8_t)((int8_t)(cpu->A >> 4) - (int8_t)(value >> 4));

    if (low < 0)
    {
        low = (int8_t)(low + 10);
        high = (int8_t)(high - 1);
    }

    uint8_t result = (uint8_t)(((unsigned int)(uint8_t)high << 4u) |
                               ((unsigned int)(uint8_t)low & 0x0fu));
    flags |= (uint8_t)(result & flag_N);
    if ((((cpu->A ^ value) & (cpu->A ^ result)) & flag_N) != 0u)
    {
        flags |= flag_V;
    }

    if (high < 0)
    {
        result = (uint8_t)(result + (10u * 16u));
        flags = (uint8_t)(flags & (uint8_t)(~flag_C));
    }

    update_flags_cvzn(cpu, flags);
    cpu->A = result;
}

static inline void sbc_value(qe6502_t* cpu, uint8_t value)
{
    if ((cpu->P & flag_D) == 0u)
    {
        adc_binary(cpu, (uint8_t)(value ^ 0xffu));
    }
    else
    {
        sbc_decimal_nmos(cpu, value);
    }
}

static inline void adc_decimal_cmos(qe6502_t* cpu, uint8_t value)
{
    const uint8_t carry = ((cpu->P & flag_C) != 0u) ? 1u : 0u;
    uint16_t result = (uint16_t)((cpu->A & 0x0fu) + (value & 0x0fu) + carry);
    uint8_t flags = 0u;

    if (result >= 0x0au)
    {
        result = (uint16_t)(0x10u | ((result + 6u) & 0x0fu));
    }

    result = (uint16_t)(result + (cpu->A & 0xf0u) + (value & 0xf0u));
    if (result >= 0xa0u)
    {
        flags = (uint8_t)(flags | flag_C);
        flags = (uint8_t)(flags | flag_V_if((result < 0x180u) && (((cpu->A ^ value) & flag_N) == 0u)));
        result = (uint16_t)(result + 0x60u);
    }
    else
    {
        flags = (uint8_t)(flags | flag_V_if((result >= 0x80u) && (((cpu->A ^ value) & flag_N) == 0u)));
    }

    const uint8_t result8 = (uint8_t)result;
    flags = (uint8_t)(flags | flag_Z_if(result8 == 0u) | (result8 & flag_N));

    update_flags_cvzn(cpu, flags);
    cpu->A = result8;
}

static inline void sbc_decimal_cmos(qe6502_t* cpu, uint8_t value)
{
    const uint8_t carry = ((cpu->P & flag_C) != 0u) ? 1u : 0u;
    uint8_t low = (uint8_t)(0x0fu + (cpu->A & 0x0fu) - (value & 0x0fu) + carry);
    uint16_t high = (uint16_t)((uint16_t)(cpu->A & 0xf0u) - (uint16_t)(value & 0xf0u));
    uint8_t flags = 0u;

    if (low < 0x10u)
    {
        low = (uint8_t)(low - 0x06u);
    }
    else
    {
        high = (uint16_t)(high + 0x10u);
        low = (uint8_t)(low - 0x10u);
    }

    const uint8_t overflow_probe = (uint8_t)((high & 0xf0u) + (low & 0x0fu) - 0x10u);
    if ((((cpu->A ^ value) & (cpu->A ^ overflow_probe)) & flag_N) != 0u)
    {
        flags = (uint8_t)(flags | flag_V);
    }

    high = (uint16_t)(high + 0xf0u);
    if (high < 0x100u)
    {
        high = (uint16_t)(high - 0x60u);
    }
    else
    {
        flags = (uint8_t)(flags | flag_C);
    }

    const uint8_t result = (uint8_t)(high + low);
    flags = (uint8_t)(flags | flag_Z_if(result == 0u) | (result & flag_N));

    update_flags_cvzn(cpu, flags);
    cpu->A = result;
}

static inline uint16_t calculate_reset_pc(uint16_t pc, uint8_t bus)
{
    uint8_t old_high = (uint8_t)(pc >> 8);
    uint8_t new_low  = (uint8_t)(old_high - 1);
    uint8_t new_high = bus;
    return (uint16_t)(((uint16_t)new_high << 8) | new_low);
}

static inline uint8_t flag(uint8_t flags, uint8_t mask)
{
    return (uint8_t)(flags & mask);
}

static inline uint8_t flag_on(uint8_t flags, uint8_t mask)
{
    return (uint8_t)(flags | mask);
}

static inline uint8_t flag_off(uint8_t flags, uint8_t mask)
{
    return (uint8_t)(flags & (~mask));
}

static inline void update_nmi_last_sampled(qe6502_t* cpu)
{
    if (flag(cpu->interrupts, qe6502_interrupt_nmi_inv_pin) != 0)
    {
        cpu->interrupts = flag_on(cpu->interrupts, qe6502_interrupt_nmi_inv_last_sampled_pin);
    }
    else
    {
        cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_nmi_inv_last_sampled_pin);
    }
}

static inline uint8_t find_active_interrupt(uint8_t interrupts, uint8_t cpu_flags)
{
    if (flag(interrupts, qe6502_interrupt_nmi_edge) != 0u)
    {
        interrupts = flag_off(interrupts, qe6502_interrupt_nmi_edge);
        interrupts = flag_on(interrupts, qe6502_interrupt_nmi_taken);
        interrupts = flag_off(interrupts, qe6502_interrupt_irq_taken);
    }
    else if(flag(cpu_flags, flag_I) == 0u && flag(interrupts, qe6502_interrupt_irq_inv_pin) != 0u)
    {
        interrupts = flag_on(interrupts, qe6502_interrupt_irq_taken);
        interrupts = flag_off(interrupts, qe6502_interrupt_nmi_taken);
    }
    return interrupts;
}

static qe6502_tick_t read_pc_inc(qe6502_t* cpu)
{
    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* shared_handler; role=kil_jam; action=read_jam_vector_high */
static qe6502_tick_t interrupt_resolver(qe6502_t* cpu, uint8_t bus)
{
    if (flag(cpu->interrupts, qe6502_interrupt_sampling_off) == 0u)
    {
        if (flag(cpu->interrupts, qe6502_interrupt_nmi_inv_pin) != 0u &&
            flag(cpu->interrupts, qe6502_interrupt_nmi_inv_last_sampled_pin) == 0u)
        {
            cpu->interrupts = flag_on(cpu->interrupts, qe6502_interrupt_nmi_edge);
        }
        update_nmi_last_sampled(cpu);
    }

    uint8_t initial_cpu_flags = cpu->P;

    qe6502_tick_t tick = qe6502_control_store[cpu->microcode](cpu, bus);
    cpu->microcode++;

    if(flag(cpu->interrupts, qe6502_interrupt_sampling) != 0u)
    {
        cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_sampling);
        cpu->interrupts = find_active_interrupt(cpu->interrupts, initial_cpu_flags);
    }
    else if((tick.status & qe6502_status_opcode_fetch) != 0u)
    {
        if (flag(cpu->interrupts, qe6502_interrupt_nmi_taken) != 0u)
        {
            cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_nmi_taken);
            cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_irq_taken);
            enter_service_slot(cpu, service_slot_nmi, 0);
            return tick;
        }
        else if(flag(cpu->interrupts, qe6502_interrupt_irq_taken) != 0u)
        {
            cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_nmi_taken);
            cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_irq_taken);
            enter_service_slot(cpu, service_slot_irq, 0);
            return tick;
        }
    }

    if( flag(cpu->interrupts, qe6502_interrupt_nmi_edge) == 0u &&
        flag(cpu->interrupts, qe6502_interrupt_irq_inv_pin) == 0u &&
        (
            flag(cpu->interrupts, qe6502_interrupt_nmi_inv_pin) == 0u ||
            flag(cpu->interrupts, qe6502_interrupt_nmi_inv_last_sampled_pin) != 0u
        ) &&
        flag(cpu->interrupts, qe6502_interrupt_nmi_taken) == 0u &&
        flag(cpu->interrupts, qe6502_interrupt_irq_taken) == 0u)
    {
        cpu->hijack_microcode = 0;
    }

    return tick;
}

static inline void prefetch(qe6502_t* cpu)
{
    cpu->interrupts = flag_on(cpu->interrupts, qe6502_interrupt_sampling);
}

/* Microcode handlers. */

/* shared_handler; role=kil_jam; action=read_jam_vector_high */
static qe6502_tick_t mc_kil_jam_read_ffff(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, 0xffffu);
}

/* common_handler; role=read_fixed_fffe; action=read_fixed_address_fffe */
static qe6502_tick_t mc_read_fffe(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, 0xfffeu);
}

/* shared_handler; role=kil_jam; action=repeat_jammed_vector_high_read_forever */
static qe6502_tick_t op_kil_jam_r_ffff_pending_data_loop(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    loop_here(cpu);
    return (qe6502_tick_t){
        .address = 0xffffu,
        .bus = 0xffu,
        .status = (uint8_t)(qe6502_status_cpu_jammed)
    };
}

/* shared_handler; role=dispatch; action=consume_fetched_opcode_and_dispatch_to_opcode_class */
static qe6502_tick_t mc_dispatch(qe6502_t* cpu, uint8_t bus)
{
    cpu->microcode = (uint16_t)(((uint16_t)cpu->model << 12u) | ((uint16_t)bus << 3u));
    cpu->PC++;
    return qe6502_control_store[cpu->microcode](cpu, bus);
}

/* shared_handler; role=fetch; action=consume_previous_bus_cycle_and_fetch_next_opcode */
static qe6502_tick_t mc_fetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return fetch(cpu);
}

/* shared_handler; role=fetch_illegal_ext; action=fetch_next_opcode_then_continue_at_illegal_extension_service_dispatch */
static qe6502_tick_t mc_fetch_illegal_ext_dispatch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    next_enter_service_slot(cpu, service_slot_illegal_ext, 0);
    return fetch(cpu);
}

/* shared_handler; role=read_pc_inc; action=read_pc_and_increment_pc */
static qe6502_tick_t mc_read_pc_inc(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read_pc_inc(cpu);
}

/* shared_handler; role=read_pc; action=read_pc_without_incrementing_pc */
static qe6502_tick_t mc_read_pc(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, cpu->PC);
}

/* shared_handler; role=clear_latch_addr_read_pc_inc; action=clear_effective_address_then_read_pc_and_increment_pc */
static qe6502_tick_t mc_clear_latch_addr_read_pc_inc(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->latch_addr = 0u;
    return read_pc_inc(cpu);
}

/* shared_handler; role=clear_latch_addr_read_pc; action=clear_effective_address_then_read_pc_without_incrementing_pc */
static qe6502_tick_t mc_clear_latch_addr_read_pc(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->latch_addr = 0u;
    return read(cpu, cpu->PC);
}

/* shared_handler; role=read_latch_addr; action=read_effective_address_from_latch_addr */
static qe6502_tick_t mc_read_latch_addr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, cpu->latch_addr);
}

/* shared_handler; role=latch_data_write_latch_addr; action=latch_bus_data_then_write_effective_address */
static qe6502_tick_t mc_latch_data_write_latch_addr(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;
    return write(cpu, cpu->latch_addr, cpu->latch_data);
}


/* shared_handler; role=read_stack_current_s; action=read_stack_address_without_incrementing_s */
static qe6502_tick_t mc_read_stack_current_s(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, (uint16_t)(0x0100u | cpu->S));
}

/* common_handler; action=ignore_bus_increment_s_and_read_stack */
static qe6502_tick_t mc_stack_pull_read(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return stack_read(cpu);
}

static qe6502_tick_t mc_internal_reset(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data++;
    cpu->PC = calculate_reset_pc(cpu->PC, bus);
    if (cpu->latch_data == 8)
    {
        next_enter_service_slot(cpu, service_slot_reset_0, 0);
        return read(cpu, cpu->PC);
    }
    /* else */
    loop_here(cpu);
    qe6502_tick_t tick = read(cpu, cpu->PC);
    tick.status = flag_on(tick.status, qe6502_status_internal_reset);
    return tick;
}

static qe6502_tick_t mc_restart_read_hhff(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = calculate_reset_pc(cpu->PC, bus);
    return read(cpu, cpu->PC);
}

static qe6502_tick_t mc_restart_read_zzhh_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = calculate_reset_pc(cpu->PC, bus);
    return fake_fetch(cpu);
}

static qe6502_tick_t mc_restart_read_zzhh(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, cpu->PC);
}

/* service_handler; role=reset_stack_read; action=read_stack_current_s_and_decrement_s */
static qe6502_tick_t mc_reset_stack_read(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    const uint16_t address = (uint16_t)(0x0100u | cpu->S);
    cpu->S--;
    return read(cpu, address);
}

/* service_handler; role=vec_lo; action=read_reset_vector_low */
static qe6502_tick_t mc_reset_vec_lo(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, 0xfffcu);
}

/* service_handler; role=vec_hi; action=consume_reset_vector_low_set_interrupt_disable_and_read_reset_vector_high */
static qe6502_tick_t mc_reset_vec_hi(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = u16_set_byte(cpu->PC, 0, bus);
    cpu->P = flag_on(cpu->P, flag_B | flag_I );
    return read(cpu, 0xfffdu);
}

/* interrupt_handler; role=latch_pch_interrupt_fetch; action=consume_vector_high_and_request_interrupt_handler_opcode */
static qe6502_tick_t mc_latch_pch_reset_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = u16_set_byte(cpu->PC, 1, bus);
    return fetch(cpu);
}

/* interrupt_handler; role=latch_pch_interrupt_fetch; action=consume_vector_high_and_request_interrupt_handler_opcode */
static qe6502_tick_t mc_latch_pch_nmi_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = u16_set_byte(cpu->PC, 1, bus);
    return fetch(cpu);
}

/* interrupt_handler; role=latch_pch_interrupt_fetch; action=consume_vector_high_and_request_interrupt_handler_opcode */
static qe6502_tick_t mc_latch_pch_irq_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = u16_set_byte(cpu->PC, 1, bus);
    return fetch(cpu);
}

/* interrupt_handler; role=latch_pch_interrupt_fetch; action=consume_vector_high_and_request_interrupt_handler_opcode */
static qe6502_tick_t mc_latch_pch_brk_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = u16_set_byte(cpu->PC, 1, bus);
    return fetch(cpu);
}

/* interrupt_handler; role=latch_pch_interrupt_fetch; action=consume_vector_high_and_request_interrupt_handler_opcode */
static qe6502_tick_t mc_latch_pch_rti_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = u16_set_byte(cpu->PC, 1, bus);
    return fetch(cpu);
}

/* control_handler; role=apply; action=apply_signed_offset_to_pc_low_and_request_branch_dummy_read */
static inline qe6502_tick_t mc_branch_rel_c1_apply(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t old_pc = cpu->PC;
    const int8_t offset = (int8_t)bus;
    const uint16_t target = (uint16_t)(old_pc + offset);

    qe6502_tick_t tick = read(cpu, old_pc);
    cpu->PC = u16_set_byte(cpu->PC, 0, u16_get_byte(target, 0));

    if (u16_get_byte(old_pc, 1) == u16_get_byte(target, 1))
    {
        cpu->microcode++;
    }
    else
    {
        cpu->latch_addr = target;
    }
    return tick;
}

/* control_handler; role=pgfix; action=perform_page_cross_branch_correction_read_and_commit_target_pc */
static inline qe6502_tick_t mc_branch_pgfix_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC = cpu->latch_addr;
    return tick;
}

/* control_handler; model=wdc; role=apply; action=apply_bbr_bbs_signed_offset_and_dummy_read_branch_base_address */
static qe6502_tick_t mc_wdc_bbr_bbs_c4_apply(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t old_pc = cpu->PC;
    const int8_t offset = (int8_t)bus;
    const uint16_t target = (uint16_t)(old_pc + offset);

    qe6502_tick_t tick = read(cpu, old_pc);

    if (u16_get_byte(old_pc, 1) == u16_get_byte(target, 1))
    {
        cpu->PC = target;
        cpu->microcode++;
    }
    else
    {
        cpu->latch_addr = target;
    }
    return tick;
}

/* common_handler; role=push_pc_high; action=push_pc_high_to_stack */
static qe6502_tick_t mc_stack_push_pc_high(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return stack_write(cpu, u16_get_byte(cpu->PC, 1));
}

/* common_handler; role=push_pc_low; action=push_pc_low_to_stack */
static qe6502_tick_t mc_stack_push_pc_low(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return stack_write(cpu, u16_get_byte(cpu->PC, 0));
}

/* common_handler; role=push_status_b; action=push_status_with_break_flag_to_stack */
static qe6502_tick_t mc_stack_push_status_b(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    qe6502_tick_t tick = stack_write(cpu, stack_status(cpu->P, flag_B));
    cpu->interrupts = find_active_interrupt(cpu->interrupts, cpu->P);
    cpu->interrupts = flag_on(cpu->interrupts, qe6502_interrupt_sampling_off);
    if (flag(cpu->interrupts, qe6502_interrupt_nmi_taken) != 0u)
    {
        cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_nmi_taken);
        cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_irq_taken);
        next_enter_service_slot(cpu, service_slot_nmi, 4);
    }
    else if(flag(cpu->interrupts, qe6502_interrupt_irq_taken) != 0u)
    {
        cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_nmi_taken);
        cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_irq_taken);
        next_enter_service_slot(cpu, service_slot_irq, 4);
    }
    return tick;
}

/* common_handler; role=push_status_b; action=push_status_with_break_flag_to_stack */
static qe6502_tick_t mc_cmos_stack_push_status_b(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return stack_write(cpu, stack_status(cpu->P, flag_B));
}

/* special_handler; role=vec_hi; action=read_brk_vector_high_to_pc_high_and_set_interrupt_disable */
static qe6502_tick_t mc_brk_c5_vec_hi(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = u16_set_byte(cpu->PC, 0, bus);
    cpu->P = (uint8_t)(cpu->P | flag_I);
    cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_sampling_off);

    return read(cpu, 0xffffu);
}

/* special_handler; model=cmos; role=vec_hi; action=read_brk_vector_high_to_pc_high_set_interrupt_disable_and_clear_decimal */
static qe6502_tick_t mc_cmos_brk_c5_vec_hi(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = u16_set_byte(cpu->PC, 0, bus);
    cpu->P = (uint8_t)((cpu->P | flag_I) & (uint8_t)(~flag_D));
    return read(cpu, 0xffffu);
}

/* interrupt_helper; model=nmos; role=vector_low; action=latch_vector_low_and_set_interrupt_disable */
static inline void nmos_interrupt_vector_low(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = u16_set_byte(cpu->PC, 0, bus);
    cpu->P = (uint8_t)(cpu->P | flag_I);
}

/* interrupt_helper; model=cmos; role=vector_low; action=latch_vector_low_set_interrupt_disable_and_clear_decimal */
static inline void cmos_interrupt_vector_low(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = u16_set_byte(cpu->PC, 0, bus);
    cpu->P = (uint8_t)((cpu->P | flag_I) & (uint8_t)(~flag_D));
    cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_nmi_edge);
    update_nmi_last_sampled(cpu);
}


/* interrupt_handler; role=push_p; action=push_hardware_interrupt_status_to_stack */
static inline qe6502_tick_t mc_nmi_c3_push_p(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->interrupts = find_active_interrupt(cpu->interrupts, cpu->P);
    cpu->interrupts = flag_on(cpu->interrupts, qe6502_interrupt_sampling_off);
    cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_nmi_edge);
    cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_nmi_taken);
    cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_irq_taken);
    return stack_write(cpu, stack_status(cpu->P, 0u));
}

/* interrupt_handler; role=push_p; action=push_hardware_interrupt_status_to_stack */
static inline qe6502_tick_t mc_irq_c3_push_p(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    qe6502_tick_t tick = stack_write(cpu, stack_status(cpu->P, 0u));
    cpu->interrupts = find_active_interrupt(cpu->interrupts, cpu->P);
    cpu->interrupts = flag_on(cpu->interrupts, qe6502_interrupt_sampling_off);
    cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_irq_taken);
    if (flag(cpu->interrupts, qe6502_interrupt_nmi_taken) != 0u)
    {
        cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_nmi_taken);
        next_enter_service_slot(cpu, service_slot_nmi, 4);
    }
    return tick;
}

/* interrupt_handler; role=push_p; action=push_hardware_interrupt_status_to_stack */
static inline qe6502_tick_t mc_cmos_nmi_c3_push_p(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_nmi_edge);
    cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_nmi_taken);
    update_nmi_last_sampled(cpu);

    return stack_write(cpu, stack_status(cpu->P, 0u));
}

/* interrupt_handler; role=push_p; action=push_hardware_interrupt_status_to_stack */
static inline qe6502_tick_t mc_cmos_irq_c3_push_p(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    qe6502_tick_t tick = stack_write(cpu, stack_status(cpu->P, 0u));
    cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_irq_taken);
    return tick;
}

/* interrupt_handler; role=vec_lo; action=read_nmi_vector_low_and_mark_nmi_ack */
static inline qe6502_tick_t mc_nmi_c4_vec_lo(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, 0xfffau);
}

/* interrupt_handler; model=nmos; role=vec_hi; action=consume_nmi_vector_low_set_interrupt_disable_and_read_nmi_vector_high */
static inline qe6502_tick_t mc_nmos_nmi_c5_vec_hi(qe6502_t* cpu, uint8_t bus)
{
    nmos_interrupt_vector_low(cpu, bus);
    cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_sampling_off);
    update_nmi_last_sampled(cpu);

    return read(cpu, 0xfffbu);
}

/* interrupt_handler; model=cmos; role=vec_hi; action=consume_nmi_vector_low_set_interrupt_disable_clear_decimal_and_read_nmi_vector_high */
static inline qe6502_tick_t mc_cmos_nmi_c5_vec_hi(qe6502_t* cpu, uint8_t bus)
{
    cmos_interrupt_vector_low(cpu, bus);
    return read(cpu, 0xfffbu);
}

/* interrupt_handler; role=vec_lo; action=read_irq_vector_low_and_mark_irq_ack */
static inline qe6502_tick_t mc_irq_c4_vec_lo(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, 0xfffeu);
}

/* interrupt_handler; model=nmos; role=vec_hi; action=consume_irq_vector_low_set_interrupt_disable_and_read_irq_vector_high */
static inline qe6502_tick_t mc_nmos_irq_c5_vec_hi(qe6502_t* cpu, uint8_t bus)
{
    nmos_interrupt_vector_low(cpu, bus);
    cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_sampling_off);
    return read(cpu, 0xffffu);
}

/* interrupt_handler; model=cmos; role=vec_hi; action=consume_irq_vector_low_set_interrupt_disable_clear_decimal_and_read_irq_vector_high */
static inline qe6502_tick_t mc_cmos_irq_c5_vec_hi(qe6502_t* cpu, uint8_t bus)
{
    cmos_interrupt_vector_low(cpu, bus);
    return read(cpu, 0xffffu);
}

/* common_handler; action=latch_bus_as_addr_low_read_pc_and_increment_pc */
static inline qe6502_tick_t mc_latch_addr0_read_pc_inc(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr0(cpu, bus);
    return read_pc_inc(cpu);
}

/* common_handler; role=latch_addr1_set_pc_fetch; action=set_effective_address_high_set_pc_and_fetch_next_opcode */
static inline qe6502_tick_t mc_latch_addr1_set_pc_fetch(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr1(cpu, bus);
    cpu->PC = cpu->latch_addr;
    return fetch(cpu);
}

/* special_handler; role=target_lo; action=read_pointer_to_ea_low_then_increment_pointer_low_wraparound */
static inline qe6502_tick_t mc_jmp_ind_c2_target_lo(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr1(cpu, bus);
    qe6502_tick_t tick = read(cpu, cpu->latch_addr);
    set_latch_addr0(cpu, (uint8_t)(u16_get_byte(cpu->latch_addr, 0) + 1u));
    return tick;
}

/* common_handler; action=latch_bus_as_data_read_latch_addr */
static inline qe6502_tick_t mc_latch_data_read_latch_addr(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;
    return read(cpu, cpu->latch_addr);
}

/* special_handler; role=fetch; action=set_pc_to_effective_address_and_fetch_next_opcode */
static inline qe6502_tick_t mc_jmp_ind_c4_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = u16_set_byte(cpu->latch_data, 1, bus);
    return fetch(cpu);
}

/* special_handler; model=cmos; role=dummy; action=latch_absolute_base_high_and_dummy_read_low_operand_address */
static inline qe6502_tick_t mc_cmos_jmp_ind_x_c2_dummy(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr1(cpu, bus);
    return read(cpu, (uint16_t)(cpu->PC - 2u));
}

/* special_handler; model=cmos; role=target_lo; action=add_x_to_absolute_base_read_target_low_then_increment_pointer */
static inline qe6502_tick_t mc_cmos_jmp_ind_x_c3_target_lo(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    const uint16_t pointer = (uint16_t)(cpu->latch_addr + cpu->X);
    qe6502_tick_t tick = read(cpu, pointer);
    cpu->latch_addr = (uint16_t)(pointer + 1u);
    return tick;
}

/* special_handler; role=dummy; action=dummy_stack_read_before_pushes */
static inline qe6502_tick_t mc_jsr_abs_c1_dummy(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr0(cpu, bus);
    return read(cpu, (uint16_t)(0x0100u | cpu->S));
}

/* common_handler; action=latch_bus_as_addr_high_read_latch_addr */
static inline qe6502_tick_t mc_latch_addr1_read_latch_addr(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr1(cpu, bus);
    return read(cpu, cpu->latch_addr);
}

/* common_handler; role=latch_abx_base_read_pc_inc; action=read_pc_to_ea_high_increment_pc_add_x_low_and_record_page_cross */
static inline qe6502_tick_t mc_latch_abx_base_read_pc_inc(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t indexed = (uint16_t)(bus + cpu->X);
    cpu->latch_addr = (uint16_t)(indexed & 0x00FFu);
    cpu->latch_data = (uint8_t)(indexed >> 8u);
    return read_pc_inc(cpu);
}

/* addressing_handler; model=cmos; role=latch_abx_base_read_pc; action=read_pc_to_ea_high_add_x_low_record_page_cross_without_incrementing_pc */
static inline qe6502_tick_t mc_cmos_latch_abx_base_read_pc(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t indexed = (uint16_t)(bus + cpu->X);
    cpu->latch_addr = (uint16_t)(indexed & 0x00ffu);
    cpu->latch_data = (uint8_t)(indexed >> 8u);
    return read(cpu, cpu->PC);
}

/* common_handler; role=latch_aby_base_read_pc_inc; action=read_pc_to_ea_high_increment_pc_add_y_low_and_record_page_cross */
static inline qe6502_tick_t mc_latch_aby_base_read_pc_inc(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t indexed = (uint16_t)(bus + cpu->Y);
    cpu->latch_addr = (uint16_t)(indexed & 0x00FFu);
    cpu->latch_data = (uint8_t)(indexed >> 8u);
    return read_pc_inc(cpu);
}

/* addressing_handler; model=cmos; role=latch_aby_base_read_pc; action=read_pc_to_ea_high_add_y_low_record_page_cross_without_incrementing_pc */
static inline qe6502_tick_t mc_cmos_latch_aby_base_read_pc(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t indexed = (uint16_t)(bus + cpu->Y);
    cpu->latch_addr = (uint16_t)(indexed & 0x00ffu);
    cpu->latch_data = (uint8_t)(indexed >> 8u);
    return read(cpu, cpu->PC);
}

/* addressing_handler; model=cmos; role=r_indexed_probe; action=read_cmos_indexed_data_or_pc_dummy_and_increment_pc */
static inline qe6502_tick_t mc_cmos_r_indexed_probe(qe6502_t* cpu, uint8_t bus)
{
    const uint8_t page_cross = cpu->latch_data;
    set_latch_addr1(cpu, (uint8_t)(bus + page_cross));
    if (page_cross == 0u)
    {
        cpu->PC++;
        cpu->microcode++;
        return read(cpu, cpu->latch_addr);
    }
    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; model=cmos; role=w_indexed_probe; action=dummy_read_pc_increment_pc_and_fix_effective_address_high */
static inline qe6502_tick_t mc_cmos_w_indexed_probe(qe6502_t* cpu, uint8_t bus)
{
    const uint8_t page_cross = cpu->latch_data;
    set_latch_addr1(cpu, (uint8_t)(bus + page_cross));
    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; role=idx; action=dummy_read_zp_pointer_then_add_x_wraparound */
static inline qe6502_tick_t mc_r_izx_c1_idx(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr0(cpu, bus);
    qe6502_tick_t tick = read(cpu, cpu->latch_addr);
    set_latch_addr0(cpu, (uint8_t)(bus + cpu->X));
    return tick;
}

/* common_handler; role=izx_read_ptrlo_inc_ptr; action=read_zp_pointer_to_ea_low_then_increment_pointer_wraparound */
static inline qe6502_tick_t mc_izx_read_ptrlo_inc_ptr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    const uint16_t addr = cpu->latch_addr;
    qe6502_tick_t tick = read(cpu, addr);
    set_latch_addr0(cpu, (uint8_t)(u16_get_byte(addr, 0) + 1u));
    return tick;
}

/* addressing_handler; model=cmos; role=ptrhi; action=latch_effective_address_low_then_read_zp_pointer_high_wraparound */
static inline qe6502_tick_t mc_izp_c2_ptrhi(qe6502_t* cpu, uint8_t bus)
{
    const uint8_t ptr = u16_get_byte(cpu->latch_addr, 0);
    const uint16_t ptr_hi_addr = (uint8_t)(ptr + 1u);
    set_latch_addr0(cpu, bus);
    return read(cpu, ptr_hi_addr);
}

/* addressing_handler; role=data; action=read_effective_address_to_data */
static inline qe6502_tick_t mc_r_izx_c4_data(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_addr = u16_set_byte(cpu->latch_data, 1, bus);
    return read(cpu, cpu->latch_addr);
}

/* common_handler; action=latch_bus_as_addr_low_read_latch_addr */
static inline qe6502_tick_t mc_latch_addr0_read_latch_addr(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr0(cpu, bus);
    return read(cpu, cpu->latch_addr);
}

/* addressing_handler; role=ptrhi; action=increment_pointer_read_ea_high_add_y_low_and_record_page_cross */
static inline qe6502_tick_t mc_r_izy_c2_ptrhi(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t indexed = (uint16_t)(bus + cpu->Y);
    const uint16_t ptr_hi_addr = u16_set_byte(
        cpu->latch_addr,
        0,
        (uint8_t)(u16_get_byte(cpu->latch_addr, 0) + 1u)
        );

    cpu->latch_addr = (uint16_t)(indexed & 0x00FFu);
    cpu->latch_data = (uint8_t)(indexed >> 8u);
    return read(cpu, ptr_hi_addr);
}

/* addressing_handler; role=data; action=latch_zero_page_effective_address_and_read_data */
static qe6502_tick_t mc_latch_zp_addr_read_latch_addr(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_addr = bus;
    return read(cpu, (uint16_t)bus);
}

/* addressing_handler; role=idx; action=dummy_read_zero_page_base_then_add_x_wraparound */
static qe6502_tick_t mc_zpx_c1_idx(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t addr = u16_set_byte(cpu->latch_addr, 0, bus);
    qe6502_tick_t tick = read(cpu, addr);
    set_latch_addr0(cpu, (uint8_t)(bus + cpu->X));
    return tick;
}

/* addressing_handler; role=idx; action=dummy_read_zero_page_base_then_add_y_wraparound */
static qe6502_tick_t mc_r_zpy_c1_idx(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t addr = u16_set_byte(cpu->latch_addr, 0, bus);
    qe6502_tick_t tick = read(cpu, addr);
    set_latch_addr0(cpu, (uint8_t)(bus + cpu->Y));
    return tick;
}

/* special_handler; role=pull_pcl; action=normalize_status_increment_s_and_read_pc_low */
static qe6502_tick_t mc_rti_c3_pull_pcl(qe6502_t* cpu, uint8_t bus)
{
    cpu->P = (uint8_t)((bus | flag_UN) & (uint8_t)(~flag_B));
    return stack_read(cpu);
}

/* common_handler; role=latch_pcl_stack_read; action=consume_pc_low_increment_s_and_read_pc_high */
static qe6502_tick_t mc_latch_pcl_stack_read(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = u16_set_byte(cpu->PC, 0, bus);
    return stack_read(cpu);
}

/* rw_abx c1: capture low byte, add X to low byte, request high operand byte */
static inline qe6502_tick_t mc_rw_abx_c1_hi(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t indexed = (uint16_t)((uint16_t)bus + cpu->X);
    cpu->latch_addr = (uint16_t)(indexed & 0x00FFu);
    cpu->latch_data = (uint8_t)(indexed >> 8u);
    return read_pc_inc(cpu);
}

/* common_handler; role=w_indexed_probe; action=dummy_read_indexed_probe_then_fix_ea_high */
static qe6502_tick_t mc_w_indexed_probe(qe6502_t* cpu, uint8_t bus)
{
    const uint8_t page_cross = cpu->latch_data;
    const uint16_t addr = u16_set_byte(cpu->latch_addr, 1, bus);
    qe6502_tick_t tick = read(cpu, addr);
    set_latch_addr1(cpu, (uint8_t)(bus + page_cross));
    return tick;
}

/* common_handler; role=unstable_store_indexed_probe_preserve_base_high; action=dummy_read_indexed_probe_and_preserve_base_high_for_unstable_store */
static qe6502_tick_t mc_unstable_store_indexed_probe_preserve_base_high(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t address = u16_set_byte(cpu->latch_addr, 1, bus);
    qe6502_tick_t tick = read(cpu, address);
    set_latch_addr1(cpu, bus);
    return tick;
}

/* common_handler; role=read_zp_latch_x; action=dummy_read_zero_page_base_then_add_x_wraparound */
static qe6502_tick_t mc_read_zp_latch_x(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t addr = bus;
    qe6502_tick_t tick = read(cpu, addr);
    cpu->latch_addr = (uint8_t)(bus + cpu->X);
    return tick;
}

/* addressing_handler; role=ptrhi; action=read_zp_pointer_to_ea_high */
static qe6502_tick_t mc_w_izx_c3_ptrhi(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t ptr_hi_addr = cpu->latch_addr;
    set_latch_addr0(cpu, bus);
    return read(cpu, ptr_hi_addr);
}

/* addressing_handler; role=ptrhi; action=read_zp_pointer_to_ea_high_add_y_low_and_record_page_cross */
static qe6502_tick_t mc_w_izy_c2_ptrhi(qe6502_t* cpu, uint8_t bus)
{
    const uint8_t ptr = u16_get_byte(cpu->latch_addr, 0);
    const uint16_t ptr_hi_addr = (uint8_t)(ptr + 1u);
    const uint16_t indexed = (uint16_t)(bus + cpu->Y);
    cpu->latch_addr = (uint16_t)(indexed & 0x00FFu);
    cpu->latch_data = (uint8_t)(indexed >> 8u);
    return read(cpu, ptr_hi_addr);
}

/* addressing_handler; role=idx; action=dummy_read_zero_page_base_then_add_y_wraparound */
static qe6502_tick_t mc_w_zpy_c1_idx(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t addr = bus;
    qe6502_tick_t tick = read(cpu, addr);
    cpu->latch_addr = (uint8_t)(bus + cpu->Y);
    return tick;
}

/* mnemonic_handler; role=exec_fetch; action=execute_read_operand_mnemonic_and_fetch_next_opcode */
static qe6502_tick_t op_adc_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    adc_value(cpu, bus);
    return fetch(cpu);
}

/* mnemonic_handler; model=nes; role=exec_fetch; action=execute_binary_adc_operand_and_fetch_next_opcode */
static qe6502_tick_t op_nes_adc_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    adc_binary(cpu, bus);
    return fetch(cpu);
}

/* mnemonic_handler; model=cmos; role=exec_fetch; action=execute_cmos_decimal_adc_and_fetch_next_opcode */
static qe6502_tick_t op_cmos_adc_dec_ready_none_pending_dummy_fetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    adc_decimal_cmos(cpu, cpu->latch_data);
    return fetch(cpu);
}

/* mnemonic_handler; role=exec_fetch; action=execute_read_operand_mnemonic_and_fetch_next_opcode */
static qe6502_tick_t op_and_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->A = (uint8_t)(cpu->A & bus);
    update_flags_nz(cpu, cpu->A);
    return fetch(cpu);
}

/* mnemonic_handler; role=exec_fetch; action=execute_illegal_anc_immediate_and_fetch_next_opcode */
static qe6502_tick_t op_anc_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->A = (uint8_t)(cpu->A & bus);
    update_flags_nzc(cpu, cpu->A, flag_C_if((cpu->A & flag_N) != 0u));
    return fetch(cpu);
}

/* mnemonic_handler; role=exec_fetch; action=execute_illegal_alr_immediate_and_fetch_next_opcode */
static qe6502_tick_t op_alr_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->A = lsr_value(cpu, (uint8_t)(cpu->A & bus));
    return fetch(cpu);
}

/* mnemonic_handler; role=exec_fetch; action=execute_illegal_arr_immediate_and_fetch_next_opcode */
static qe6502_tick_t op_arr_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    uint8_t value = (uint8_t)(cpu->A & bus);
    if ((cpu->P & flag_D) == 0u)
    {
        cpu->A = arr_binary_value(cpu, value);
    }
    else
    {
        cpu->A = arr_decimal_nmos_value(cpu, value);
    }
    return fetch(cpu);
}

/* mnemonic_handler; model=nes; role=exec_fetch; action=execute_illegal_arr_immediate_and_fetch_next_opcode */
static qe6502_tick_t op_nes_arr_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->A = arr_binary_value(cpu, (uint8_t)(cpu->A & bus));
    return fetch(cpu);
}

/* mnemonic_handler; role=exec_fetch; action=execute_illegal_axs_immediate_and_fetch_next_opcode */
static qe6502_tick_t op_axs_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    uint8_t lhs = (uint8_t)(cpu->A & cpu->X);
    const uint8_t result = (uint8_t)(lhs - bus);
    cpu->X = result;
    update_flags_nzc(cpu, result, flag_C_if(lhs >= bus));
    return fetch(cpu);
}

/* mnemonic_handler; role=exec_fetch; action=execute_illegal_xaa_immediate_and_fetch_next_opcode */
static qe6502_tick_t op_xaa_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->A = (uint8_t)((cpu->A | 0xeeu) & cpu->X & bus);
    update_flags_nz(cpu, cpu->A);
    return fetch(cpu);
}

/* mnemonic_handler; role=exec_fetch; action=execute_illegal_lxa_immediate_and_fetch_next_opcode */
static qe6502_tick_t op_lxa_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    const uint8_t value = (uint8_t)((cpu->A | 0xeeu) & bus);
    cpu->A = value;
    cpu->X = value;
    update_flags_nz(cpu, value);
    return fetch(cpu);
}

static inline qe6502_tick_t branch_c0_offset(qe6502_t* cpu, bool taken)
{
    qe6502_tick_t tick = read_pc_inc(cpu);

    prefetch(cpu);
    if (!taken)
    {
        cpu->microcode = (uint16_t)(cpu->microcode + 2u);
    }

    return tick;
}

/* mnemonic_handler; role=exec_fetch; action=execute_read_operand_mnemonic_and_fetch_next_opcode */
static qe6502_tick_t op_bit_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    const uint8_t mask = (uint8_t)(flag_Z | flag_V | flag_N);
    const uint8_t flags = (uint8_t)(
        flag_Z_if((cpu->A & bus) == 0u) |
        (bus & (uint8_t)(flag_V | flag_N))
        );

    cpu->P = (uint8_t)((cpu->P & (uint8_t)(~mask)) | flags);
    return fetch(cpu);
}

/* mnemonic_handler; model=cmos; role=exec_fetch; action=execute_immediate_bit_test_and_fetch_next_opcode */
static qe6502_tick_t op_cmos_bit_imm_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    const uint8_t flags = flag_Z_if((cpu->A & bus) == 0u);
    cpu->P = (uint8_t)((cpu->P & (uint8_t)(~flag_Z)) | flags);
    return fetch(cpu);
}

/* mnemonic_handler; role=exec_fetch; action=execute_read_operand_mnemonic_and_fetch_next_opcode */
static qe6502_tick_t op_cmp_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    compare_register_with_value(cpu, cpu->A, bus);
    return fetch(cpu);
}

/* mnemonic_handler; role=exec_fetch; action=execute_read_operand_mnemonic_and_fetch_next_opcode */
static qe6502_tick_t op_cpx_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    compare_register_with_value(cpu, cpu->X, bus);
    return fetch(cpu);
}

/* mnemonic_handler; role=exec_fetch; action=execute_read_operand_mnemonic_and_fetch_next_opcode */
static qe6502_tick_t op_cpy_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    compare_register_with_value(cpu, cpu->Y, bus);
    return fetch(cpu);
}

/* mnemonic_handler; role=exec_fetch; action=execute_read_operand_mnemonic_and_fetch_next_opcode */
static qe6502_tick_t op_eor_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->A = (uint8_t)(cpu->A ^ bus);
    update_flags_nz(cpu, cpu->A);
    return fetch(cpu);
}

/* common_handler; role=load_a_fetch; action=load_a_update_nz_and_fetch_next_opcode */
static qe6502_tick_t mc_load_a_update_nz_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->A = bus;
    update_flags_nz(cpu, cpu->A);
    return fetch(cpu);
}

/* mnemonic_handler; role=exec_fetch; action=execute_read_operand_mnemonic_and_fetch_next_opcode */
static qe6502_tick_t op_lax_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->A = bus;
    cpu->X = bus;
    update_flags_nz(cpu, bus);
    return fetch(cpu);
}

/* mnemonic_handler; role=exec_fetch; action=execute_read_operand_mnemonic_and_fetch_next_opcode */
static qe6502_tick_t op_las_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    const uint8_t value = (uint8_t)(bus & cpu->S);
    cpu->A = value;
    cpu->X = value;
    cpu->S = value;
    update_flags_nz(cpu, value);
    return fetch(cpu);
}

/* common_handler; role=load_x_fetch; action=load_x_update_nz_and_fetch_next_opcode */
static qe6502_tick_t mc_load_x_update_nz_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->X = bus;
    update_flags_nz(cpu, cpu->X);
    return fetch(cpu);
}

/* common_handler; role=load_y_fetch; action=load_y_update_nz_and_fetch_next_opcode */
static qe6502_tick_t mc_load_y_update_nz_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->Y = bus;
    update_flags_nz(cpu, cpu->Y);
    return fetch(cpu);
}

/* mnemonic_handler; role=exec_fetch; action=execute_read_operand_mnemonic_and_fetch_next_opcode */
static qe6502_tick_t op_ora_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->A = (uint8_t)(cpu->A | bus);
    update_flags_nz(cpu, cpu->A);
    return fetch(cpu);
}

/* mnemonic_handler; role=exec_fetch; action=consume_pulled_stack_value_apply_mnemonic_semantics_and_fetch_next_opcode */
static qe6502_tick_t op_plp_stack_pull_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->P = (uint8_t)((bus | flag_UN) & (uint8_t)(~flag_B));
    return fetch(cpu);
}

/* mnemonic_handler; role=exec_fetch; action=execute_read_operand_mnemonic_and_fetch_next_opcode */
static qe6502_tick_t op_sbc_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    sbc_value(cpu, bus);
    return fetch(cpu);
}

/* mnemonic_handler; model=nes; role=exec_fetch; action=execute_binary_sbc_operand_and_fetch_next_opcode */
static qe6502_tick_t op_nes_sbc_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    adc_binary(cpu, (uint8_t)(bus ^ 0xffu));
    return fetch(cpu);
}

/* mnemonic_handler; model=cmos; role=exec_fetch; action=execute_cmos_decimal_sbc_and_fetch_next_opcode */
static qe6502_tick_t op_cmos_sbc_dec_ready_none_pending_dummy_fetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    sbc_decimal_cmos(cpu, cpu->latch_data);
    return fetch(cpu);
}

/* common_helper; role=unstable_store; action=write_value_to_base_or_value_high_address */
static inline qe6502_tick_t write_unstable_store(qe6502_t* cpu, uint8_t value)
{
    const uint8_t base_high = u16_get_byte(cpu->latch_addr, 1);
    const uint8_t page_cross = cpu->latch_data;
    const uint8_t write_high = (page_cross == 0u) ? base_high : value;
    const uint16_t address = u16_set_byte(cpu->latch_addr, 1, write_high);

    return write(cpu, address, value);
}

/* common_helper; role=unstable_store; action=compute_base_high_plus_one_mask */
static inline uint8_t unstable_store_mask(qe6502_t* cpu)
{
    const uint8_t base_high = u16_get_byte(cpu->latch_addr, 1);
    return (uint8_t)(base_high + 1u);
}

/* Prefetch microcode variants. */
/* prefetch_handler; prefetc; condition=always; base=mc_cmos_jmp_ind_c4_target_hi_retry */
static qe6502_tick_t mc_cmos_jmp_ind_c4_target_hi_retry_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    if (u16_get_byte(cpu->latch_addr, 0) == 0u)
    {
        set_latch_addr1(cpu, (uint8_t)(u16_get_byte(cpu->latch_addr, 1) + 1u));
    }
    return read(cpu, cpu->latch_addr);
}

/* prefetch_handler; prefetc; condition=no_page_cross_decimal_clear; base=mc_cmos_r_indexed_probe */
static qe6502_tick_t mc_cmos_r_indexed_probe_prefetch_no_page_cross_decimal_clear(qe6502_t* cpu, uint8_t bus)
{
    if ((cpu->latch_data == 0u) && ((cpu->P & flag_D) == 0u))
    {
        prefetch(cpu);
    }
    return mc_cmos_r_indexed_probe(cpu, bus);
}

/* prefetch_handler; prefetc; condition=no_page_cross; base=mc_cmos_r_indexed_probe_no_latch */
static qe6502_tick_t mc_cmos_r_indexed_probe_no_latch_prefetch_no_page_cross(qe6502_t* cpu, uint8_t bus)
{
    if (cpu->latch_data == 0u)
    {
        prefetch(cpu);
    }
    const uint8_t page_cross = cpu->latch_data;
    const uint8_t high = (uint8_t)(bus + page_cross);

    if (page_cross == 0u)
    {
        const uint16_t address = u16_set_byte(cpu->latch_addr, 1, high);

        cpu->PC++;
        cpu->microcode++;
        return read(cpu, address);
    }

    set_latch_addr1(cpu, high);
    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* prefetch_handler; prefetc; condition=always; base=mc_latch_addr0_read_pc_inc */
static qe6502_tick_t mc_latch_addr0_read_pc_inc_prefetch(qe6502_t* cpu, uint8_t bus)
{
    prefetch(cpu);
    return mc_latch_addr0_read_pc_inc(cpu, bus);
}

/* prefetch_handler; prefetc; condition=decimal_clear; base=mc_latch_addr1_read_latch_addr */
static qe6502_tick_t mc_latch_addr1_read_latch_addr_prefetch_decimal_clear(qe6502_t* cpu, uint8_t bus)
{
    if ((cpu->P & flag_D) == 0u)
    {
        prefetch(cpu);
    }
    return mc_latch_addr1_read_latch_addr(cpu, bus);
}

/* prefetch_handler; prefetc; condition=always; base=mc_latch_data_read_latch_addr */
static qe6502_tick_t mc_latch_data_read_latch_addr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    prefetch(cpu);
    return mc_latch_data_read_latch_addr(cpu, bus);
}

/* prefetch_handler; prefetc; condition=always; base=mc_latch_pcl_stack_read */
static qe6502_tick_t mc_latch_pcl_stack_read_prefetch(qe6502_t* cpu, uint8_t bus)
{
    prefetch(cpu);
    return mc_latch_pcl_stack_read(cpu, bus);
}

/* prefetch_handler; prefetc; condition=decimal_clear; base=mc_latch_zp_addr_read_latch_addr */
static qe6502_tick_t mc_latch_zp_addr_read_latch_addr_prefetch_decimal_clear(qe6502_t* cpu, uint8_t bus)
{
    if ((cpu->P & flag_D) == 0u)
    {
        prefetch(cpu);
    }

    return mc_latch_zp_addr_read_latch_addr(cpu, bus);
}

/* prefetch_handler; prefetc; condition=no_page_cross; base=mc_r_indexed_probe_no_latch */
static qe6502_tick_t mc_r_indexed_probe_no_latch_prefetch_no_page_cross(qe6502_t* cpu, uint8_t bus)
{
    if (cpu->latch_data == 0u)
    {
        prefetch(cpu);
    }
    const uint8_t page_cross = cpu->latch_data;
    const uint16_t addr = u16_set_byte(cpu->latch_addr, 1, bus);

    qe6502_tick_t tick = read(cpu, addr);

    if (page_cross == 0u)
    {
        cpu->microcode++;
    }
    else
    {
        set_latch_addr1(cpu, (uint8_t)(bus + page_cross));
    }

    return tick;
}

/* prefetch_handler; prefetc; condition=decimal_clear; base=mc_r_izx_c4_data */
static qe6502_tick_t mc_r_izx_c4_data_prefetch_decimal_clear(qe6502_t* cpu, uint8_t bus)
{
    if ((cpu->P & flag_D) == 0u)
    {
        prefetch(cpu);
    }

    return mc_r_izx_c4_data(cpu, bus);
}

/* prefetch_handler; prefetc; condition=always; base=mc_r_izx_c4_data_no_latch */
static qe6502_tick_t mc_r_izx_c4_data_no_latch_prefetch(qe6502_t* cpu, uint8_t bus)
{
    prefetch(cpu);
    const uint16_t address = u16_set_byte(cpu->latch_data, 1, bus);
    return read(cpu, address);
}

/* prefetch_handler; prefetc; condition=always; base=mc_r_zp_c1_data */
static qe6502_tick_t mc_r_zp_c1_data_prefetch(qe6502_t* cpu, uint8_t bus)
{
    prefetch(cpu);
    return read(cpu, bus);
}

/* prefetch_handler; prefetc; condition=always; base=mc_read_addr_with_bus_hi */
static qe6502_tick_t mc_read_addr_with_bus_hi_prefetch(qe6502_t* cpu, uint8_t bus)
{
    prefetch(cpu);
    const uint16_t address = u16_set_byte(cpu->latch_addr, 1, bus);
    return read(cpu, address);
}

/* prefetch_handler; prefetc; condition=always; */
static qe6502_tick_t mc_read_latch_addr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);

    return read(cpu, cpu->latch_addr);
}

/* prefetch_handler; prefetc; condition=decimal_clear; */
static qe6502_tick_t mc_read_latch_addr_prefetch_decimal_clear(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    if ((cpu->P & flag_D) == 0u)
    {
        prefetch(cpu);
    }

    return read(cpu, cpu->latch_addr);
}

/* prefetch_handler; prefetc; condition=always; base=mc_read_pc */
static qe6502_tick_t mc_read_pc_prefetch(qe6502_t* cpu, uint8_t bus)
{
    prefetch(cpu);

    return mc_read_pc(cpu, bus);
}

/* prefetch_handler; prefetc; condition=always; base=mc_read_pc_inc */
static qe6502_tick_t mc_read_pc_inc_prefetch(qe6502_t* cpu, uint8_t bus)
{
    prefetch(cpu);

    return mc_read_pc_inc(cpu, bus);
}

/* prefetch_handler; prefetc; condition=decimal_clear; base=mc_read_pc_inc */
static qe6502_tick_t mc_read_pc_inc_prefetch_decimal_clear(qe6502_t* cpu, uint8_t bus)
{
    if ((cpu->P & flag_D) == 0u)
    {
        prefetch(cpu);
    }

    return mc_read_pc_inc(cpu, bus);
}

/* prefetch_handler; prefetc; condition=always; base=mc_read_pc_minus_one */
static qe6502_tick_t mc_read_pc_minus_one_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    return read(cpu, (uint16_t)(cpu->PC - 1u));
}

/* prefetch_handler; prefetc; condition=always; base=mc_rts_c4_inc_pc_dummy */
static qe6502_tick_t mc_rts_c4_inc_pc_dummy_prefetch(qe6502_t* cpu, uint8_t bus)
{
    prefetch(cpu);
    cpu->PC = u16_set_byte(cpu->PC, 1, bus);
    return read_pc_inc(cpu);
}

/* prefetch_handler; prefetc; condition=always; base=mc_stack_pull_read */
static qe6502_tick_t mc_stack_pull_read_prefetch(qe6502_t* cpu, uint8_t bus)
{
    prefetch(cpu);
    return mc_stack_pull_read(cpu, bus);
}

/* prefetch_handler; prefetc; condition=always; base=mc_wdc_bbr_bbs_c3_offset_or_skip */
static qe6502_tick_t mc_wdc_bbr_bbs_c3_offset_or_skip_prefetch(qe6502_t* cpu, uint8_t bus)
{
    prefetch(cpu);
    const uint8_t opcode = current_opcode_slot(cpu);
    uint8_t bit = (uint8_t)(opcode >> 4u);
    bool taken;

    if (bit < 8u)
    {
        const uint8_t mask = (uint8_t)(1u << bit);
        taken = (bus & mask) == 0u;
    }
    else
    {
        bit = (uint8_t)(bit - 8u);
        const uint8_t mask = (uint8_t)(1u << bit);
        taken = (bus & mask) != 0u;
    }

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;

    if (!taken)
    {
        cpu->microcode = (uint16_t)(cpu->microcode + 2u);
    }

    return tick;
}

/* prefetch_handler; prefetc; condition=always; base=op_ahx_unstable_w_ready_addr_pending_none_wr */
static qe6502_tick_t op_ahx_unstable_w_ready_addr_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    const uint8_t value = (uint8_t)(cpu->A & cpu->X & unstable_store_mask(cpu));
    return write_unstable_store(cpu, value);
}

/* prefetch_handler; prefetc; condition=always; base=op_asl_acc_ready_none_pending_none_dummy */
static qe6502_tick_t op_asl_acc_ready_none_pending_none_dummy_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    cpu->A = asl_value(cpu, cpu->A);
    return read(cpu, cpu->PC);
}

/* prefetch_handler; prefetc; condition=always; base=op_asl_rw_ready_addr_data_pending_none_wr */
static qe6502_tick_t op_asl_rw_ready_addr_data_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    const uint8_t value = asl_value(cpu, cpu->latch_data);
    return write(cpu, cpu->latch_addr, value);
}

/* prefetch_handler; prefetc; condition=always; base=op_bcc_branch_c0_offset */
static qe6502_tick_t op_bcc_branch_c0_offset_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return branch_c0_offset(cpu, (cpu->P & flag_C) == 0u);
}

/* prefetch_handler; prefetc; condition=always; base=op_bcs_branch_c0_offset */
static qe6502_tick_t op_bcs_branch_c0_offset_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return branch_c0_offset(cpu, (cpu->P & flag_C) != 0u);
}

/* prefetch_handler; prefetc; condition=always; base=op_beq_branch_c0_offset */
static qe6502_tick_t op_beq_branch_c0_offset_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return branch_c0_offset(cpu, (cpu->P & flag_Z) != 0u);
}

/* prefetch_handler; prefetc; condition=always; base=op_bmi_branch_c0_offset */
static qe6502_tick_t op_bmi_branch_c0_offset_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return branch_c0_offset(cpu, (cpu->P & flag_N) != 0u);
}

/* prefetch_handler; prefetc; condition=always; base=op_bne_branch_c0_offset */
static qe6502_tick_t op_bne_branch_c0_offset_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return branch_c0_offset(cpu, (cpu->P & flag_Z) == 0u);
}

/* prefetch_handler; prefetc; condition=always; base=op_bpl_branch_c0_offset */
static qe6502_tick_t op_bpl_branch_c0_offset_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return branch_c0_offset(cpu, (cpu->P & flag_N) == 0u);
}

/* prefetch_handler; prefetc; condition=always; base=op_bvc_branch_c0_offset */
static qe6502_tick_t op_bvc_branch_c0_offset_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return branch_c0_offset(cpu, (cpu->P & flag_V) == 0u);
}

/* prefetch_handler; prefetc; condition=always; base=op_bvs_branch_c0_offset */
static qe6502_tick_t op_bvs_branch_c0_offset_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return branch_c0_offset(cpu, (cpu->P & flag_V) != 0u);
}

/* prefetch_handler; prefetc; condition=always; base=op_clc_imp_ready_none_pending_none_dummy */
static qe6502_tick_t op_clc_imp_ready_none_pending_none_dummy_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    cpu->P = (uint8_t)(cpu->P & (uint8_t)(~flag_C));
    return read(cpu, cpu->PC);
}

/* prefetch_handler; prefetc; condition=always; base=op_cld_imp_ready_none_pending_none_dummy */
static qe6502_tick_t op_cld_imp_ready_none_pending_none_dummy_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    cpu->P = (uint8_t)(cpu->P & (uint8_t)(~flag_D));
    return read(cpu, cpu->PC);
}

/* prefetch_handler; prefetc; condition=always; base=op_cli_imp_ready_none_pending_none_dummy */
static qe6502_tick_t op_cli_imp_ready_none_pending_none_dummy_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    cpu->P = (uint8_t)(cpu->P & (uint8_t)(~flag_I));
    return read(cpu, cpu->PC);
}

/* prefetch_handler; prefetc; condition=always; base=op_clv_imp_ready_none_pending_none_dummy */
static qe6502_tick_t op_clv_imp_ready_none_pending_none_dummy_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    cpu->P = (uint8_t)(cpu->P & (uint8_t)(~flag_V));
    return read(cpu, cpu->PC);
}

/* prefetch_handler; prefetc; condition=decimal_set; base=op_cmos_adc_r_ready_addr_pending_data_fetch_or_decimal_dummy */
static qe6502_tick_t op_cmos_adc_r_ready_addr_pending_data_fetch_or_decimal_dummy_prefetch_decimal_set(qe6502_t* cpu, uint8_t bus)
{
    if ((cpu->P & flag_D) != 0u)
    {
        prefetch(cpu);
    }

    if ((cpu->P & flag_D) == 0u)
    {
        adc_binary(cpu, bus);
        cpu->microcode++;
        return fetch(cpu);
    }

    cpu->latch_data = bus;
    return read(cpu, cpu->latch_addr);
}

/* prefetch_handler; prefetc; condition=always; base=op_cmos_bra_branch_c0_offset */
static qe6502_tick_t op_cmos_bra_branch_c0_offset_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    return read_pc_inc(cpu);
}

/* prefetch_handler; prefetc; condition=always; base=op_cmos_dec_acc_ready_none_pending_none_dummy */
static qe6502_tick_t op_cmos_dec_acc_ready_none_pending_none_dummy_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    cpu->A--;
    update_flags_nz(cpu, cpu->A);
    return read(cpu, cpu->PC);
}

/* prefetch_handler; prefetc; condition=always; base=op_cmos_inc_acc_ready_none_pending_none_dummy */
static qe6502_tick_t op_cmos_inc_acc_ready_none_pending_none_dummy_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    cpu->A++;
    update_flags_nz(cpu, cpu->A);
    return read(cpu, cpu->PC);
}

/* prefetch_handler; prefetc; condition=always; base=op_cmos_phx_stack_push_ready_none_pending_none_wr */
static qe6502_tick_t op_cmos_phx_stack_push_ready_none_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    return stack_write(cpu, cpu->X);
}

/* prefetch_handler; prefetc; condition=always; base=op_cmos_phy_stack_push_ready_none_pending_none_wr */
static qe6502_tick_t op_cmos_phy_stack_push_ready_none_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    return stack_write(cpu, cpu->Y);
}

/* prefetch_handler; prefetc; condition=decimal_set; base=op_cmos_sbc_imm_ready_none_pending_data_fetch_or_decimal_dummy */
static qe6502_tick_t op_cmos_sbc_imm_ready_none_pending_data_fetch_or_decimal_dummy_prefetch_decimal_set(qe6502_t* cpu, uint8_t bus)
{
    if ((cpu->P & flag_D) != 0u)
    {
        prefetch(cpu);
    }

    if ((cpu->P & flag_D) == 0u)
    {
        adc_binary(cpu, (uint8_t)(bus ^ 0xffu));
        cpu->microcode++;
        return fetch(cpu);
    }

    cpu->latch_data = bus;
    return read(cpu, 0x0000u);
}

/* prefetch_handler; prefetc; condition=decimal_set; base=op_cmos_sbc_r_ready_addr_pending_data_fetch_or_decimal_dummy */
static qe6502_tick_t op_cmos_sbc_r_ready_addr_pending_data_fetch_or_decimal_dummy_prefetch_decimal_set(qe6502_t* cpu, uint8_t bus)
{
    if ((cpu->P & flag_D) != 0u)
    {
        prefetch(cpu);
    }

    if ((cpu->P & flag_D) == 0u)
    {
        adc_binary(cpu, (uint8_t)(bus ^ 0xffu));
        cpu->microcode++;
        return fetch(cpu);
    }

    cpu->latch_data = bus;
    return read(cpu, cpu->latch_addr);
}

/* prefetch_handler; prefetc; condition=always; base=op_cmos_stz_w_ready_addr_pending_none_wr */
static qe6502_tick_t op_cmos_stz_w_ready_addr_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    return write(cpu, cpu->latch_addr, 0u);
}

/* prefetch_handler; prefetc; condition=always; base=op_cmos_stz_w_ready_addrlo_pending_addrhi_wr */
static qe6502_tick_t op_cmos_stz_w_ready_addrlo_pending_addrhi_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    prefetch(cpu);
    const uint16_t address = u16_set_byte(cpu->latch_addr, 1, bus);
    return write(cpu, address, 0u);
}

/* prefetch_handler; prefetc; condition=always; base=op_cmos_stz_w_ready_none_pending_addrlo_wr */
static qe6502_tick_t op_cmos_stz_w_ready_none_pending_addrlo_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    prefetch(cpu);
    return write(cpu, (uint16_t)bus, 0u);
}

/* prefetch_handler; prefetc; condition=always; base=op_cmos_trb_rw_ready_addr_data_pending_none_wr */
static qe6502_tick_t op_cmos_trb_rw_ready_addr_data_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    const uint8_t flags = flag_Z_if((cpu->A & cpu->latch_data) == 0u);
    const uint8_t value = (uint8_t)(cpu->latch_data & (uint8_t)(~cpu->A));

    cpu->P = (uint8_t)((cpu->P & (uint8_t)(~flag_Z)) | flags);
    return write(cpu, cpu->latch_addr, value);
}

/* prefetch_handler; prefetc; condition=always; base=op_cmos_tsb_rw_ready_addr_data_pending_none_wr */
static qe6502_tick_t op_cmos_tsb_rw_ready_addr_data_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    const uint8_t flags = flag_Z_if((cpu->A & cpu->latch_data) == 0u);
    const uint8_t value = (uint8_t)(cpu->latch_data | cpu->A);
    cpu->P = (uint8_t)((cpu->P & (uint8_t)(~flag_Z)) | flags);
    return write(cpu, cpu->latch_addr, value);
}

/* prefetch_handler; prefetc; condition=always; base=op_dcp_rw_ready_addr_data_pending_none_wr */
static qe6502_tick_t op_dcp_rw_ready_addr_data_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    const uint8_t value = (uint8_t)(cpu->latch_data - 1u);
    compare_register_with_value(cpu, cpu->A, value);
    return write(cpu, cpu->latch_addr, value);
}

/* prefetch_handler; prefetc; condition=always; base=op_dec_rw_ready_addr_data_pending_none_wr */
static qe6502_tick_t op_dec_rw_ready_addr_data_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    const uint8_t value = (uint8_t)(cpu->latch_data - 1u);
    update_flags_nz(cpu, value);
    return write(cpu, cpu->latch_addr, value);
}

/* prefetch_handler; prefetc; condition=always; base=op_dex_imp_ready_none_pending_none_dummy */
static qe6502_tick_t op_dex_imp_ready_none_pending_none_dummy_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    cpu->X--;
    update_flags_nz(cpu, cpu->X);
    return read(cpu, cpu->PC);
}

/* prefetch_handler; prefetc; condition=always; base=op_dey_imp_ready_none_pending_none_dummy */
static qe6502_tick_t op_dey_imp_ready_none_pending_none_dummy_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    cpu->Y--;
    update_flags_nz(cpu, cpu->Y);
    return read(cpu, cpu->PC);
}

/* prefetch_handler; prefetc; condition=always; base=op_inc_rw_ready_addr_data_pending_none_wr */
static qe6502_tick_t op_inc_rw_ready_addr_data_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    const uint8_t value = (uint8_t)(cpu->latch_data + 1u);
    update_flags_nz(cpu, value);
    return write(cpu, cpu->latch_addr, value);
}

/* prefetch_handler; prefetc; condition=always; base=op_inx_imp_ready_none_pending_none_dummy */
static qe6502_tick_t op_inx_imp_ready_none_pending_none_dummy_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    cpu->X++;
    update_flags_nz(cpu, cpu->X);
    return read(cpu, cpu->PC);
}

/* prefetch_handler; prefetc; condition=always; base=op_iny_imp_ready_none_pending_none_dummy */
static qe6502_tick_t op_iny_imp_ready_none_pending_none_dummy_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    cpu->Y++;
    update_flags_nz(cpu, cpu->Y);
    return read(cpu, cpu->PC);
}

/* prefetch_handler; prefetc; condition=always; base=op_isc_rw_ready_addr_data_pending_none_wr */
static qe6502_tick_t op_isc_rw_ready_addr_data_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    const uint8_t value = (uint8_t)(cpu->latch_data + 1u);
    sbc_value(cpu, value);
    return write(cpu, cpu->latch_addr, value);
}

/* prefetch_handler; prefetc; condition=always; base=op_lsr_acc_ready_none_pending_none_dummy */
static qe6502_tick_t op_lsr_acc_ready_none_pending_none_dummy_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    cpu->A = lsr_value(cpu, cpu->A);
    return read(cpu, cpu->PC);
}

/* prefetch_handler; prefetc; condition=always; base=op_lsr_rw_ready_addr_data_pending_none_wr */
static qe6502_tick_t op_lsr_rw_ready_addr_data_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    const uint8_t value = lsr_value(cpu, cpu->latch_data);
    return write(cpu, cpu->latch_addr, value);
}

/* prefetch_handler; prefetc; condition=always; base=op_nes_isc_rw_ready_addr_data_pending_none_wr */
static qe6502_tick_t mc_nes_isc_rw_ready_addr_data_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    const uint8_t value = (uint8_t)(cpu->latch_data + 1u);
    adc_binary(cpu, (uint8_t)(value ^ 0xffu));
    return write(cpu, cpu->latch_addr, value);
}

/* prefetch_handler; prefetc; condition=always; base=op_nes_rra_rw_ready_addr_data_pending_none_wr */
static qe6502_tick_t op_nes_rra_rw_ready_addr_data_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    const uint8_t value = ror_value(cpu, cpu->latch_data);
    adc_binary(cpu, value);
    return write(cpu, cpu->latch_addr, value);
}

/* prefetch_handler; prefetc; condition=always; base=op_pha_stack_push_ready_none_pending_none_wr */
static qe6502_tick_t op_pha_stack_push_ready_none_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    return stack_write(cpu, cpu->A);
}

/* prefetch_handler; prefetc; condition=always; */
static qe6502_tick_t op_php_stack_push_ready_none_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    return stack_write(cpu, stack_status(cpu->P, flag_B));
}

/* prefetch_handler; prefetc; condition=always; base=op_rla_rw_ready_addr_data_pending_none_wr */
static qe6502_tick_t op_rla_rw_ready_addr_data_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    const uint8_t value = rol_value(cpu, cpu->latch_data);
    cpu->A = (uint8_t)(cpu->A & value);
    update_flags_nz(cpu, cpu->A);
    return write(cpu, cpu->latch_addr, value);
}

/* prefetch_handler; prefetc; condition=always; base=op_rol_acc_ready_none_pending_none_dummy */
static qe6502_tick_t op_rol_acc_ready_none_pending_none_dummy_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    cpu->A = rol_value(cpu, cpu->A);
    return read(cpu, cpu->PC);
}

/* prefetch_handler; prefetc; condition=always; base=op_rol_rw_ready_addr_data_pending_none_wr */
static qe6502_tick_t op_rol_rw_ready_addr_data_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    prefetch(cpu);
    (void)bus;

    const uint8_t value = rol_value(cpu, cpu->latch_data);
    return write(cpu, cpu->latch_addr, value);
}

/* prefetch_handler; prefetc; condition=always; base=op_ror_acc_ready_none_pending_none_dummy */
static qe6502_tick_t op_ror_acc_ready_none_pending_none_dummy_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    cpu->A = ror_value(cpu, cpu->A);
    return read(cpu, cpu->PC);
}

/* prefetch_handler; prefetc; condition=always; base=op_ror_rw_ready_addr_data_pending_none_wr */
static qe6502_tick_t op_ror_rw_ready_addr_data_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    const uint8_t value = ror_value(cpu, cpu->latch_data);
    return write(cpu, cpu->latch_addr, value);
}

/* prefetch_handler; prefetc; condition=always; base=op_rra_rw_ready_addr_data_pending_none_wr */
static qe6502_tick_t op_rra_rw_ready_addr_data_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    const uint8_t value = ror_value(cpu, cpu->latch_data);
    adc_value(cpu, value);
    return write(cpu, cpu->latch_addr, value);
}

/* prefetch_handler; prefetc; condition=decimal_set; base=op_rw_adc_imm_ready_none_pending_data_fetch_or_decimal_dummy */
static qe6502_tick_t op_rw_adc_imm_ready_none_pending_data_fetch_or_decimal_dummy_prefetch_decimal_set(qe6502_t* cpu, uint8_t bus)
{
    if ((cpu->P & flag_D) != 0u)
    {
        prefetch(cpu);
    }

    if ((cpu->P & flag_D) == 0u)
    {
        adc_binary(cpu, bus);
        cpu->microcode++;
        return fetch(cpu);
    }

    cpu->latch_data = bus;
    return read(cpu, 0x0059u);
}

/* prefetch_handler; prefetc; condition=always; base=op_sax_w_ready_addr_pending_none_wr */
static qe6502_tick_t op_sax_w_ready_addr_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    const uint8_t value = (uint8_t)(cpu->A & cpu->X);
    return write(cpu, cpu->latch_addr, value);
}

/* prefetch_handler; prefetc; condition=always; base=op_sax_w_ready_addrlo_pending_addrhi_wr */
static qe6502_tick_t op_sax_w_ready_addrlo_pending_addrhi_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    prefetch(cpu);
    const uint8_t value = (uint8_t)(cpu->A & cpu->X);
    const uint16_t address = u16_set_byte(cpu->latch_addr, 1, bus);
    return write(cpu, address, value);
}

/* prefetch_handler; prefetc; condition=always; base=op_sax_w_ready_none_pending_addrlo_wr */
static qe6502_tick_t op_sax_w_ready_none_pending_addrlo_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    prefetch(cpu);
    const uint8_t value = (uint8_t)(cpu->A & cpu->X);
    return write(cpu, (uint16_t)bus, value);
}

/* prefetch_handler; prefetc; condition=always; base=op_sec_imp_ready_none_pending_none_dummy */
static qe6502_tick_t op_sec_imp_ready_none_pending_none_dummy_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    cpu->P = (uint8_t)(cpu->P | flag_C);
    return read(cpu, cpu->PC);
}

/* prefetch_handler; prefetc; condition=always; base=op_sed_imp_ready_none_pending_none_dummy */
static qe6502_tick_t op_sed_imp_ready_none_pending_none_dummy_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    cpu->P = (uint8_t)(cpu->P | flag_D);
    return read(cpu, cpu->PC);
}

/* prefetch_handler; prefetc; condition=always; base=op_sei_imp_ready_none_pending_none_dummy */
static qe6502_tick_t op_sei_imp_ready_none_pending_none_dummy_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    cpu->P = (uint8_t)(cpu->P | flag_I);
    return read(cpu, cpu->PC);
}

/* prefetch_handler; prefetc; condition=always; base=op_shx_unstable_w_ready_addr_pending_none_wr */
static qe6502_tick_t op_shx_unstable_w_ready_addr_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    const uint8_t value = (uint8_t)(cpu->X & unstable_store_mask(cpu));
    return write_unstable_store(cpu, value);
}

/* prefetch_handler; prefetc; condition=always; base=op_shy_unstable_w_ready_addr_pending_none_wr */
static qe6502_tick_t op_shy_unstable_w_ready_addr_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    const uint8_t value = (uint8_t)(cpu->Y & unstable_store_mask(cpu));
    return write_unstable_store(cpu, value);
}

/* prefetch_handler; prefetc; condition=always; base=op_slo_rw_ready_addr_data_pending_none_wr */
static qe6502_tick_t op_slo_rw_ready_addr_data_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    const uint8_t value = asl_value(cpu, cpu->latch_data);
    cpu->A = (uint8_t)(cpu->A | value);
    update_flags_nz(cpu, cpu->A);
    return write(cpu, cpu->latch_addr, value);
}

/* prefetch_handler; prefetc; condition=always; base=op_sre_rw_ready_addr_data_pending_none_wr */
static qe6502_tick_t op_sre_rw_ready_addr_data_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    const uint8_t value = lsr_value(cpu, cpu->latch_data);
    cpu->A = (uint8_t)(cpu->A ^ value);
    update_flags_nz(cpu, cpu->A);
    return write(cpu, cpu->latch_addr, value);
}

/* prefetch_handler; prefetc; condition=decimal_set; base=op_st_adc_imm_ready_none_pending_data_fetch_or_decimal_dummy */
static qe6502_tick_t op_st_adc_imm_ready_none_pending_data_fetch_or_decimal_dummy_prefetch_decimal_set(qe6502_t* cpu, uint8_t bus)
{
    if ((cpu->P & flag_D) != 0u)
    {
        prefetch(cpu);
    }

    if ((cpu->P & flag_D) == 0u)
    {
        adc_binary(cpu, bus);
        cpu->microcode++;
        return fetch(cpu);
    }

    cpu->latch_data = bus;
    return read(cpu, 0x0056u);
}

/* prefetch_handler; prefetc; condition=always; base=op_sta_w_ready_addr_pending_none_wr */
static qe6502_tick_t op_sta_w_ready_addr_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    return write(cpu, cpu->latch_addr, cpu->A);
}

/* prefetch_handler; prefetc; condition=always; base=op_sta_w_ready_addrlo_pending_addrhi_wr */
static qe6502_tick_t op_sta_w_ready_addrlo_pending_addrhi_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    prefetch(cpu);
    const uint16_t address = u16_set_byte(cpu->latch_addr, 1, bus);
    return write(cpu, address, cpu->A);
}

/* prefetch_handler; prefetc; condition=always; base=op_sta_w_ready_none_pending_addrlo_wr */
static qe6502_tick_t op_sta_w_ready_none_pending_addrlo_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    prefetch(cpu);
    return write(cpu, (uint16_t)bus, cpu->A);
}

/* prefetch_handler; prefetc; condition=always; base=op_stx_w_ready_addr_pending_none_wr */
static qe6502_tick_t op_stx_w_ready_addr_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    return write(cpu, cpu->latch_addr, cpu->X);
}

/* prefetch_handler; prefetc; condition=always; base=op_stx_w_ready_addrlo_pending_addrhi_wr */
static qe6502_tick_t op_stx_w_ready_addrlo_pending_addrhi_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    prefetch(cpu);
    const uint16_t address = u16_set_byte(cpu->latch_addr, 1, bus);
    return write(cpu, address, cpu->X);
}

/* prefetch_handler; prefetc; condition=always; base=op_stx_w_ready_none_pending_addrlo_wr */
static qe6502_tick_t op_stx_w_ready_none_pending_addrlo_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    prefetch(cpu);

    return write(cpu, (uint16_t)bus, cpu->X);
}

/* prefetch_handler; prefetc; condition=always; base=op_sty_w_ready_addr_pending_none_wr */
static qe6502_tick_t op_sty_w_ready_addr_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    return write(cpu, cpu->latch_addr, cpu->Y);
}

/* prefetch_handler; prefetc; condition=always; base=op_sty_w_ready_addrlo_pending_addrhi_wr */
static qe6502_tick_t op_sty_w_ready_addrlo_pending_addrhi_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    prefetch(cpu);
    const uint16_t address = u16_set_byte(cpu->latch_addr, 1, bus);
    return write(cpu, address, cpu->Y);
}

/* prefetch_handler; prefetc; condition=always; base=op_sty_w_ready_none_pending_addrlo_wr */
static qe6502_tick_t op_sty_w_ready_none_pending_addrlo_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    prefetch(cpu);
    return write(cpu, (uint16_t)bus, cpu->Y);
}

/* prefetch_handler; prefetc; condition=always; base=op_tas_unstable_w_ready_addr_pending_none_wr */
static qe6502_tick_t op_tas_unstable_w_ready_addr_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    const uint8_t stack = (uint8_t)(cpu->A & cpu->X);
    const uint8_t value = (uint8_t)(stack & unstable_store_mask(cpu));
    cpu->S = stack;
    return write_unstable_store(cpu, value);
}

/* prefetch_handler; prefetc; condition=always; base=op_tax_imp_ready_none_pending_none_dummy */
static qe6502_tick_t op_tax_imp_ready_none_pending_none_dummy_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    cpu->X = cpu->A;
    update_flags_nz(cpu, cpu->X);
    return read(cpu, cpu->PC);
}

/* prefetch_handler; prefetc; condition=always; base=op_tay_imp_ready_none_pending_none_dummy */
static qe6502_tick_t op_tay_imp_ready_none_pending_none_dummy_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    cpu->Y = cpu->A;
    update_flags_nz(cpu, cpu->Y);
    return read(cpu, cpu->PC);
}

/* prefetch_handler; prefetc; condition=always; base=op_tsx_imp_ready_none_pending_none_dummy */
static qe6502_tick_t op_tsx_imp_ready_none_pending_none_dummy_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    cpu->X = cpu->S;
    update_flags_nz(cpu, cpu->X);
    return read(cpu, cpu->PC);
}

/* prefetch_handler; prefetc; condition=always; base=op_txa_imp_ready_none_pending_none_dummy */
static qe6502_tick_t op_txa_imp_ready_none_pending_none_dummy_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    cpu->A = cpu->X;
    update_flags_nz(cpu, cpu->A);
    return read(cpu, cpu->PC);
}

/* prefetch_handler; prefetc; condition=always; base=op_txs_imp_ready_none_pending_none_dummy */
static qe6502_tick_t op_txs_imp_ready_none_pending_none_dummy_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    cpu->S = cpu->X;
    return read(cpu, cpu->PC);
}

/* prefetch_handler; prefetc; condition=always; base=op_tya_imp_ready_none_pending_none_dummy */
static qe6502_tick_t op_tya_imp_ready_none_pending_none_dummy_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    cpu->A = cpu->Y;
    update_flags_nz(cpu, cpu->A);
    return read(cpu, cpu->PC);
}

/* prefetch_handler; prefetc; condition=decimal_set; base=op_wdc_adc_imm_ready_none_pending_data_fetch_or_decimal_dummy */
static qe6502_tick_t op_wdc_adc_imm_ready_none_pending_data_fetch_or_decimal_dummy_prefetch_decimal_set(qe6502_t* cpu, uint8_t bus)
{
    if ((cpu->P & flag_D) != 0u)
    {
        prefetch(cpu);
    }

    if ((cpu->P & flag_D) == 0u)
    {
        adc_binary(cpu, bus);
        cpu->microcode++;
        return fetch(cpu);
    }

    cpu->latch_data = bus;
    return read(cpu, 0x007fu);
}

/* prefetch_handler; prefetc; condition=always; base=op_wdc_rmb_smb_rw_ready_addr_data_pending_none_wr */
static qe6502_tick_t op_wdc_rmb_smb_rw_ready_addr_data_pending_none_wr_prefetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    prefetch(cpu);
    const uint8_t opcode = current_opcode_slot(cpu);
    uint8_t bit = (uint8_t)(opcode >> 4u);
    uint8_t value;

    if (bit < 8u)
    {
        const uint8_t mask = (uint8_t)(1u << bit);
        value = (uint8_t)(cpu->latch_data & (uint8_t)(~mask));
    }
    else
    {
        bit = (uint8_t)(bit - 8u);
        const uint8_t mask = (uint8_t)(1u << bit);
        value = (uint8_t)(cpu->latch_data | mask);
    }

    return write(cpu, cpu->latch_addr, value);
}

/* ABI/API helpers */

static inline qe6502abi_tick_t pack_tick(qe6502_tick_t tick)
{
    return ((uint32_t)tick.address << QE6502_ABI_TICK_ADDRESS_SHIFT) |
           ((uint32_t)tick.bus << QE6502_ABI_TICK_BUS_SHIFT) |
           ((uint32_t)tick.status << QE6502_ABI_TICK_STATUS_SHIFT);
}

static inline qe6502_tick_t unpack_tick(qe6502abi_tick_t tick)
{
    qe6502_tick_t result;
    result.address = QE6502_ABI_TICK_ADDRESS(tick);
    result.bus = QE6502_ABI_TICK_BUS(tick);
    result.status = QE6502_ABI_TICK_STATUS(tick);
    return result;
}

typedef struct qe6502abi_impl
{
    qe6502_t cpu;
    uint16_t magic;
    uint16_t version;
    uint8_t reserve[44];
} qe6502abi_impl_t;

QE6502_STATIC_ASSERT(sizeof(qe6502_t) == 16u,
                     "qe6502 CPU context must be 16 bytes");
QE6502_STATIC_ASSERT(sizeof(qe6502abi_impl_t) == QE6502_ABI_CONTEXT_SIZE,
                     "qe6502 ABI implementation context must be 64 bytes");
QE6502_STATIC_ASSERT(QE6502_ABI_SNAPSHOT_SIZE == QE6502_SNAPSHOT_SIZE,
                     "qe6502 Snapshot must be 64 bytes");

static inline qe6502abi_impl_t *qe6502abi_impl(qe6502abi_context_t *ctx)
{
    return (qe6502abi_impl_t *)(void *)ctx;
}

static inline const qe6502abi_impl_t *qe6502abi_const_impl(const qe6502abi_context_t *ctx)
{
    return (const qe6502abi_impl_t *)(const void *)ctx;
}

static inline void clear_buffer(uint8_t *bytes, uint32_t count)
{
    uint32_t i;
    for (i = 0u; i < count; ++i)
    {
        bytes[i] = 0u;
    }
}

static inline void qe6502abi_write_u16(uint8_t *dst, uint16_t value)
{
    dst[0] = (uint8_t)(((uint16_t)value >> 8u) & 0xffu);
    dst[1] = (uint8_t)((uint16_t)value & 0xffu);
}

static inline uint16_t qe6502abi_read_u16(const uint8_t *src)
{
    return (uint16_t)(((uint16_t)src[0] << 8u) | (uint16_t)src[1]);
}

static inline void qe6502abi_write_u32(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t)((value >> 24u) & 0xffu);
    dst[1] = (uint8_t)((value >> 16u) & 0xffu);
    dst[2] = (uint8_t)((value >> 8u) & 0xffu);
    dst[3] = (uint8_t)(value & 0xffu);
}

static inline uint32_t qe6502abi_read_u32(const uint8_t *src)
{
    return ((uint32_t)src[0] << 24u) |
           ((uint32_t)src[1] << 16u) |
           ((uint32_t)src[2] << 8u) |
           ((uint32_t)src[3]);
}

static inline void qe6502abi_clear_cpu(qe6502_t *cpu)
{
    cpu->model = 0u;
    cpu->reserved_extension = 0u;
    cpu->microcode = 0u;
    cpu->latch_addr = 0u;
    cpu->latch_data = 0u;
    cpu->hijack_microcode = 0u;
    cpu->PC = 0u;
    cpu->S = 0u;
    cpu->A = 0u;
    cpu->X = 0u;
    cpu->Y = 0u;
    cpu->P = 0u;
    cpu->interrupts = 0u;
}

static inline void qe6502abi_write_cpu(uint8_t *dst, const qe6502_t *cpu)
{
    dst[0] = cpu->model;
    dst[1] = cpu->reserved_extension;
    qe6502abi_write_u16(&dst[2], cpu->microcode);
    qe6502abi_write_u16(&dst[4], cpu->latch_addr);
    dst[6] = cpu->latch_data;
    dst[7] = cpu->hijack_microcode;
    qe6502abi_write_u16(&dst[8], cpu->PC);
    dst[10] = cpu->S;
    dst[11] = cpu->A;
    dst[12] = cpu->X;
    dst[13] = cpu->Y;
    dst[14] = cpu->P;
    dst[15] = cpu->interrupts;
}

static inline void qe6502abi_read_cpu(qe6502_t *cpu, const uint8_t *src)
{
    cpu->model = src[0];
    cpu->reserved_extension = src[1];
    cpu->microcode = qe6502abi_read_u16(&src[2]);
    cpu->latch_addr = qe6502abi_read_u16(&src[4]);
    cpu->latch_data = src[6];
    cpu->hijack_microcode = src[7];
    cpu->PC = qe6502abi_read_u16(&src[8]);
    cpu->S = src[10];
    cpu->A = src[11];
    cpu->X = src[12];
    cpu->Y = src[13];
    cpu->P = src[14];
    cpu->interrupts = src[15];
}

/* Public ABI. */

QE6502_ABI_API uint32_t qe6502abi_version(void)
{
    return QE6502_ABI_VERSION;
}

QE6502_ABI_API void qe6502abi_setup(qe6502abi_context_t *ctx, uint32_t model)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    qe6502abi_clear_cpu(&impl->cpu);
    impl->magic = (uint16_t)QE6502_ABI_CONTEXT_MAGIC;
    impl->version = (uint16_t)QE6502_ABI_CONTEXT_VERSION;
    clear_buffer(impl->reserve, (uint32_t)sizeof(impl->reserve));
    impl->cpu.model = (uint8_t)model;
}

QE6502_ABI_API qe6502abi_tick_t qe6502abi_restart(qe6502abi_context_t *ctx)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    return pack_tick(qe6502_restart(&impl->cpu));
}

QE6502_ABI_API qe6502abi_tick_t qe6502abi_tick(qe6502abi_context_t *ctx, uint32_t bus)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    return pack_tick(qe6502_tick(&impl->cpu, (uint8_t)bus));
}

QE6502_ABI_API qe6502abi_tick_t qe6502abi_goto(qe6502abi_context_t *ctx, uint32_t address)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    return pack_tick(qe6502_goto(&impl->cpu, (uint16_t)address));
}

QE6502_ABI_API void qe6502abi_nmi_assert(qe6502abi_context_t *ctx, uint8_t assert_nmi)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    qe6502_nmi_assert(&impl->cpu, assert_nmi);
}

QE6502_ABI_API void qe6502abi_irq_assert(qe6502abi_context_t *ctx, uint8_t assert_irq)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    qe6502_irq_assert(&impl->cpu, assert_irq);
}

QE6502_ABI_API uint8_t qe6502abi_is_nmi_asserted(const qe6502abi_context_t *ctx)
{
    const qe6502abi_impl_t *impl = qe6502abi_const_impl(ctx);
    return qe6502_is_nmi_asserted(&impl->cpu);
}

QE6502_ABI_API uint8_t qe6502abi_is_irq_asserted(const qe6502abi_context_t *ctx)
{
    const qe6502abi_impl_t *impl = qe6502abi_const_impl(ctx);
    return qe6502_is_irq_asserted(&impl->cpu);
}

QE6502_ABI_API void qe6502abi_save(const qe6502abi_context_t *ctx,
                                   qe6502abi_tick_t tick,
                                   uint8_t snapshot[QE6502_ABI_SNAPSHOT_SIZE])
{
    const qe6502abi_impl_t *impl = qe6502abi_const_impl(ctx);

    qe6502abi_write_u16(&snapshot[0], impl->magic);
    qe6502abi_write_u16(&snapshot[2], impl->version);
    qe6502abi_write_cpu(&snapshot[4], &impl->cpu);
    qe6502abi_write_u32(&snapshot[20], tick);
    clear_buffer(&snapshot[24], QE6502_ABI_SNAPSHOT_SIZE - 24u);
}

QE6502_ABI_API qe6502abi_tick_t qe6502abi_load(qe6502abi_context_t *ctx,
                                               const uint8_t snapshot[QE6502_ABI_SNAPSHOT_SIZE])
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);

    impl->magic = qe6502abi_read_u16(&snapshot[0]);
    impl->version = qe6502abi_read_u16(&snapshot[2]);
    qe6502abi_read_cpu(&impl->cpu, &snapshot[4]);
    const qe6502abi_tick_t tick = qe6502abi_read_u32(&snapshot[20]);
    clear_buffer(impl->reserve, (uint32_t)sizeof(impl->reserve));

    return tick;
}

QE6502_ABI_API uint32_t qe6502abi_get_pc(const qe6502abi_context_t *ctx)
{
    const qe6502abi_impl_t *impl = qe6502abi_const_impl(ctx);
    return impl->cpu.PC;
}

QE6502_ABI_API void qe6502abi_set_pc(qe6502abi_context_t *ctx, uint32_t value)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    impl->cpu.PC = (uint16_t)value;
}

QE6502_ABI_API uint32_t qe6502abi_get_s(const qe6502abi_context_t *ctx)
{
    const qe6502abi_impl_t *impl = qe6502abi_const_impl(ctx);
    return impl->cpu.S;
}

QE6502_ABI_API void qe6502abi_set_s(qe6502abi_context_t *ctx, uint32_t value)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    impl->cpu.S = (uint8_t)value;
}

QE6502_ABI_API uint32_t qe6502abi_get_a(const qe6502abi_context_t *ctx)
{
    const qe6502abi_impl_t *impl = qe6502abi_const_impl(ctx);
    return impl->cpu.A;
}

QE6502_ABI_API void qe6502abi_set_a(qe6502abi_context_t *ctx, uint32_t value)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    impl->cpu.A = (uint8_t)value;
}

QE6502_ABI_API uint32_t qe6502abi_get_x(const qe6502abi_context_t *ctx)
{
    const qe6502abi_impl_t *impl = qe6502abi_const_impl(ctx);
    return impl->cpu.X;
}

QE6502_ABI_API void qe6502abi_set_x(qe6502abi_context_t *ctx, uint32_t value)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    impl->cpu.X = (uint8_t)value;
}

QE6502_ABI_API uint32_t qe6502abi_get_y(const qe6502abi_context_t *ctx)
{
    const qe6502abi_impl_t *impl = qe6502abi_const_impl(ctx);
    return impl->cpu.Y;
}

QE6502_ABI_API void qe6502abi_set_y(qe6502abi_context_t *ctx, uint32_t value)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    impl->cpu.Y = (uint8_t)value;
}

QE6502_ABI_API uint32_t qe6502abi_get_p(const qe6502abi_context_t *ctx)
{
    const qe6502abi_impl_t *impl = qe6502abi_const_impl(ctx);
    return impl->cpu.P;
}

QE6502_ABI_API void qe6502abi_set_p(qe6502abi_context_t *ctx, uint32_t value)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    impl->cpu.P = (uint8_t)value;
}

QE6502_ABI_API uint32_t qe6502abi_get_model(const qe6502abi_context_t *ctx)
{
    const qe6502abi_impl_t *impl = qe6502abi_const_impl(ctx);
    return impl->cpu.model;
}

QE6502_ABI_API void qe6502abi_set_model(qe6502abi_context_t *ctx, uint32_t value)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    impl->cpu.model = (uint8_t)value;
}


/* Public API. */

qe6502_tick_t qe6502_goto(qe6502_t *cpu, uint16_t address)
{
    cpu->reserved_extension = 0;
    cpu->PC = address;
    enter_service_slot(cpu, service_slot_goto, 0);
    return qe6502_tick(cpu, 0u);
}

void qe6502_nmi_assert(qe6502_t *cpu, uint8_t assert_nmi)
{
    cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_sampling);
    if (assert_nmi != 0u)
    {
        if (flag(cpu->interrupts, qe6502_interrupt_nmi_inv_pin) == 0)
        {
            cpu->interrupts = flag_on(cpu->interrupts, qe6502_interrupt_nmi_inv_pin);
            cpu->hijack_microcode = 1;
        }
    }
    else if (flag(cpu->interrupts, qe6502_interrupt_nmi_inv_pin) != 0)
    {
        cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_nmi_inv_pin);
        cpu->hijack_microcode = 1;
    }
}

void qe6502_irq_assert(qe6502_t *cpu, uint8_t assert_irq)
{
    cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_sampling);
    if (assert_irq != 0u)
    {
        cpu->interrupts = flag_on(cpu->interrupts, qe6502_interrupt_irq_inv_pin);
        cpu->hijack_microcode = 1;
    }
    else
    {
        cpu->interrupts = flag_off(cpu->interrupts, qe6502_interrupt_irq_inv_pin);
    }
}

uint8_t qe6502_is_nmi_asserted(const qe6502_t *cpu)
{
    return flag(cpu->interrupts, qe6502_interrupt_nmi_inv_pin) != 0;
}

uint8_t qe6502_is_irq_asserted(const qe6502_t *cpu)
{
    return flag(cpu->interrupts, qe6502_interrupt_irq_inv_pin) != 0;
}

qe6502_tick_t qe6502_restart(qe6502_t *cpu)
{
    const uint8_t model = cpu->model;

    *cpu = (qe6502_t){0};
    cpu->model = model;
    cpu->PC = 0x01ff;
    cpu->S = 0xc0u;
    cpu->X = 0xc0u;
    cpu->P = qe6502_flag_Z;
    enter_service_slot(cpu, service_slot_internal_reset, 0);

    cpu->latch_data = 0;
    qe6502_tick_t tick = read(cpu, 0x00ff);
    tick.status = flag_on(tick.status, qe6502_status_internal_reset);
    return tick;
}

void qe6502_save(const qe6502_t *cpu,
                 qe6502_tick_t tick,
                 uint8_t snapshot[QE6502_SNAPSHOT_SIZE])
{
    qe6502abi_context_t abi_ctx;
    qe6502abi_impl_t *ctx = qe6502abi_impl(&abi_ctx);

    ctx->cpu = *cpu;
    ctx->magic = (uint16_t)QE6502_ABI_CONTEXT_MAGIC;
    ctx->version = (uint16_t)QE6502_ABI_CONTEXT_VERSION;
    clear_buffer(ctx->reserve, (uint32_t)sizeof(ctx->reserve));

    qe6502abi_save(&abi_ctx, pack_tick(tick), snapshot);
}

qe6502_tick_t qe6502_load(qe6502_t *ctx,
                          const uint8_t snapshot[QE6502_SNAPSHOT_SIZE])
{
    qe6502abi_context_t abi_ctx;
    const qe6502abi_tick_t tick = qe6502abi_load(&abi_ctx, snapshot);
    const qe6502abi_impl_t *snapshot_ctx = qe6502abi_const_impl(&abi_ctx);

    *ctx = snapshot_ctx->cpu;
    return unpack_tick(tick);
}

const qe6502_microcode_fn qe6502_control_store[qe6502_control_store_size] =
{
#include "control_store/nmos_block.inc"
,
#include "control_store/nes_block.inc"
,
#include "control_store/wdc_block.inc"
,
#include "control_store/rw_block.inc"
,
#include "control_store/st_block.inc"
};

#undef IDX
