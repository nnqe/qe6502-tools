package qe6502;

import java.io.IOException;
import java.io.InputStream;
import java.lang.foreign.Arena;
import java.lang.foreign.FunctionDescriptor;
import java.lang.foreign.Linker;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.SymbolLookup;
import java.lang.invoke.MethodHandle;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;

import static java.lang.foreign.ValueLayout.ADDRESS;
import static java.lang.foreign.ValueLayout.JAVA_BYTE;
import static java.lang.foreign.ValueLayout.JAVA_INT;

final class NativeLibrary implements AutoCloseable {
    static final int CONTEXT_SIZE = 64;
    static final int CONTEXT_ALIGN = 8;
    static final int SNAPSHOT_SIZE = 64;

    private static final String NATIVE_PATH_PROPERTY = "qe6502.native.path";

    private final Arena arena;
    private final MethodHandle version;
    private final MethodHandle setup;
    private final MethodHandle restart;
    private final MethodHandle tick;
    private final MethodHandle goTo;
    private final MethodHandle nmiAssert;
    private final MethodHandle irqAssert;
    private final MethodHandle isNmiAsserted;
    private final MethodHandle isIrqAsserted;
    private final MethodHandle save;
    private final MethodHandle load;
    private final MethodHandle getPc;
    private final MethodHandle setPc;
    private final MethodHandle getS;
    private final MethodHandle setS;
    private final MethodHandle getA;
    private final MethodHandle setA;
    private final MethodHandle getX;
    private final MethodHandle setX;
    private final MethodHandle getY;
    private final MethodHandle setY;
    private final MethodHandle getP;
    private final MethodHandle setP;
    private final MethodHandle getModel;
    private final MethodHandle setModel;

