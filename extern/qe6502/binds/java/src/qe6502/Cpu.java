package qe6502;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.util.Objects;

/** Cycle-accurate qe6502 CPU wrapper over the stable native ABI. */
public final class Cpu implements AutoCloseable {
    private final Arena arena;
    private final MemorySegment ctx;
    private boolean closed;
    private int tick;

    /** Creates an NMOS 6502 CPU context. */
    public Cpu() {
        this(Model.NMOS);
    }

    /**
     * Creates a CPU context for the selected model.
     *
     * @param model CPU model to initialize
     */
    public Cpu(Model model) {
        Objects.requireNonNull(model, "model");
        NativeLibrary nativeLibrary = SharedNative.LIBRARY;
        ensureAbiVersion(nativeLibrary);
        arena = Arena.ofConfined();
        ctx = nativeLibrary.allocateContext(arena);
        nativeLibrary.setup(ctx, model.abiValue());
    }

    /**
     * Returns the CPU model stored in the native context.
     *
     * @return current CPU model
     */
    public Model model() {
        ensureOpen();
        return Model.fromAbiValue(SharedNative.LIBRARY.getModel(ctx));
    }

    /**
     * Changes the CPU model stored in the native context.
     *
     * @param model CPU model to store
     */
    public void model(Model model) {
        ensureOpen();
        Objects.requireNonNull(model, "model");
        SharedNative.LIBRARY.setModel(ctx, model.abiValue());
    }

    /**
     * Returns the raw packed ABI tick value from the last CPU cycle.
     *
     * @return raw packed tick value
     */
    public int rawTick() {
        return tick;
    }

    /**
     * Returns the current bus address from the last CPU cycle.
     *
     * @return 16-bit bus address
     */
    public int address() {
        return tick & 0xffff;
    }

    /**
     * Returns the bus data byte from the last CPU cycle.
     *
     * @return 8-bit bus data
     */
    public int data() {
        return (tick >>> 24) & 0xff;
    }

    /**
     * Reports whether the current bus cycle writes data to address.
     *
     * @return true for a write cycle, false for a read cycle
     */
    public boolean isWrite() {
        return (tick & 0x00010000) != 0;
    }

    /**
     * Reports whether the current bus cycle is an opcode fetch.
     *
     * @return true for an opcode-fetch cycle
     */
    public boolean isOpcodeFetch() {
        return (tick & 0x00020000) != 0;
    }

    /**
     * Reports whether the CPU is performing the internal reset sequence.
     *
     * @return true while reset sequencing is active
     */
    public boolean isInternalReset() {
        return (tick & 0x00400000) != 0;
    }

    /**
     * Reports whether the CPU is jammed by a KIL/JAM opcode.
     *
     * @return true when the CPU is jammed
     */
    public boolean isJammed() {
        return (tick & 0x00800000) != 0;
    }

    /**
     * Returns the program counter register.
     *
     * @return 16-bit program counter
     */
    public int pc() {
        ensureOpen();
        return SharedNative.LIBRARY.getPc(ctx) & 0xffff;
    }

    /**
     * Sets the program counter register.
     *
     * @param value 16-bit program counter value
     */
    public void pc(int value) {
        ensureOpen();
        SharedNative.LIBRARY.setPc(ctx, value & 0xffff);
    }

    /**
     * Returns the stack pointer register.
     *
     * @return 8-bit stack pointer
     */
    public int s() {
        ensureOpen();
        return SharedNative.LIBRARY.getS(ctx) & 0xff;
    }

    /**
     * Sets the stack pointer register.
     *
     * @param value 8-bit stack pointer value
     */
    public void s(int value) {
        ensureOpen();
        SharedNative.LIBRARY.setS(ctx, value & 0xff);
    }

    /**
     * Returns the accumulator register.
     *
     * @return 8-bit accumulator value
     */
    public int a() {
        ensureOpen();
        return SharedNative.LIBRARY.getA(ctx) & 0xff;
    }

