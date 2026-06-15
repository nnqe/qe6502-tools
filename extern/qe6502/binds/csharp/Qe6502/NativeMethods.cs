using System.Runtime.InteropServices;

namespace Qe6502
{
    [StructLayout(LayoutKind.Sequential, Size = NativeMethods.ContextSize)]
    internal struct Qe6502AbiContext
    {
#pragma warning disable 0169
        private ulong _0;
        private ulong _1;
        private ulong _2;
        private ulong _3;
        private ulong _4;
        private ulong _5;
        private ulong _6;
        private ulong _7;
#pragma warning restore 0169
    }

    internal static class NativeMethods
    {
        internal const string LibraryName = "libqe6502";
        internal const int ContextSize = 64;
        internal const int SnapshotSize = 64;

        [DllImport(LibraryName, EntryPoint = "qe6502abi_version", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        internal static extern uint Version();

        [DllImport(LibraryName, EntryPoint = "qe6502abi_setup", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        internal static extern void Setup(ref Qe6502AbiContext ctx, uint model);

        [DllImport(LibraryName, EntryPoint = "qe6502abi_restart", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        internal static extern uint Restart(ref Qe6502AbiContext ctx);

        [DllImport(LibraryName, EntryPoint = "qe6502abi_tick", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        internal static extern uint Tick(ref Qe6502AbiContext ctx, uint bus);

        [DllImport(LibraryName, EntryPoint = "qe6502abi_goto", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        internal static extern uint Goto(ref Qe6502AbiContext ctx, uint address);

        [DllImport(LibraryName, EntryPoint = "qe6502abi_nmi_assert", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        internal static extern void NmiAssert(ref Qe6502AbiContext ctx, byte assertNmi);

        [DllImport(LibraryName, EntryPoint = "qe6502abi_irq_assert", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        internal static extern void IrqAssert(ref Qe6502AbiContext ctx, byte assertIrq);

        [DllImport(LibraryName, EntryPoint = "qe6502abi_is_nmi_asserted", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        internal static extern byte IsNmiAsserted(ref Qe6502AbiContext ctx);

        [DllImport(LibraryName, EntryPoint = "qe6502abi_is_irq_asserted", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        internal static extern byte IsIrqAsserted(ref Qe6502AbiContext ctx);

        [DllImport(LibraryName, EntryPoint = "qe6502abi_save", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        internal static extern void Save(ref Qe6502AbiContext ctx, uint tick, [Out] byte[] snapshot);

        [DllImport(LibraryName, EntryPoint = "qe6502abi_load", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        internal static extern uint Load(ref Qe6502AbiContext ctx, [In] byte[] snapshot);

        [DllImport(LibraryName, EntryPoint = "qe6502abi_get_pc", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        internal static extern uint GetPc(ref Qe6502AbiContext ctx);

        [DllImport(LibraryName, EntryPoint = "qe6502abi_set_pc", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        internal static extern void SetPc(ref Qe6502AbiContext ctx, uint value);

        [DllImport(LibraryName, EntryPoint = "qe6502abi_get_s", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        internal static extern uint GetS(ref Qe6502AbiContext ctx);

        [DllImport(LibraryName, EntryPoint = "qe6502abi_set_s", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        internal static extern void SetS(ref Qe6502AbiContext ctx, uint value);

        [DllImport(LibraryName, EntryPoint = "qe6502abi_get_a", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        internal static extern uint GetA(ref Qe6502AbiContext ctx);

        [DllImport(LibraryName, EntryPoint = "qe6502abi_set_a", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        internal static extern void SetA(ref Qe6502AbiContext ctx, uint value);

        [DllImport(LibraryName, EntryPoint = "qe6502abi_get_x", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        internal static extern uint GetX(ref Qe6502AbiContext ctx);

        [DllImport(LibraryName, EntryPoint = "qe6502abi_set_x", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        internal static extern void SetX(ref Qe6502AbiContext ctx, uint value);

        [DllImport(LibraryName, EntryPoint = "qe6502abi_get_y", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        internal static extern uint GetY(ref Qe6502AbiContext ctx);

        [DllImport(LibraryName, EntryPoint = "qe6502abi_set_y", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        internal static extern void SetY(ref Qe6502AbiContext ctx, uint value);

        [DllImport(LibraryName, EntryPoint = "qe6502abi_get_p", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        internal static extern uint GetP(ref Qe6502AbiContext ctx);

        [DllImport(LibraryName, EntryPoint = "qe6502abi_set_p", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        internal static extern void SetP(ref Qe6502AbiContext ctx, uint value);

        [DllImport(LibraryName, EntryPoint = "qe6502abi_get_model", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        internal static extern uint GetModel(ref Qe6502AbiContext ctx);

        [DllImport(LibraryName, EntryPoint = "qe6502abi_set_model", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        internal static extern void SetModel(ref Qe6502AbiContext ctx, uint value);
    }
}
