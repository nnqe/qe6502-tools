package qe6502.klaus2m5;

import qe6502.Cpu;
import qe6502.Model;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Locale;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public final class Main {
    private static final int MEMORY_SIZE = 0x10000;
    private static final int START_ADDRESS = 0x0400;

    private static final String STANDARD_ROM = "6502_functional_test.hex";
    private static final int STANDARD_SUCCESS_ADDRESS = 0x3469;
    private static final long STANDARD_EXPECTED_CYCLES = 30_646_176L;

    private static final String EXTENDED_ROM = "65C02_extended_opcodes_test.hex";
    private static final int EXTENDED_SUCCESS_ADDRESS = 0x24F1;
    private static final long EXTENDED_EXPECTED_CYCLES = 21_986_985L;

    private static final String[][] DEFAULT_SUITE = {
        { "nmos", "standard" },
        { "wdc", "standard" },
        { "wdc", "extended" },
        { "rw", "standard" },
        { "rw", "extended" },
        { "st", "standard" }
    };

    private Main() {
    }

    public static void main(String[] args) {
        try {
            System.exit(run(args));
        } catch (RuntimeException | IOException ex) {
            System.err.println("qe6502 Java Klaus2m5: FAIL");
            System.err.println(ex.getClass().getSimpleName() + ": " + ex.getMessage());
            System.exit(1);
        }
    }

    private static int run(String[] args) throws IOException {
        Path romDir;
        String singleModel = null;
        String singleTest = null;

        if (args.length == 0) {
            romDir = Path.of("klaus2m5");
        } else if (args.length == 1) {
            romDir = Path.of(args[0]);
        } else if (args.length == 2) {
            romDir = Path.of("klaus2m5");
            singleModel = args[0];
            singleTest = args[1];
        } else if (args.length == 3) {
            romDir = Path.of(args[0]);
            singleModel = args[1];
            singleTest = args[2];
        } else {
            printUsage();
            return 1;
        }

        RomSet roms = loadRoms(romDir);
        int failures = 0;

        if (singleModel != null && singleTest != null) {
            failures += runOne(roms, singleModel, singleTest) ? 0 : 1;
        } else {
            for (String[] entry : DEFAULT_SUITE) {
                failures += runOne(roms, entry[0], entry[1]) ? 0 : 1;
            }
        }

        return failures == 0 ? 0 : 1;
    }

    private static void printUsage() {
        System.err.println("Usage: java qe6502.klaus2m5.Main [<rom-dir>] [<model> <test>]");
        System.err.println("       java qe6502.klaus2m5.Main <model> <test>");
        System.err.println();
        System.err.println("With no model/test arguments, runs the default v2 Klaus suite.");
        System.err.println("With no rom-dir, uses ./klaus2m5 next to the executable.");
        System.err.println();
        System.err.println("Models: nmos, wdc, rw, st");
        System.err.println("Tests: standard, extended");
    }

    private static boolean runOne(RomSet roms, String modelName, String testName) {
        ModelInfo modelInfo = parseModel(modelName);
        TestImage testImage = selectTestImage(roms, testName, modelInfo);
        RunResult result = runKlausTest(modelInfo.model, testImage);

        System.out.println(modelInfo.displayName + " CPU " + testName + " Java test " +
            (result.passed ? "[PASS]" : "[FAIL]") + " " +
            result.message + " (" +
            String.format(Locale.ROOT, "%.2f", result.mhz) + " MHz, " +
            result.busTicks + " bus ticks, " +
            result.opcodeCycles + " opcode cycles)");

        return result.passed;
    }

    private static RomSet loadRoms(Path romDir) throws IOException {
        if (!Files.isDirectory(romDir)) {
            throw new IOException("Klaus ROM directory not found: " + romDir);
        }

        byte[] standard = loadHexRom(romDir.resolve(STANDARD_ROM));
        byte[] extended = loadHexRom(romDir.resolve(EXTENDED_ROM));
        return new RomSet(standard, extended);
    }

    private static byte[] loadHexRom(Path path) throws IOException {
        String text = Files.readString(path);
        Matcher matcher = Pattern.compile("0x[0-9a-fA-F]+").matcher(text);
        byte[] image = new byte[MEMORY_SIZE];
        int index = 0;

        while (matcher.find()) {
            if (index >= image.length) {
                break;
            }
            image[index++] = (byte)Integer.parseInt(matcher.group().substring(2), 16);
        }

        if (index != MEMORY_SIZE || matcher.find()) {
            throw new IllegalStateException(path + " contains " + index + " bytes; expected " + MEMORY_SIZE + ".");
        }

        return image;
    }

    private static ModelInfo parseModel(String modelName) {
        if (modelName.equals("nmos") || modelName.equals("mos")) {
            return new ModelInfo(Model.NMOS, "NMOS 6502");
        }

        if (modelName.equals("wdc")) {
            return new ModelInfo(Model.WDC, "WDC 65C02");
        }

        if (modelName.equals("rw") || modelName.equals("rockwell")) {
            return new ModelInfo(Model.ROCKWELL, "Rockwell 65C02");
        }

        if (modelName.equals("st") || modelName.equals("synertek")) {
            return new ModelInfo(Model.ST, "Synertek 65C02");
        }

        throw new IllegalArgumentException("Unknown model: " + modelName);
    }

    private static TestImage selectTestImage(RomSet roms, String testName, ModelInfo modelInfo) {
        if (testName.equals("standard")) {
            return new TestImage(roms.standard, STANDARD_SUCCESS_ADDRESS, STANDARD_EXPECTED_CYCLES);
        }

        if (testName.equals("extended")) {
            if (modelInfo.model != Model.WDC && modelInfo.model != Model.ROCKWELL) {
                throw new IllegalArgumentException(
                    "Extended test is only valid for WDC/Rockwell 65C02 v2 models, not " +
                    modelInfo.displayName + ".");
            }

            return new TestImage(roms.extended, EXTENDED_SUCCESS_ADDRESS, EXTENDED_EXPECTED_CYCLES);
        }

        throw new IllegalArgumentException("Unknown test: " + testName);
    }

    private static RunResult runKlausTest(Model model, TestImage testImage) {
        try (Cpu cpu = new Cpu(model)) {
            byte[] memory = testImage.image.clone();
            RunResult result = new RunResult();
            cpu.restart();
            cpu.jumpTo(START_ADDRESS);

            long startNanos = System.nanoTime();

            for (;;) {
                if (cpu.isWrite()) {
                    memory[cpu.address()] = (byte)cpu.data();
                    cpu.tick();
                } else {
                    cpu.tick(memory[cpu.address()] & 0xff);
                }

                ++result.busTicks;

                if (cpu.isOpcodeFetch()) {
                    ++result.opcodeCycles;
                    if (result.opcodeCycles > 2L * testImage.expectedCycles) {
                        result.message = "Test fail, takes too many cycles!";
                        break;
                    }
                    if (cpu.address() == testImage.successAddress) {
                        result.passed = true;
                        result.message = "OK";
                        break;
                    }
                }
            }

            long elapsedNanos = System.nanoTime() - startNanos;
            result.mhz = emulatedMhz(result.busTicks, elapsedNanos);
            return result;
        }
    }

    private static double emulatedMhz(long busTicks, long elapsedNanos) {
        if (elapsedNanos <= 0L) {
            return 0.0;
        }

        return ((double)busTicks / ((double)elapsedNanos / 1_000_000_000.0)) / 1_000_000.0;
    }

    private record ModelInfo(Model model, String displayName) {
    }

    private record TestImage(byte[] image, int successAddress, long expectedCycles) {
    }

    private record RomSet(byte[] standard, byte[] extended) {
    }

    private static final class RunResult {
        boolean passed;
        String message = "CPU Error";
        double mhz;
        long busTicks;
        long opcodeCycles;
    }
}
