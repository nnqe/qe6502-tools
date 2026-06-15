using System;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Text.RegularExpressions;
using Qe6502;

namespace Qe6502CsKlaus2m5
{
    internal static class Program
    {
        private const int MemorySize = 0x10000;
        private const ushort StartAddress = 0x0400;

        private const string StandardRom = "6502_functional_test.hex";
        private const ushort StandardSuccessAddress = 0x3469;
        private const ulong StandardExpectedCycles = 30646176ul;

        private const string ExtendedRom = "65C02_extended_opcodes_test.hex";
        private const ushort ExtendedSuccessAddress = 0x24F1;
        private const ulong ExtendedExpectedCycles = 21986985ul;

        private static readonly string[,] DefaultSuite = new string[,]
        {
            { "nmos", "standard" },
            { "wdc", "standard" },
            { "wdc", "extended" },
            { "rw", "standard" },
            { "rw", "extended" },
            { "st", "standard" }
        };

        private sealed class ModelInfo
        {
            public ModelInfo(Model model, string displayName)
            {
                Model = model;
                DisplayName = displayName;
            }

            public Model Model { get; private set; }
            public string DisplayName { get; private set; }
        }

        private sealed class TestImage
        {
            public TestImage(byte[] image, ushort successAddress, ulong expectedCycles)
            {
                Image = image;
                SuccessAddress = successAddress;
                ExpectedCycles = expectedCycles;
            }

            public byte[] Image { get; private set; }
            public ushort SuccessAddress { get; private set; }
            public ulong ExpectedCycles { get; private set; }
        }

        private sealed class RomSet
        {
            public RomSet(byte[] standard, byte[] extended)
            {
                Standard = standard;
                Extended = extended;
            }

            public byte[] Standard { get; private set; }
            public byte[] Extended { get; private set; }
        }

        private sealed class RunResult
        {
            public bool Passed;
            public string Message = "CPU Error";
            public double MHz;
            public ulong BusTicks;
            public ulong OpcodeCycles;
        }

        private static int Main(string[] args)
        {
            try {
                return Run(args);
            } catch (Exception ex) {
                Console.Error.WriteLine("qe6502 C# Klaus2m5: FAIL");
                Console.Error.WriteLine(ex.GetType().Name + ": " + ex.Message);
                return 1;
            }
        }

        private static int Run(string[] args)
        {
            string romDir;
            string singleModel = null;
            string singleTest = null;

            if (args.Length == 0) {
                romDir = Path.Combine(AppContext.BaseDirectory, "klaus2m5");
            } else if (args.Length == 1) {
                romDir = args[0];
            } else if (args.Length == 2) {
                romDir = Path.Combine(AppContext.BaseDirectory, "klaus2m5");
                singleModel = args[0];
                singleTest = args[1];
            } else if (args.Length == 3) {
                romDir = args[0];
                singleModel = args[1];
                singleTest = args[2];
            } else {
                PrintUsage();
                return 1;
            }

            RomSet roms = LoadRoms(romDir);
            int failures = 0;

            if (singleModel != null && singleTest != null) {
                failures += RunOne(roms, singleModel, singleTest) ? 0 : 1;
            } else {
                for (int i = 0; i < DefaultSuite.GetLength(0); ++i) {
                    failures += RunOne(roms, DefaultSuite[i, 0], DefaultSuite[i, 1]) ? 0 : 1;
                }
            }

            return failures == 0 ? 0 : 1;
        }

        private static void PrintUsage()
        {
            Console.Error.WriteLine(
                "Usage: dotnet Qe6502.CsKlaus2m5.dll [<rom-dir>] [<model> <test>]\n" +
                "       dotnet Qe6502.CsKlaus2m5.dll <model> <test>\n" +
                "\n" +
                "With no model/test arguments, runs the default v2 Klaus suite.\n" +
                "With no rom-dir, uses ./klaus2m5 next to the executable.\n" +
                "\n" +
                "Models: nmos, wdc, rw, st\n" +
                "Tests: standard, extended");
        }

