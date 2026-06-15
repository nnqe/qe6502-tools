pub(crate) const SNAPSHOT_SIZE: usize = 64;

pub(crate) const MODEL_NMOS: u8 = 0;
pub(crate) const MODEL_NES: u8 = 1;
pub(crate) const MODEL_WDC: u8 = 2;
pub(crate) const MODEL_RW: u8 = 3;
pub(crate) const MODEL_ST: u8 = 4;


pub(crate) const STATUS_WRITING: u8 = 1 << 0;
pub(crate) const STATUS_OPCODE_FETCH: u8 = 1 << 1;
pub(crate) const STATUS_INTERNAL_RESET: u8 = 1 << 6;
pub(crate) const STATUS_CPU_JAMMED: u8 = 1 << 7;

#[repr(C)]
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub(crate) struct CpuContext {
    pub(crate) model: u8,
    pub(crate) reserved_extension: u8,
    pub(crate) microcode: u16,
    pub(crate) latch_addr: u16,
    pub(crate) latch_data: u8,
    pub(crate) service_mode: u8,
    pub(crate) pc: u16,
    pub(crate) s: u8,
    pub(crate) a: u8,
    pub(crate) x: u8,
    pub(crate) y: u8,
    pub(crate) p: u8,
    pub(crate) interrupts: u8,
}

const _: [(); 16] = [(); core::mem::size_of::<CpuContext>()];
const _: [(); 2] = [(); core::mem::align_of::<CpuContext>()];

impl Default for CpuContext {
    fn default() -> Self {
        Self {
            model: 0,
            reserved_extension: 0,
            microcode: 0,
            latch_addr: 0,
            latch_data: 0,
            service_mode: 0,
            pc: 0,
            s: 0,
            a: 0,
            x: 0,
            y: 0,
            p: 0,
            interrupts: 0,
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub(crate) struct Tick {
    pub(crate) address: u16,
    pub(crate) data: u8,
    pub(crate) flags: u8,
}

const _: [(); 4] = [(); core::mem::size_of::<Tick>()];
const _: [(); 2] = [(); core::mem::align_of::<Tick>()];

unsafe extern "C" {
    pub(crate) fn qe6502_setup(model: u32) -> CpuContext;
    pub(crate) fn qe6502_restart(cpu: *mut CpuContext) -> Tick;
    pub(crate) fn qe6502_goto(cpu: *mut CpuContext, address: u16) -> Tick;
    pub(crate) fn qe6502_tick_exported(cpu: *mut CpuContext, bus: u8) -> Tick;

    pub(crate) fn qe6502_nmi_assert(cpu: *mut CpuContext, assert_nmi: u8);
    pub(crate) fn qe6502_irq_assert(cpu: *mut CpuContext, assert_irq: u8);
    pub(crate) fn qe6502_is_nmi_asserted(cpu: *const CpuContext) -> u8;
    pub(crate) fn qe6502_is_irq_asserted(cpu: *const CpuContext) -> u8;

    pub(crate) fn qe6502_save(cpu: *const CpuContext, tick: Tick, snapshot: *mut u8);
    pub(crate) fn qe6502_load(cpu: *mut CpuContext, snapshot: *const u8) -> Tick;

    pub(crate) fn qe6502_get_pc(cpu: *const CpuContext) -> u16;
    pub(crate) fn qe6502_set_pc(cpu: *mut CpuContext, value: u16);
    pub(crate) fn qe6502_get_s(cpu: *const CpuContext) -> u8;
    pub(crate) fn qe6502_set_s(cpu: *mut CpuContext, value: u8);
    pub(crate) fn qe6502_get_a(cpu: *const CpuContext) -> u8;
    pub(crate) fn qe6502_set_a(cpu: *mut CpuContext, value: u8);
    pub(crate) fn qe6502_get_x(cpu: *const CpuContext) -> u8;
    pub(crate) fn qe6502_set_x(cpu: *mut CpuContext, value: u8);
    pub(crate) fn qe6502_get_y(cpu: *const CpuContext) -> u8;
    pub(crate) fn qe6502_set_y(cpu: *mut CpuContext, value: u8);
    pub(crate) fn qe6502_get_p(cpu: *const CpuContext) -> u8;
    pub(crate) fn qe6502_set_p(cpu: *mut CpuContext, value: u8);

    pub(crate) fn qe6502_get_flag_c(cpu: *const CpuContext) -> u8;
    pub(crate) fn qe6502_set_flag_c(cpu: *mut CpuContext, value: u8);
    pub(crate) fn qe6502_get_flag_z(cpu: *const CpuContext) -> u8;
    pub(crate) fn qe6502_set_flag_z(cpu: *mut CpuContext, value: u8);
    pub(crate) fn qe6502_get_flag_i(cpu: *const CpuContext) -> u8;
    pub(crate) fn qe6502_set_flag_i(cpu: *mut CpuContext, value: u8);
    pub(crate) fn qe6502_get_flag_d(cpu: *const CpuContext) -> u8;
    pub(crate) fn qe6502_set_flag_d(cpu: *mut CpuContext, value: u8);
    pub(crate) fn qe6502_get_flag_b(cpu: *const CpuContext) -> u8;
    pub(crate) fn qe6502_set_flag_b(cpu: *mut CpuContext, value: u8);
    pub(crate) fn qe6502_get_flag_un(cpu: *const CpuContext) -> u8;
    pub(crate) fn qe6502_set_flag_un(cpu: *mut CpuContext, value: u8);
    pub(crate) fn qe6502_get_flag_v(cpu: *const CpuContext) -> u8;
    pub(crate) fn qe6502_set_flag_v(cpu: *mut CpuContext, value: u8);
    pub(crate) fn qe6502_get_flag_n(cpu: *const CpuContext) -> u8;
    pub(crate) fn qe6502_set_flag_n(cpu: *mut CpuContext, value: u8);
}
