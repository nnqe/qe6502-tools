package qe6502.smoke;

import qe6502.Cpu;
import qe6502.Model;

public final class Main {
    private static final int PROGRAM_ADDRESS = 0x8000;
    private static final int STORE_ADDRESS = 0x0200;

    private Main() {
    }

    public static void main(String[] args) {
        try {
            runSmoke();
            System.out.println("qe6502 Java smoke: PASS");
        } catch (RuntimeException ex) {
            System.err.println("qe6502 Java smoke: FAIL");
            System.err.println(ex.getClass().getSimpleName() + ": " + ex.getMessage());
            System.exit(1);
        }
    }

    private static void runSmoke() {
        System.out.println("qe6502 Java smoke: starting");

        try (Cpu cpu = new Cpu(Model.NMOS)) {
            byte[] memory = new byte[65536];

            memory[0x8000] = (byte)0xA9; // LDA #$42
            memory[0x8001] = 0x42;
            memory[0x8002] = (byte)0x8D; // STA $0200
            memory[0x8003] = 0x00;
            memory[0x8004] = 0x02;
            memory[0x8005] = 0x4C; // JMP $8000
            memory[0x8006] = 0x00;
            memory[0x8007] = (byte)0x80;

            cpu.jumpTo(PROGRAM_ADDRESS);
            System.out.println("Initial tick: address=0x" + hex4(cpu.address()) +
                " data=0x" + hex2(cpu.data()) +
                " write=" + cpu.isWrite());

            require(cpu.address() == PROGRAM_ADDRESS, "JumpTo did not produce the expected fetch address.");
            require(!cpu.isWrite(), "Initial JumpTo tick must be a read cycle.");

            runBusCycles(cpu, memory, 32);

            System.out.println("After 32 cycles: mem[0x0200]=0x" + hex2(memory[STORE_ADDRESS] & 0xff) +
                " A=0x" + hex2(cpu.a()) +
                " PC=0x" + hex4(cpu.pc()));

            require((memory[STORE_ADDRESS] & 0xff) == 0x42, "Program did not store 0x42 at 0x0200.");
            require(cpu.a() == 0x42, "Accumulator did not keep the loaded value.");

            cpu.carryFlag(true);
            cpu.zeroFlag(false);
            cpu.negativeFlag(true);
            require(cpu.carryFlag(), "CarryFlag setter/getter failed.");
            require(!cpu.zeroFlag(), "ZeroFlag setter/getter failed.");
            require(cpu.negativeFlag(), "NegativeFlag setter/getter failed.");

            cpu.nmiAsserted(true);
            cpu.irqAsserted(true);
            require(cpu.nmiAsserted(), "NmiAsserted setter/getter failed.");
            require(cpu.irqAsserted(), "IrqAsserted setter/getter failed.");
            cpu.nmiAsserted(false);
            cpu.irqAsserted(false);
            require(!cpu.nmiAsserted(), "NmiAsserted deassert failed.");
            require(!cpu.irqAsserted(), "IrqAsserted deassert failed.");

            byte[] snapshot = cpu.save();
            int savedA = cpu.a();
            int savedPc = cpu.pc();
            int savedTick = cpu.rawTick();

            cpu.a(0x00);
            cpu.pc(0x1234);
            cpu.load(snapshot);

            require(cpu.a() == savedA, "Save/Load did not restore A.");
            require(cpu.pc() == savedPc, "Save/Load did not restore PC.");
            require(cpu.rawTick() == savedTick, "Save/Load did not restore the raw tick.");

            System.out.println("Snapshot restore: A=0x" + hex2(cpu.a()) +
                " PC=0x" + hex4(cpu.pc()) +
                " rawTick=0x" + hex8(cpu.rawTick()));
        }
    }

    private static void runBusCycles(Cpu cpu, byte[] memory, int cycleCount) {
        for (int i = 0; i < cycleCount; ++i) {
            if (cpu.isWrite()) {
                memory[cpu.address()] = (byte)cpu.data();
                cpu.tick();
            } else {
                cpu.tick(memory[cpu.address()] & 0xff);
            }
        }
    }

    private static void require(boolean condition, String message) {
        if (!condition) {
            throw new IllegalStateException(message);
        }
    }

    private static String hex2(int value) {
        return String.format("%02X", value & 0xff);
    }

    private static String hex4(int value) {
        return String.format("%04X", value & 0xffff);
    }

    private static String hex8(int value) {
        return String.format("%08X", value);
    }
}
