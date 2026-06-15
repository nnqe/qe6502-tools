if(NOT DEFINED QE6502_JAVA_JAVAC_EXECUTABLE OR QE6502_JAVA_JAVAC_EXECUTABLE STREQUAL "")
    message(FATAL_ERROR "QE6502_JAVA_JAVAC_EXECUTABLE is required")
endif()

if(NOT DEFINED QE6502_JAVA_JAVA_EXECUTABLE OR QE6502_JAVA_JAVA_EXECUTABLE STREQUAL "")
    message(FATAL_ERROR "QE6502_JAVA_JAVA_EXECUTABLE is required")
endif()

if(NOT DEFINED QE6502_JAVA_PACKAGE_JAR OR QE6502_JAVA_PACKAGE_JAR STREQUAL "")
    message(FATAL_ERROR "QE6502_JAVA_PACKAGE_JAR is required")
endif()

if(NOT EXISTS "${QE6502_JAVA_PACKAGE_JAR}")
    message(FATAL_ERROR "QE6502 Java package jar does not exist: ${QE6502_JAVA_PACKAGE_JAR}")
endif()

if(NOT DEFINED QE6502_JAVA_PACKAGE_SMOKE_DIR OR QE6502_JAVA_PACKAGE_SMOKE_DIR STREQUAL "")
    message(FATAL_ERROR "QE6502_JAVA_PACKAGE_SMOKE_DIR is required")
endif()

file(REMOVE_RECURSE "${QE6502_JAVA_PACKAGE_SMOKE_DIR}")
file(MAKE_DIRECTORY "${QE6502_JAVA_PACKAGE_SMOKE_DIR}/src/qe6502/package_smoke")
file(MAKE_DIRECTORY "${QE6502_JAVA_PACKAGE_SMOKE_DIR}/classes")

set(smoke_source "${QE6502_JAVA_PACKAGE_SMOKE_DIR}/src/qe6502/package_smoke/Main.java")
file(WRITE "${smoke_source}" [=[
package qe6502.package_smoke;

import qe6502.Cpu;
import qe6502.Model;

public final class Main {
    private Main() {
    }

    public static void main(String[] args) {
        byte[] memory = new byte[65536];
        memory[0x8000] = (byte)0xEE; // INC $0200
        memory[0x8001] = 0x00;
        memory[0x8002] = 0x02;
        memory[0x8003] = 0x4C; // JMP $8000
        memory[0x8004] = 0x00;
        memory[0x8005] = (byte)0x80;

        try (Cpu cpu = new Cpu(Model.NMOS)) {
            cpu.jumpTo(0x8000);
            for (int i = 0; i < 64; ++i) {
                if (cpu.isWrite()) {
                    memory[cpu.address()] = (byte)cpu.data();
                    cpu.tick();
                } else {
                    cpu.tick(memory[cpu.address()] & 0xff);
                }
            }

            if ((memory[0x0200] & 0xff) == 0) {
                throw new AssertionError("program did not update memory");
            }

            cpu.a(0x42);
            byte[] snapshot = cpu.save();
            cpu.a(0x00);
            cpu.load(snapshot);
            if (cpu.a() != 0x42) {
                throw new AssertionError("snapshot restore failed");
            }
        }

        System.out.println("qe6502 Java package consumer smoke OK");
    }
}
]=])

execute_process(
    COMMAND "${QE6502_JAVA_JAVAC_EXECUTABLE}"
        --release 25
        -cp "${QE6502_JAVA_PACKAGE_JAR}"
        -d "${QE6502_JAVA_PACKAGE_SMOKE_DIR}/classes"
        "${smoke_source}"
    RESULT_VARIABLE compile_result
    OUTPUT_VARIABLE compile_stdout
    ERROR_VARIABLE compile_stderr
)
if(NOT compile_result EQUAL 0)
    message(FATAL_ERROR "Java package consumer compile failed (${compile_result})\njar: ${QE6502_JAVA_PACKAGE_JAR}\nstdout:\n${compile_stdout}\nstderr:\n${compile_stderr}")
endif()

if(CMAKE_HOST_WIN32)
    set(classpath "${QE6502_JAVA_PACKAGE_JAR};${QE6502_JAVA_PACKAGE_SMOKE_DIR}/classes")
else()
    set(classpath "${QE6502_JAVA_PACKAGE_JAR}:${QE6502_JAVA_PACKAGE_SMOKE_DIR}/classes")
endif()

execute_process(
    COMMAND "${QE6502_JAVA_JAVA_EXECUTABLE}"
        --enable-native-access=ALL-UNNAMED
        -cp "${classpath}"
        qe6502.package_smoke.Main
    WORKING_DIRECTORY "${QE6502_JAVA_PACKAGE_SMOKE_DIR}"
    RESULT_VARIABLE smoke_result
    OUTPUT_VARIABLE smoke_stdout
    ERROR_VARIABLE smoke_stderr
)
if(NOT smoke_result EQUAL 0)
    message(FATAL_ERROR "Java package consumer smoke failed (${smoke_result})\njar: ${QE6502_JAVA_PACKAGE_JAR}\nstdout:\n${smoke_stdout}\nstderr:\n${smoke_stderr}")
endif()

message(STATUS "Java package consumer smoke passed for ${QE6502_JAVA_PACKAGE_JAR}")
message(STATUS "${smoke_stdout}")