    /**
     * Sets the accumulator register.
     *
     * @param value 8-bit accumulator value
     */
    public void a(int value) {
        ensureOpen();
        SharedNative.LIBRARY.setA(ctx, value & 0xff);
    }

    /**
     * Returns the X index register.
     *
     * @return 8-bit X index value
     */
    public int x() {
        ensureOpen();
        return SharedNative.LIBRARY.getX(ctx) & 0xff;
    }

    /**
     * Sets the X index register.
     *
     * @param value 8-bit X index value
     */
    public void x(int value) {
        ensureOpen();
        SharedNative.LIBRARY.setX(ctx, value & 0xff);
    }

    /**
     * Returns the Y index register.
     *
     * @return 8-bit Y index value
     */
    public int y() {
        ensureOpen();
        return SharedNative.LIBRARY.getY(ctx) & 0xff;
    }

    /**
     * Sets the Y index register.
     *
     * @param value 8-bit Y index value
     */
    public void y(int value) {
        ensureOpen();
        SharedNative.LIBRARY.setY(ctx, value & 0xff);
    }

    /**
     * Returns the processor status register.
     *
     * @return 8-bit processor status value
     */
    public int p() {
        ensureOpen();
        return SharedNative.LIBRARY.getP(ctx) & 0xff;
    }

    /**
     * Sets the processor status register.
     *
     * @param value 8-bit processor status value
     */
    public void p(int value) {
        ensureOpen();
        SharedNative.LIBRARY.setP(ctx, value & 0xff);
    }

    /**
     * Returns the carry status flag.
     *
     * @return true when the carry flag is set
     */
    public boolean carryFlag() { return (p() & 0x01) != 0; }

    /**
     * Sets the carry status flag.
     *
     * @param value true to set the flag, false to clear it
     */
    public void carryFlag(boolean value) { setPFlag(0x01, value); }

    /**
     * Returns the zero status flag.
     *
     * @return true when the zero flag is set
     */
    public boolean zeroFlag() { return (p() & 0x02) != 0; }

    /**
     * Sets the zero status flag.
     *
     * @param value true to set the flag, false to clear it
     */
    public void zeroFlag(boolean value) { setPFlag(0x02, value); }

    /**
     * Returns the interrupt-disable status flag.
     *
     * @return true when the interrupt-disable flag is set
     */
    public boolean interruptDisableFlag() { return (p() & 0x04) != 0; }

    /**
     * Sets the interrupt-disable status flag.
     *
     * @param value true to set the flag, false to clear it
     */
    public void interruptDisableFlag(boolean value) { setPFlag(0x04, value); }

    /**
     * Returns the decimal-mode status flag.
     *
     * @return true when the decimal-mode flag is set
     */
    public boolean decimalFlag() { return (p() & 0x08) != 0; }

    /**
     * Sets the decimal-mode status flag.
     *
     * @param value true to set the flag, false to clear it
     */
    public void decimalFlag(boolean value) { setPFlag(0x08, value); }

    /**
     * Returns the break status flag.
     *
     * @return true when the break flag is set
     */
    public boolean breakFlag() { return (p() & 0x10) != 0; }

    /**
     * Sets the break status flag.
     *
     * @param value true to set the flag, false to clear it
     */
    public void breakFlag(boolean value) { setPFlag(0x10, value); }

    /**
     * Returns the unused status flag.
     *
     * @return true when the unused flag bit is set
     */
    public boolean unusedFlag() { return (p() & 0x20) != 0; }

    /**
     * Sets the unused status flag.
     *
     * @param value true to set the flag, false to clear it
     */
    public void unusedFlag(boolean value) { setPFlag(0x20, value); }

    /**
     * Returns the overflow status flag.
     *
     * @return true when the overflow flag is set
     */
    public boolean overflowFlag() { return (p() & 0x40) != 0; }

    /**
     * Sets the overflow status flag.
     *
     * @param value true to set the flag, false to clear it
     */
    public void overflowFlag(boolean value) { setPFlag(0x40, value); }

    /**
     * Returns the negative status flag.
     *
     * @return true when the negative flag is set
     */
    public boolean negativeFlag() { return (p() & 0x80) != 0; }

