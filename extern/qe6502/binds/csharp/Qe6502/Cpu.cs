using System;
using System.Runtime.CompilerServices;

namespace Qe6502
{
    /// <summary>Supported qe6502 CPU models.</summary>
    public enum Model : uint
    {
        /// <summary>NMOS 6502 model.</summary>
        Nmos = 0,

        /// <summary>NES/RP2A03 CPU model.</summary>
        Nes = 1,

        /// <summary>WDC 65C02 model.</summary>
        Wdc = 2,

        /// <summary>Rockwell 65C02 model.</summary>
        Rockwell = 3,

        /// <summary>Synertek 65C02 model.</summary>
        St = 4
    }

    /// <summary>Cycle-accurate qe6502 CPU wrapper over the stable native ABI.</summary>
    public sealed class Cpu
    {
        private Qe6502AbiContext _ctx;
        private uint _tick;

        /// <summary>Creates a CPU context for the selected model.</summary>
        public Cpu(Model model = Model.Nmos)
        {
            EnsureAbiVersion();
            ValidateModel(model);
            NativeMethods.Setup(ref _ctx, (uint)model);
        }

        /// <summary>The CPU model stored in the native context.</summary>
        public Model Model
        {
            get { return (Model)NativeMethods.GetModel(ref _ctx); }
            set
            {
                ValidateModel(value);
                NativeMethods.SetModel(ref _ctx, (uint)value);
            }
        }

        /// <summary>The raw packed ABI tick value from the last CPU cycle.</summary>
        public uint RawTick { get { return _tick; } }

        /// <summary>The current bus address from the last CPU cycle.</summary>
        public ushort Address { get { return (ushort)_tick; } }

        /// <summary>The bus data byte from the last CPU cycle.</summary>
        public byte Data { get { return (byte)(_tick >> 24); } }

        /// <summary>True when the current bus cycle writes Data to Address; otherwise the cycle reads from Address.</summary>
        public bool IsWrite { get { return (_tick & 0x00010000u) != 0u; } }

        /// <summary>True when the current bus cycle is an opcode fetch.</summary>
        public bool IsOpcodeFetch { get { return (_tick & 0x00020000u) != 0u; } }

        /// <summary>True while the CPU is performing the internal reset sequence.</summary>
        public bool IsInternalReset { get { return (_tick & 0x00400000u) != 0u; } }

        /// <summary>True when the CPU is jammed by a KIL/JAM opcode.</summary>
        public bool IsJammed { get { return (_tick & 0x00800000u) != 0u; } }

        /// <summary>Program counter register.</summary>
        public ushort PC
        {
            get { return (ushort)NativeMethods.GetPc(ref _ctx); }
            set { NativeMethods.SetPc(ref _ctx, value); }
        }

        /// <summary>Stack pointer register.</summary>
        public byte S
        {
            get { return (byte)NativeMethods.GetS(ref _ctx); }
            set { NativeMethods.SetS(ref _ctx, value); }
        }

        /// <summary>Accumulator register.</summary>
        public byte A
        {
            get { return (byte)NativeMethods.GetA(ref _ctx); }
            set { NativeMethods.SetA(ref _ctx, value); }
        }

        /// <summary>X index register.</summary>
        public byte X
        {
            get { return (byte)NativeMethods.GetX(ref _ctx); }
            set { NativeMethods.SetX(ref _ctx, value); }
        }

        /// <summary>Y index register.</summary>
        public byte Y
        {
            get { return (byte)NativeMethods.GetY(ref _ctx); }
            set { NativeMethods.SetY(ref _ctx, value); }
        }

        /// <summary>Processor status register.</summary>
        public byte P
        {
            get { return (byte)NativeMethods.GetP(ref _ctx); }
            set { NativeMethods.SetP(ref _ctx, value); }
        }

        /// <summary>Carry processor status flag.</summary>
        public bool CarryFlag
        {
            get { return (P & 0x01u) != 0u; }
            set { SetPFlag(0x01u, value); }
        }

        /// <summary>Zero processor status flag.</summary>
        public bool ZeroFlag
        {
            get { return (P & 0x02u) != 0u; }
            set { SetPFlag(0x02u, value); }
        }

