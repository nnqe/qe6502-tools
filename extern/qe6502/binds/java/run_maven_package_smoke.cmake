if(NOT DEFINED QE6502_MAVEN_EXECUTABLE OR QE6502_MAVEN_EXECUTABLE STREQUAL "")
    message(FATAL_ERROR "QE6502_MAVEN_EXECUTABLE is required")
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

if(NOT DEFINED QE6502_JAVA_PACKAGE_POM OR QE6502_JAVA_PACKAGE_POM STREQUAL "")
    message(FATAL_ERROR "QE6502_JAVA_PACKAGE_POM is required")
endif()

if(NOT EXISTS "${QE6502_JAVA_PACKAGE_POM}")
    message(FATAL_ERROR "QE6502 Java package POM does not exist: ${QE6502_JAVA_PACKAGE_POM}")
endif()

if(NOT DEFINED QE6502_JAVA_MAVEN_SMOKE_DIR OR QE6502_JAVA_MAVEN_SMOKE_DIR STREQUAL "")
    message(FATAL_ERROR "QE6502_JAVA_MAVEN_SMOKE_DIR is required")
endif()

if(NOT DEFINED QE6502_JAVA_MAVEN_GROUP_ID OR QE6502_JAVA_MAVEN_GROUP_ID STREQUAL "")
    message(FATAL_ERROR "QE6502_JAVA_MAVEN_GROUP_ID is required")
endif()

if(NOT DEFINED QE6502_JAVA_MAVEN_ARTIFACT_ID OR QE6502_JAVA_MAVEN_ARTIFACT_ID STREQUAL "")
    message(FATAL_ERROR "QE6502_JAVA_MAVEN_ARTIFACT_ID is required")
endif()

if(NOT DEFINED QE6502_JAVA_MAVEN_VERSION OR QE6502_JAVA_MAVEN_VERSION STREQUAL "")
    message(FATAL_ERROR "QE6502_JAVA_MAVEN_VERSION is required")
endif()

file(REMOVE_RECURSE "${QE6502_JAVA_MAVEN_SMOKE_DIR}")
set(local_repo "${QE6502_JAVA_MAVEN_SMOKE_DIR}/local-repo")
set(consumer_dir "${QE6502_JAVA_MAVEN_SMOKE_DIR}/consumer")

string(REPLACE "." "/" group_path "${QE6502_JAVA_MAVEN_GROUP_ID}")
set(artifact_dir "${local_repo}/${group_path}/${QE6502_JAVA_MAVEN_ARTIFACT_ID}/${QE6502_JAVA_MAVEN_VERSION}")
set(local_repo_jar "${artifact_dir}/${QE6502_JAVA_MAVEN_ARTIFACT_ID}-${QE6502_JAVA_MAVEN_VERSION}.jar")
set(local_repo_pom "${artifact_dir}/${QE6502_JAVA_MAVEN_ARTIFACT_ID}-${QE6502_JAVA_MAVEN_VERSION}.pom")

file(MAKE_DIRECTORY "${artifact_dir}")
configure_file("${QE6502_JAVA_PACKAGE_JAR}" "${local_repo_jar}" COPYONLY)
configure_file("${QE6502_JAVA_PACKAGE_POM}" "${local_repo_pom}" COPYONLY)

file(MAKE_DIRECTORY "${consumer_dir}/src/main/java/qe6502/maven_smoke")

file(WRITE "${consumer_dir}/pom.xml" [=[
<?xml version="1.0" encoding="UTF-8"?>
<project xmlns="http://maven.apache.org/POM/4.0.0"
         xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
         xsi:schemaLocation="http://maven.apache.org/POM/4.0.0 https://maven.apache.org/xsd/maven-4.0.0.xsd">
  <modelVersion>4.0.0</modelVersion>

  <groupId>qe6502.maven-smoke</groupId>
  <artifactId>qe6502-maven-smoke</artifactId>
  <version>1.0.0</version>

  <properties>
    <project.build.sourceEncoding>UTF-8</project.build.sourceEncoding>
    <maven.compiler.release>25</maven.compiler.release>
  </properties>

  <dependencies>
    <dependency>
      <groupId>__QE6502_JAVA_MAVEN_GROUP_ID__</groupId>
      <artifactId>__QE6502_JAVA_MAVEN_ARTIFACT_ID__</artifactId>
      <version>__QE6502_JAVA_MAVEN_VERSION__</version>
    </dependency>
  </dependencies>
</project>
]=])

file(READ "${consumer_dir}/pom.xml" consumer_pom)
string(REPLACE "__QE6502_JAVA_MAVEN_GROUP_ID__" "${QE6502_JAVA_MAVEN_GROUP_ID}" consumer_pom "${consumer_pom}")
string(REPLACE "__QE6502_JAVA_MAVEN_ARTIFACT_ID__" "${QE6502_JAVA_MAVEN_ARTIFACT_ID}" consumer_pom "${consumer_pom}")
string(REPLACE "__QE6502_JAVA_MAVEN_VERSION__" "${QE6502_JAVA_MAVEN_VERSION}" consumer_pom "${consumer_pom}")
file(WRITE "${consumer_dir}/pom.xml" "${consumer_pom}")

file(WRITE "${consumer_dir}/src/main/java/qe6502/maven_smoke/Main.java" [=[
package qe6502.maven_smoke;

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

        System.out.println("qe6502 Java Maven consumer smoke OK");
    }
}
]=])

execute_process(
    COMMAND "${QE6502_MAVEN_EXECUTABLE}"
        -q
        "-Dmaven.repo.local=${local_repo}"
        package
    WORKING_DIRECTORY "${consumer_dir}"
    RESULT_VARIABLE maven_result
    OUTPUT_VARIABLE maven_stdout
    ERROR_VARIABLE maven_stderr
)
if(NOT maven_result EQUAL 0)
    message(FATAL_ERROR "Java Maven package consumer build failed (${maven_result})\nrepo: ${local_repo}\nstdout:\n${maven_stdout}\nstderr:\n${maven_stderr}")
endif()

if(CMAKE_HOST_WIN32)
    set(classpath "${consumer_dir}/target/classes;${local_repo_jar}")
else()
    set(classpath "${consumer_dir}/target/classes:${local_repo_jar}")
endif()

execute_process(
    COMMAND "${QE6502_JAVA_JAVA_EXECUTABLE}"
        --enable-native-access=ALL-UNNAMED
        -cp "${classpath}"
        qe6502.maven_smoke.Main
    WORKING_DIRECTORY "${consumer_dir}"
    RESULT_VARIABLE smoke_result
    OUTPUT_VARIABLE smoke_stdout
    ERROR_VARIABLE smoke_stderr
)
if(NOT smoke_result EQUAL 0)
    message(FATAL_ERROR "Java Maven package consumer smoke failed (${smoke_result})\nrepo jar: ${local_repo_jar}\nstdout:\n${smoke_stdout}\nstderr:\n${smoke_stderr}")
endif()

message(STATUS "Java Maven package consumer smoke passed for ${QE6502_JAVA_MAVEN_GROUP_ID}:${QE6502_JAVA_MAVEN_ARTIFACT_ID}:${QE6502_JAVA_MAVEN_VERSION}")
message(STATUS "${smoke_stdout}")