    /**
     * Sets the negative status flag.
     *
     * @param value true to set the flag, false to clear it
     */
    public void negativeFlag(boolean value) { setPFlag(0x80, value); }

    /**
     * Reports whether the NMI input is logically asserted.
     *
     * @return true when NMI is asserted
     */
    public boolean nmiAsserted() {
        ensureOpen();
        return SharedNative.LIBRARY.isNmiAsserted(ctx);
    }

    /**
     * Asserts or deasserts the NMI input.
     *
     * @param value true to assert NMI, false to deassert it
     */
    public void nmiAsserted(boolean value) {
        ensureOpen();
        SharedNative.LIBRARY.nmiAssert(ctx, value);
    }

    /**
     * Reports whether the IRQ input is logically asserted.
     *
     * @return true when IRQ is asserted
     */
    public boolean irqAsserted() {
        ensureOpen();
        return SharedNative.LIBRARY.isIrqAsserted(ctx);
    }

    /**
     * Asserts or deasserts the IRQ input.
     *
     * @param value true to assert IRQ, false to deassert it
     */
    public void irqAsserted(boolean value) {
        ensureOpen();
        SharedNative.LIBRARY.irqAssert(ctx, value);
    }

    /** Starts the CPU reset sequence and stores the first reset tick. */
    public void restart() {
        ensureOpen();
        tick = SharedNative.LIBRARY.restart(ctx);
    }

    /**
     * Places the CPU at an address and stores the first fetch tick.
     *
     * @param address 16-bit address to place in the program counter
     */
    public void jumpTo(int address) {
        ensureOpen();
        tick = SharedNative.LIBRARY.goTo(ctx, address & 0xffff);
    }

    /** Runs one bus cycle, feeding zero on the data bus. */
    public void tick() {
        tick(0);
    }

    /**
     * Runs one bus cycle, feeding the byte observed on the data bus.
     *
     * @param data 8-bit data bus value for the cycle
     */
    public void tick(int data) {
        ensureOpen();
        tick = SharedNative.LIBRARY.tick(ctx, data & 0xff);
    }

    /**
     * Saves the portable 64-byte CPU snapshot, including the last tick.
     *
     * @return snapshot bytes
     */
    public byte[] save() {
        ensureOpen();
        return SharedNative.LIBRARY.save(ctx, tick);
    }

    /**
     * Loads a portable 64-byte CPU snapshot and restores its tick.
     *
     * @param snapshot snapshot bytes to load
     */
    public void load(byte[] snapshot) {
        ensureOpen();
        Objects.requireNonNull(snapshot, "snapshot");
        if (snapshot.length != NativeLibrary.SNAPSHOT_SIZE) {
            throw new IllegalArgumentException("Snapshot must be exactly 64 bytes.");
        }
        tick = SharedNative.LIBRARY.load(ctx, snapshot);
    }

    @Override
    public void close() {
        if (!closed) {
            closed = true;
            arena.close();
        }
    }

    private void setPFlag(int mask, boolean value) {
        int current = p();
        p(value ? (current | mask) : (current & ~mask));
    }

    private void ensureOpen() {
        if (closed) {
            throw new IllegalStateException("CPU context is closed.");
        }
    }

    private static void ensureAbiVersion(NativeLibrary nativeLibrary) {
        int version = nativeLibrary.version();
        int major = (version >>> 16) & 0xffff;
        int minor = version & 0xffff;

        if (major != NativeVersion.COMPILED_ABI_VERSION_MAJOR ||
            minor < NativeVersion.COMPILED_ABI_VERSION_MINOR) {
            throw new IllegalStateException(
                "Unsupported qe6502 ABI version 0x" + Integer.toHexString(version) +
                "; expected ABI-compatible version 0x" +
                Integer.toHexString(NativeVersion.COMPILED_ABI_VERSION) +
                " or newer with major " + NativeVersion.COMPILED_ABI_VERSION_MAJOR + ".");
        }
    }

    private static final class SharedNative {
        static final NativeLibrary LIBRARY = new NativeLibrary();
    }
}