        /// <summary>Interrupt-disable processor status flag.</summary>
        public bool InterruptDisableFlag
        {
            get { return (P & 0x04u) != 0u; }
            set { SetPFlag(0x04u, value); }
        }

        /// <summary>Decimal-mode processor status flag.</summary>
        public bool DecimalFlag
        {
            get { return (P & 0x08u) != 0u; }
            set { SetPFlag(0x08u, value); }
        }

        /// <summary>Break processor status flag.</summary>
        public bool BreakFlag
        {
            get { return (P & 0x10u) != 0u; }
            set { SetPFlag(0x10u, value); }
        }

        /// <summary>Unused processor status flag bit.</summary>
        public bool UnusedFlag
        {
            get { return (P & 0x20u) != 0u; }
            set { SetPFlag(0x20u, value); }
        }

        /// <summary>Overflow processor status flag.</summary>
        public bool OverflowFlag
        {
            get { return (P & 0x40u) != 0u; }
            set { SetPFlag(0x40u, value); }
        }

        /// <summary>Negative processor status flag.</summary>
        public bool NegativeFlag
        {
            get { return (P & 0x80u) != 0u; }
            set { SetPFlag(0x80u, value); }
        }

        /// <summary>True when the NMI input is logically asserted.</summary>
        public bool NmiAsserted
        {
            get { return NativeMethods.IsNmiAsserted(ref _ctx) != 0; }
            set { NativeMethods.NmiAssert(ref _ctx, value ? (byte)1 : (byte)0); }
        }

        /// <summary>True when the IRQ input is logically asserted.</summary>
        public bool IrqAsserted
        {
            get { return NativeMethods.IsIrqAsserted(ref _ctx) != 0; }
            set { NativeMethods.IrqAssert(ref _ctx, value ? (byte)1 : (byte)0); }
        }

        /// <summary>Starts the CPU reset sequence and stores the first reset tick.</summary>
        public void Restart()
        {
            _tick = NativeMethods.Restart(ref _ctx);
        }

        /// <summary>Places the CPU at an address and stores the first fetch tick.</summary>
        public void JumpTo(ushort address)
        {
            _tick = NativeMethods.Goto(ref _ctx, address);
        }

        /// <summary>Runs one bus cycle, feeding the byte observed on the data bus.</summary>
        public void Tick(byte data = 0)
        {
            _tick = NativeMethods.Tick(ref _ctx, data);
        }

        /// <summary>Saves the portable 64-byte CPU snapshot, including the last tick.</summary>
        public byte[] Save()
        {
            byte[] snapshot = new byte[NativeMethods.SnapshotSize];
            NativeMethods.Save(ref _ctx, _tick, snapshot);
            return snapshot;
        }

        /// <summary>Loads a portable 64-byte CPU snapshot and restores its tick.</summary>
        public void Load(byte[] snapshot)
        {
            if (snapshot == null) {
                throw new ArgumentNullException("snapshot");
            }

            if (snapshot.Length != NativeMethods.SnapshotSize) {
                throw new ArgumentException("Snapshot must be exactly 64 bytes.", "snapshot");
            }

            _tick = NativeMethods.Load(ref _ctx, snapshot);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private void SetPFlag(uint mask, bool value)
        {
            uint p = P;

            if (value) {
                p |= mask;
            } else {
                p &= ~mask;
            }

            P = (byte)p;
        }

        private static void EnsureAbiVersion()
        {
            uint version = NativeMethods.Version();
            uint major = version >> 16;
            uint minor = version & 0x0000FFFFu;

            if (major != NativeVersion.CompiledAbiVersionMajor ||
                minor < NativeVersion.CompiledAbiVersionMinor) {
                throw new InvalidOperationException(
                    "Unsupported qe6502 ABI version 0x" + version.ToString("X8") +
                    "; expected ABI-compatible version 0x" +
                    NativeVersion.CompiledAbiVersion.ToString("X8") +
                    " or newer with major " +
                    NativeVersion.CompiledAbiVersionMajor.ToString() + ".");
            }
        }

        private static void ValidateModel(Model model)
        {
            if (model < Model.Nmos || model > Model.St) {
                throw new ArgumentOutOfRangeException("model", model, "Invalid qe6502 CPU model.");
            }
        }
    }
}
