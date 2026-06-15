mod sys;

pub const SNAPSHOT_SIZE: usize = sys::SNAPSHOT_SIZE;
pub type Snapshot = [u8; SNAPSHOT_SIZE];

#[repr(u8)]
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Model {
    Nmos = sys::MODEL_NMOS,
    Nes = sys::MODEL_NES,
    Wdc = sys::MODEL_WDC,
    Rw = sys::MODEL_RW,
    St = sys::MODEL_ST,
}

pub struct Cpu {
    ctx: sys::CpuContext,
    tick: sys::Tick,
}

impl Cpu {
    pub fn new(model: Model) -> Self {
        Self {
            ctx: unsafe { sys::qe6502_setup(model as u32) },
            tick: sys::Tick::default(),
        }
    }

    pub fn restart(&mut self) {
        self.tick = unsafe { sys::qe6502_restart(&mut self.ctx) };
    }

    pub fn jump_to(&mut self, address: u16) {
        self.tick = unsafe { sys::qe6502_goto(&mut self.ctx, address) };
    }

    #[inline(always)]
    pub fn tick(&mut self, bus: u8) {
        self.tick = unsafe { sys::qe6502_tick_exported(&mut self.ctx, bus) };
    }

    #[inline(always)]
    pub fn is_read(&self) -> bool {
        !self.is_write()
    }

    #[inline(always)]
    pub fn is_write(&self) -> bool {
        (self.tick.flags & sys::STATUS_WRITING) != 0
    }

    #[inline(always)]
    pub fn is_opcode_fetch(&self) -> bool {
        (self.tick.flags & sys::STATUS_OPCODE_FETCH) != 0
    }

    #[inline(always)]
    pub fn is_internal_reset(&self) -> bool {
        (self.tick.flags & sys::STATUS_INTERNAL_RESET) != 0
    }

    #[inline(always)]
    pub fn is_jammed(&self) -> bool {
        (self.tick.flags & sys::STATUS_CPU_JAMMED) != 0
    }

    #[inline(always)]
    pub fn bus_address(&self) -> u16 {
        self.tick.address
    }

    #[inline(always)]
    pub fn bus_data(&self) -> u8 {
        self.tick.data
    }

    #[inline(always)]
    pub fn pc(&self) -> u16 {
        unsafe { sys::qe6502_get_pc(&self.ctx) }
    }

    #[inline(always)]
    pub fn set_pc(&mut self, value: u16) {
        unsafe { sys::qe6502_set_pc(&mut self.ctx, value) };
    }

    #[inline(always)]
    pub fn s(&self) -> u8 {
        unsafe { sys::qe6502_get_s(&self.ctx) }
    }

    #[inline(always)]
    pub fn set_s(&mut self, value: u8) {
        unsafe { sys::qe6502_set_s(&mut self.ctx, value) };
    }

    #[inline(always)]
    pub fn a(&self) -> u8 {
        unsafe { sys::qe6502_get_a(&self.ctx) }
    }

    #[inline(always)]
    pub fn set_a(&mut self, value: u8) {
        unsafe { sys::qe6502_set_a(&mut self.ctx, value) };
    }

    #[inline(always)]
    pub fn x(&self) -> u8 {
        unsafe { sys::qe6502_get_x(&self.ctx) }
    }

    #[inline(always)]
    pub fn set_x(&mut self, value: u8) {
        unsafe { sys::qe6502_set_x(&mut self.ctx, value) };
    }

    #[inline(always)]
    pub fn y(&self) -> u8 {
        unsafe { sys::qe6502_get_y(&self.ctx) }
    }

    #[inline(always)]
    pub fn set_y(&mut self, value: u8) {
        unsafe { sys::qe6502_set_y(&mut self.ctx, value) };
    }

    #[inline(always)]
    pub fn p(&self) -> u8 {
        unsafe { sys::qe6502_get_p(&self.ctx) }
    }

    #[inline(always)]
    pub fn set_p(&mut self, value: u8) {
        unsafe { sys::qe6502_set_p(&mut self.ctx, value) };
    }