        private static bool RunOne(RomSet roms, string modelName, string testName)
        {
            ModelInfo modelInfo = ParseModel(modelName);
            TestImage testImage = SelectTestImage(roms, testName, modelInfo);
            RunResult result = RunKlausTest(modelInfo.Model, testImage);

            Console.WriteLine(
                modelInfo.DisplayName + " CPU " + testName + " C# test " +
                (result.Passed ? "[PASS]" : "[FAIL]") + " " +
                result.Message + " (" +
                result.MHz.ToString("F2", CultureInfo.InvariantCulture) + " MHz, " +
                result.BusTicks.ToString(CultureInfo.InvariantCulture) + " bus ticks, " +
                result.OpcodeCycles.ToString(CultureInfo.InvariantCulture) + " opcode cycles)");

            return result.Passed;
        }

        private static RomSet LoadRoms(string romDir)
        {
            if (!Directory.Exists(romDir)) {
                throw new DirectoryNotFoundException("Klaus ROM directory not found: " + romDir);
            }

            byte[] standard = LoadHexRom(Path.Combine(romDir, StandardRom));
            byte[] extended = LoadHexRom(Path.Combine(romDir, ExtendedRom));
            return new RomSet(standard, extended);
        }

        private static byte[] LoadHexRom(string path)
        {
            string text = File.ReadAllText(path);
            MatchCollection matches = Regex.Matches(text, "0x[0-9a-fA-F]+");

            if (matches.Count != MemorySize) {
                throw new InvalidOperationException(
                    path + " contains " + matches.Count.ToString(CultureInfo.InvariantCulture) +
                    " bytes; expected " + MemorySize.ToString(CultureInfo.InvariantCulture) + ".");
            }

            byte[] image = new byte[MemorySize];
            for (int i = 0; i < image.Length; ++i) {
                image[i] = Convert.ToByte(matches[i].Value.Substring(2), 16);
            }

            return image;
        }

        private static ModelInfo ParseModel(string modelName)
        {
            if (modelName == "nmos" || modelName == "mos") {
                return new ModelInfo(Model.Nmos, "NMOS 6502");
            }

            if (modelName == "wdc") {
                return new ModelInfo(Model.Wdc, "WDC 65C02");
            }

            if (modelName == "rw" || modelName == "rockwell") {
                return new ModelInfo(Model.Rockwell, "Rockwell 65C02");
            }

            if (modelName == "st" || modelName == "synertek") {
                return new ModelInfo(Model.St, "Synertek 65C02");
            }

            throw new ArgumentException("Unknown model: " + modelName);
        }

        private static TestImage SelectTestImage(RomSet roms, string testName, ModelInfo modelInfo)
        {
            if (testName == "standard") {
                return new TestImage(roms.Standard, StandardSuccessAddress, StandardExpectedCycles);
            }

            if (testName == "extended") {
                if (modelInfo.Model != Model.Wdc && modelInfo.Model != Model.Rockwell) {
                    throw new ArgumentException(
                        "Extended test is only valid for WDC/Rockwell 65C02 v2 models, not " +
                        modelInfo.DisplayName + ".");
                }

                return new TestImage(roms.Extended, ExtendedSuccessAddress, ExtendedExpectedCycles);
            }

            throw new ArgumentException("Unknown test: " + testName);
        }

        private static RunResult RunKlausTest(Model model, TestImage testImage)
        {
            Cpu cpu = new Cpu(model);
            byte[] memory = new byte[MemorySize];
            Array.Copy(testImage.Image, memory, MemorySize);

            RunResult result = new RunResult();
            cpu.Restart();
            cpu.JumpTo(StartAddress);

            Stopwatch stopwatch = Stopwatch.StartNew();

            for (;;) {
                if (cpu.IsWrite) {
                    memory[cpu.Address] = cpu.Data;
                    cpu.Tick();
                } else {
                    cpu.Tick(memory[cpu.Address]);
                }

                ++result.BusTicks;

                if (cpu.IsOpcodeFetch) {
                    ++result.OpcodeCycles;
                    if (result.OpcodeCycles > 2ul * testImage.ExpectedCycles) {
                        result.Message = "Test fail, takes too many cycles!";
                        break;
                    }
                    if (cpu.Address == testImage.SuccessAddress) {
                        result.Passed = true;
                        result.Message = "OK";
                        break;
                    }
                }
            }

            stopwatch.Stop();
            result.MHz = EmulatedMHz(result.BusTicks, stopwatch.Elapsed.TotalSeconds);
            return result;
        }

        private static double EmulatedMHz(ulong busTicks, double seconds)
        {
            if (seconds <= 0.0) {
                return 0.0;
            }

            return ((double)busTicks / seconds) / 1000000.0;
        }
    }
}