    NativeLibrary() {
        arena = Arena.ofShared();

        Linker linker = Linker.nativeLinker();
        SymbolLookup lookup = openLookup(arena);

        version = downcall(linker, lookup, "qe6502abi_version", FunctionDescriptor.of(JAVA_INT));
        setup = downcall(linker, lookup, "qe6502abi_setup", FunctionDescriptor.ofVoid(ADDRESS, JAVA_INT));
        restart = downcall(linker, lookup, "qe6502abi_restart", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        tick = downcall(linker, lookup, "qe6502abi_tick", FunctionDescriptor.of(JAVA_INT, ADDRESS, JAVA_INT));
        goTo = downcall(linker, lookup, "qe6502abi_goto", FunctionDescriptor.of(JAVA_INT, ADDRESS, JAVA_INT));
        nmiAssert = downcall(linker, lookup, "qe6502abi_nmi_assert", FunctionDescriptor.ofVoid(ADDRESS, JAVA_BYTE));
        irqAssert = downcall(linker, lookup, "qe6502abi_irq_assert", FunctionDescriptor.ofVoid(ADDRESS, JAVA_BYTE));
        isNmiAsserted = downcall(linker, lookup, "qe6502abi_is_nmi_asserted", FunctionDescriptor.of(JAVA_BYTE, ADDRESS));
        isIrqAsserted = downcall(linker, lookup, "qe6502abi_is_irq_asserted", FunctionDescriptor.of(JAVA_BYTE, ADDRESS));
        save = downcall(linker, lookup, "qe6502abi_save", FunctionDescriptor.ofVoid(ADDRESS, JAVA_INT, ADDRESS));
        load = downcall(linker, lookup, "qe6502abi_load", FunctionDescriptor.of(JAVA_INT, ADDRESS, ADDRESS));
        getPc = downcall(linker, lookup, "qe6502abi_get_pc", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        setPc = downcall(linker, lookup, "qe6502abi_set_pc", FunctionDescriptor.ofVoid(ADDRESS, JAVA_INT));
        getS = downcall(linker, lookup, "qe6502abi_get_s", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        setS = downcall(linker, lookup, "qe6502abi_set_s", FunctionDescriptor.ofVoid(ADDRESS, JAVA_INT));
        getA = downcall(linker, lookup, "qe6502abi_get_a", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        setA = downcall(linker, lookup, "qe6502abi_set_a", FunctionDescriptor.ofVoid(ADDRESS, JAVA_INT));
        getX = downcall(linker, lookup, "qe6502abi_get_x", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        setX = downcall(linker, lookup, "qe6502abi_set_x", FunctionDescriptor.ofVoid(ADDRESS, JAVA_INT));
        getY = downcall(linker, lookup, "qe6502abi_get_y", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        setY = downcall(linker, lookup, "qe6502abi_set_y", FunctionDescriptor.ofVoid(ADDRESS, JAVA_INT));
        getP = downcall(linker, lookup, "qe6502abi_get_p", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        setP = downcall(linker, lookup, "qe6502abi_set_p", FunctionDescriptor.ofVoid(ADDRESS, JAVA_INT));
        getModel = downcall(linker, lookup, "qe6502abi_get_model", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        setModel = downcall(linker, lookup, "qe6502abi_set_model", FunctionDescriptor.ofVoid(ADDRESS, JAVA_INT));
    }

    MemorySegment allocateContext(Arena contextArena) {
        return contextArena.allocate(CONTEXT_SIZE, CONTEXT_ALIGN);
    }

    int version() {
        try {
            return (int)version.invokeExact();
        } catch (Throwable throwable) {
            throw new IllegalStateException("Failed to call qe6502abi_version.", throwable);
        }
    }

    void setup(MemorySegment ctx, int model) {
        try {
            setup.invokeExact(ctx, model);
        } catch (Throwable throwable) {
            throw new IllegalStateException("Failed to call qe6502abi_setup.", throwable);
        }
    }

    int restart(MemorySegment ctx) {
        try {
            return (int)restart.invokeExact(ctx);
        } catch (Throwable throwable) {
            throw new IllegalStateException("Failed to call qe6502abi_restart.", throwable);
        }
    }

    int tick(MemorySegment ctx, int bus) {
        try {
            return (int)tick.invokeExact(ctx, bus);
        } catch (Throwable throwable) {
            throw new IllegalStateException("Failed to call qe6502abi_tick.", throwable);
        }
    }

    int goTo(MemorySegment ctx, int address) {
        try {
            return (int)goTo.invokeExact(ctx, address);
        } catch (Throwable throwable) {
            throw new IllegalStateException("Failed to call qe6502abi_goto.", throwable);
        }
    }

    void nmiAssert(MemorySegment ctx, boolean asserted) {
        try {
            nmiAssert.invokeExact(ctx, (byte)(asserted ? 1 : 0));
        } catch (Throwable throwable) {
            throw new IllegalStateException("Failed to call qe6502abi_nmi_assert.", throwable);
        }
    }

    void irqAssert(MemorySegment ctx, boolean asserted) {
        try {
            irqAssert.invokeExact(ctx, (byte)(asserted ? 1 : 0));
        } catch (Throwable throwable) {
            throw new IllegalStateException("Failed to call qe6502abi_irq_assert.", throwable);
        }
    }

    boolean isNmiAsserted(MemorySegment ctx) {
        try {
            return ((byte)isNmiAsserted.invokeExact(ctx)) != 0;
        } catch (Throwable throwable) {
            throw new IllegalStateException("Failed to call qe6502abi_is_nmi_asserted.", throwable);
        }
    }

    boolean isIrqAsserted(MemorySegment ctx) {
        try {
            return ((byte)isIrqAsserted.invokeExact(ctx)) != 0;
        } catch (Throwable throwable) {
            throw new IllegalStateException("Failed to call qe6502abi_is_irq_asserted.", throwable);
        }
    }

    byte[] save(MemorySegment ctx, int rawTick) {
        try (Arena snapshotArena = Arena.ofConfined()) {
            MemorySegment snapshot = snapshotArena.allocate(SNAPSHOT_SIZE, 1);
            save.invokeExact(ctx, rawTick, snapshot);
            byte[] data = new byte[SNAPSHOT_SIZE];
            for (int i = 0; i < data.length; ++i) {
                data[i] = snapshot.get(JAVA_BYTE, i);
            }
            return data;
        } catch (Throwable throwable) {
            throw new IllegalStateException("Failed to call qe6502abi_save.", throwable);
        }
    }

    int load(MemorySegment ctx, byte[] data) {
        try (Arena snapshotArena = Arena.ofConfined()) {
            MemorySegment snapshot = snapshotArena.allocate(SNAPSHOT_SIZE, 1);
            for (int i = 0; i < data.length; ++i) {
                snapshot.set(JAVA_BYTE, i, data[i]);
            }
            return (int)load.invokeExact(ctx, snapshot);
        } catch (Throwable throwable) {
            throw new IllegalStateException("Failed to call qe6502abi_load.", throwable);
        }
    }

    int getPc(MemorySegment ctx) { return getInt(getPc, ctx, "qe6502abi_get_pc"); }
    void setPc(MemorySegment ctx, int value) { setInt(setPc, ctx, value, "qe6502abi_set_pc"); }
    int getS(MemorySegment ctx) { return getInt(getS, ctx, "qe6502abi_get_s"); }
    void setS(MemorySegment ctx, int value) { setInt(setS, ctx, value, "qe6502abi_set_s"); }
    int getA(MemorySegment ctx) { return getInt(getA, ctx, "qe6502abi_get_a"); }
    void setA(MemorySegment ctx, int value) { setInt(setA, ctx, value, "qe6502abi_set_a"); }
    int getX(MemorySegment ctx) { return getInt(getX, ctx, "qe6502abi_get_x"); }
    void setX(MemorySegment ctx, int value) { setInt(setX, ctx, value, "qe6502abi_set_x"); }
    int getY(MemorySegment ctx) { return getInt(getY, ctx, "qe6502abi_get_y"); }
    void setY(MemorySegment ctx, int value) { setInt(setY, ctx, value, "qe6502abi_set_y"); }
    int getP(MemorySegment ctx) { return getInt(getP, ctx, "qe6502abi_get_p"); }
    void setP(MemorySegment ctx, int value) { setInt(setP, ctx, value, "qe6502abi_set_p"); }
    int getModel(MemorySegment ctx) { return getInt(getModel, ctx, "qe6502abi_get_model"); }
    void setModel(MemorySegment ctx, int value) { setInt(setModel, ctx, value, "qe6502abi_set_model"); }

    @Override
    public void close() {
        arena.close();
    }

    private static SymbolLookup openLookup(Arena arena) {
        String nativePath = System.getProperty(NATIVE_PATH_PROPERTY);
        if (nativePath != null && !nativePath.isBlank()) {
            try {
                return SymbolLookup.libraryLookup(Path.of(nativePath), arena);
            } catch (RuntimeException | Error failure) {
                throw new UnsatisfiedLinkError(
                    "Failed to load qe6502 native library from -D" + NATIVE_PATH_PROPERTY + "=" + nativePath
                    + ": " + failure.getMessage()
                );
            }
        }

        StringBuilder diagnostics = new StringBuilder();

        Path bundledLibrary = extractBundledLibrary(diagnostics);
        if (bundledLibrary != null) {
            try {
                return SymbolLookup.libraryLookup(bundledLibrary, arena);
            } catch (RuntimeException | Error failure) {
                appendDiagnostic(diagnostics, "bundled native library could not be loaded from "
                    + bundledLibrary + ": " + failure.getMessage());
            }
        }

        Path localLibrary = Path.of(platformLibraryFileName()).toAbsolutePath();
        if (Files.isRegularFile(localLibrary)) {
            try {
                return SymbolLookup.libraryLookup(localLibrary, arena);
            } catch (RuntimeException | Error failure) {
                appendDiagnostic(diagnostics, "local native library could not be loaded from "
                    + localLibrary + ": " + failure.getMessage());
            }
        } else {
            appendDiagnostic(diagnostics, "local native library not found at " + localLibrary);
        }

        try {
            return SymbolLookup.libraryLookup(defaultLibraryName(), arena);
        } catch (RuntimeException | Error failure) {
            appendDiagnostic(diagnostics, "system library lookup for " + defaultLibraryName()
                + " failed: " + failure.getMessage());
        }

        throw new UnsatisfiedLinkError("Unable to load qe6502 native library. Tried -D"
            + NATIVE_PATH_PROPERTY + ", bundled jar resource, local file, and system lookup."
            + diagnostics);
    }

    private static Path extractBundledLibrary(StringBuilder diagnostics) {
        PlatformInfo platform = detectPlatform();
        if (!platform.supported()) {
            appendDiagnostic(diagnostics, "unsupported platform for bundled native library: os.name=\""
                + platform.osName() + "\", os.arch=\"" + platform.archName() + "\"");
            return null;
        }

        String resourceName = "/qe6502/native/" + platform.resourceDirectory() + "/" + platformLibraryFileName();
        try (InputStream input = NativeLibrary.class.getResourceAsStream(resourceName)) {
            if (input == null) {
                appendDiagnostic(diagnostics, "bundled native library resource not found: " + resourceName);
                return null;
            }

            Path directory = Files.createTempDirectory(
                "qe6502-java-native-abi" + Integer.toHexString(NativeVersion.COMPILED_ABI_VERSION)
                    + "-" + platform.resourceDirectory() + "-"
            );
            Path extractedLibrary = directory.resolve(platformLibraryFileName());
            Files.copy(input, extractedLibrary, StandardCopyOption.REPLACE_EXISTING);
            directory.toFile().deleteOnExit();
            extractedLibrary.toFile().deleteOnExit();
            return extractedLibrary;
        } catch (IOException exception) {
            throw new UnsatisfiedLinkError("Failed to extract bundled qe6502 native library resource "
                + resourceName + ": " + exception.getMessage());
        }
    }

    private static PlatformInfo detectPlatform() {
        String osName = System.getProperty("os.name", "").toLowerCase();
        String archName = System.getProperty("os.arch", "").toLowerCase();

        String os;
        if (osName.contains("win")) {
            os = "win";
        } else if (osName.contains("mac") || osName.contains("darwin")) {
            os = "osx";
        } else if (osName.contains("linux")) {
            os = "linux";
        } else {
            os = null;
        }

        String arch;
        if (archName.equals("x86_64") || archName.equals("amd64")) {
            arch = "x64";
        } else if (archName.equals("aarch64") || archName.equals("arm64")) {
            arch = "arm64";
        } else {
            arch = null;
        }

        String resourceDirectory = (os == null || arch == null) ? null : os + "-" + arch;
        return new PlatformInfo(osName, archName, resourceDirectory);
    }

    private static void appendDiagnostic(StringBuilder diagnostics, String message) {
        diagnostics.append(System.lineSeparator()).append("  - ").append(message);
    }

    private record PlatformInfo(String osName, String archName, String resourceDirectory) {
        boolean supported() {
            return resourceDirectory != null;
        }
    }


    private static String platformLibraryFileName() {
        String osName = System.getProperty("os.name", "").toLowerCase();
        if (osName.contains("win")) {
            return "libqe6502.dll";
        }
        if (osName.contains("mac") || osName.contains("darwin")) {
            return "libqe6502.dylib";
        }
        return "libqe6502.so";
    }

    private static String defaultLibraryName() {
        String osName = System.getProperty("os.name", "").toLowerCase();
        if (osName.contains("win")) {
            return "libqe6502";
        }
        return "qe6502";
    }

    private static MethodHandle downcall(Linker linker, SymbolLookup lookup, String name, FunctionDescriptor descriptor) {
        MemorySegment address = lookup.find(name)
            .orElseThrow(() -> new UnsatisfiedLinkError("Native symbol not found: " + name));
        return linker.downcallHandle(address, descriptor);
    }

    private static int getInt(MethodHandle handle, MemorySegment ctx, String name) {
        try {
            return (int)handle.invokeExact(ctx);
        } catch (Throwable throwable) {
            throw new IllegalStateException("Failed to call " + name + ".", throwable);
        }
    }

    private static void setInt(MethodHandle handle, MemorySegment ctx, int value, String name) {
        try {
            handle.invokeExact(ctx, value);
        } catch (Throwable throwable) {
            throw new IllegalStateException("Failed to call " + name + ".", throwable);
        }
    }
}