    #[inline(always)]
    pub fn carry_flag(&self) -> bool {
        unsafe { sys::qe6502_get_flag_c(&self.ctx) != 0 }
    }

    #[inline(always)]
    pub fn set_carry_flag(&mut self, value: bool) {
        unsafe { sys::qe6502_set_flag_c(&mut self.ctx, u8::from(value)) };
    }

    #[inline(always)]
    pub fn zero_flag(&self) -> bool {
        unsafe { sys::qe6502_get_flag_z(&self.ctx) != 0 }
    }

    #[inline(always)]
    pub fn set_zero_flag(&mut self, value: bool) {
        unsafe { sys::qe6502_set_flag_z(&mut self.ctx, u8::from(value)) };
    }

    #[inline(always)]
    pub fn interrupt_disable_flag(&self) -> bool {
        unsafe { sys::qe6502_get_flag_i(&self.ctx) != 0 }
    }

    #[inline(always)]
    pub fn set_interrupt_disable_flag(&mut self, value: bool) {
        unsafe { sys::qe6502_set_flag_i(&mut self.ctx, u8::from(value)) };
    }

    #[inline(always)]
    pub fn decimal_flag(&self) -> bool {
        unsafe { sys::qe6502_get_flag_d(&self.ctx) != 0 }
    }

    #[inline(always)]
    pub fn set_decimal_flag(&mut self, value: bool) {
        unsafe { sys::qe6502_set_flag_d(&mut self.ctx, u8::from(value)) };
    }

    #[inline(always)]
    pub fn break_flag(&self) -> bool {
        unsafe { sys::qe6502_get_flag_b(&self.ctx) != 0 }
    }

    #[inline(always)]
    pub fn set_break_flag(&mut self, value: bool) {
        unsafe { sys::qe6502_set_flag_b(&mut self.ctx, u8::from(value)) };
    }

    #[inline(always)]
    pub fn unused_flag(&self) -> bool {
        unsafe { sys::qe6502_get_flag_un(&self.ctx) != 0 }
    }

    #[inline(always)]
    pub fn set_unused_flag(&mut self, value: bool) {
        unsafe { sys::qe6502_set_flag_un(&mut self.ctx, u8::from(value)) };
    }

    #[inline(always)]
    pub fn overflow_flag(&self) -> bool {
        unsafe { sys::qe6502_get_flag_v(&self.ctx) != 0 }
    }

    #[inline(always)]
    pub fn set_overflow_flag(&mut self, value: bool) {
        unsafe { sys::qe6502_set_flag_v(&mut self.ctx, u8::from(value)) };
    }

    #[inline(always)]
    pub fn negative_flag(&self) -> bool {
        unsafe { sys::qe6502_get_flag_n(&self.ctx) != 0 }
    }

    #[inline(always)]
    pub fn set_negative_flag(&mut self, value: bool) {
        unsafe { sys::qe6502_set_flag_n(&mut self.ctx, u8::from(value)) };
    }

    pub fn nmi_asserted(&self) -> bool {
        unsafe { sys::qe6502_is_nmi_asserted(&self.ctx) != 0 }
    }

    pub fn set_nmi_asserted(&mut self, value: bool) {
        unsafe { sys::qe6502_nmi_assert(&mut self.ctx, u8::from(value)) };
    }

    pub fn irq_asserted(&self) -> bool {
        unsafe { sys::qe6502_is_irq_asserted(&self.ctx) != 0 }
    }

    pub fn set_irq_asserted(&mut self, value: bool) {
        unsafe { sys::qe6502_irq_assert(&mut self.ctx, u8::from(value)) };
    }

    pub fn save(&self) -> Snapshot {
        let mut snapshot = [0; SNAPSHOT_SIZE];
        unsafe {
            sys::qe6502_save(&self.ctx, self.tick, snapshot.as_mut_ptr());
        }
        snapshot
    }

    pub fn load(&mut self, snapshot: &Snapshot) {
        self.tick = unsafe { sys::qe6502_load(&mut self.ctx, snapshot.as_ptr()) };
    }

}
