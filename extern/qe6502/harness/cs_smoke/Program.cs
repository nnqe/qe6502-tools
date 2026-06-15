using System;
using Qe6502;

namespace Qe6502Smoke
{
    internal static class Program
    {
        private const ushort ProgramAddress = 0x8000;
        private const ushort StoreAddress = 0x0200;

        private static int Main()
        {
            try {
                RunSmoke();
                Console.WriteLine("qe6502 C# smoke: PASS");
                return 0;
            } catch (Exception ex) {
                Console.Error.WriteLine("qe6502 C# smoke: FAIL");
                Console.Error.WriteLine(ex.GetType().Name + ": " + ex.Message);
                return 1;
            }
        }

        private static void RunSmoke()
        {
            Console.WriteLine("qe6502 C# smoke: starting");

            Cpu cpu = new Cpu(Model.Nmos);
            byte[] memory = new byte[65536];

            memory[0x8000] = 0xA9; // LDA #$42
            memory[0x8001] = 0x42;
            memory[0x8002] = 0x8D; // STA $0200
            memory[0x8003] = 0x00;
            memory[0x8004] = 0x02;
            memory[0x8005] = 0x4C; // JMP $8000
            memory[0x8006] = 0x00;
            memory[0x8007] = 0x80;

            cpu.JumpTo(ProgramAddress);
            Console.WriteLine("Initial tick: address=0x" + cpu.Address.ToString("X4") +
                              " data=0x" + cpu.Data.ToString("X2") +
                              " write=" + cpu.IsWrite);

            Require(cpu.Address == ProgramAddress, "JumpTo did not produce the expected fetch address.");
            Require(!cpu.IsWrite, "Initial JumpTo tick must be a read cycle.");

            RunBusCycles(cpu, memory, 32);

            Console.WriteLine("After 32 cycles: mem[0x0200]=0x" + memory[StoreAddress].ToString("X2") +
                              " A=0x" + cpu.A.ToString("X2") +
                              " PC=0x" + cpu.PC.ToString("X4"));

            Require(memory[StoreAddress] == 0x42, "Program did not store 0x42 at 0x0200.");
            Require(cpu.A == 0x42, "Accumulator did not keep the loaded value.");

            cpu.CarryFlag = true;
            cpu.ZeroFlag = false;
            cpu.NegativeFlag = true;
            Require(cpu.CarryFlag, "CarryFlag setter/getter failed.");
            Require(!cpu.ZeroFlag, "ZeroFlag setter/getter failed.");
            Require(cpu.NegativeFlag, "NegativeFlag setter/getter failed.");

            cpu.NmiAsserted = true;
            cpu.IrqAsserted = true;
            Require(cpu.NmiAsserted, "NmiAsserted setter/getter failed.");
            Require(cpu.IrqAsserted, "IrqAsserted setter/getter failed.");
            cpu.NmiAsserted = false;
            cpu.IrqAsserted = false;
            Require(!cpu.NmiAsserted, "NmiAsserted deassert failed.");
            Require(!cpu.IrqAsserted, "IrqAsserted deassert failed.");

            byte[] snapshot = cpu.Save();
            byte savedA = cpu.A;
            ushort savedPc = cpu.PC;
            uint savedTick = cpu.RawTick;

            cpu.A = 0x00;
            cpu.PC = 0x1234;
            cpu.Load(snapshot);

            Require(cpu.A == savedA, "Save/Load did not restore A.");
            Require(cpu.PC == savedPc, "Save/Load did not restore PC.");
            Require(cpu.RawTick == savedTick, "Save/Load did not restore the raw tick.");

            Console.WriteLine("Snapshot restore: A=0x" + cpu.A.ToString("X2") +
                              " PC=0x" + cpu.PC.ToString("X4") +
                              " rawTick=0x" + cpu.RawTick.ToString("X8"));
        }

        private static void RunBusCycles(Cpu cpu, byte[] memory, int cycleCount)
        {
            for (int i = 0; i < cycleCount; ++i) {
                if (cpu.IsWrite) {
                    memory[cpu.Address] = cpu.Data;
                    cpu.Tick();
                } else {
                    cpu.Tick(memory[cpu.Address]);
                }
            }
        }

        private static void Require(bool condition, string message)
        {
            if (!condition) {
                throw new InvalidOperationException(message);
            }
        }
    }
}
