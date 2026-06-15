include(Qe6502FindJava25)

if(NOT QE6502_BUILD_JAVA)
    message(STATUS "QE6502_BUILD_JAVA is OFF; skipping qe6502 Java smoke harness")
    return()
endif()

qe6502_find_java25()

if(NOT Java_FOUND)
    message(STATUS "Java 25+ development tools not found; skipping qe6502 Java smoke harness")
    return()
endif()

if(NOT TARGET qe6502_java)
    message(STATUS "qe6502_java target is not available; skipping qe6502 Java smoke harness")
    return()
endif()

set(QE6502_JAVA_SMOKE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(QE6502_JAVA_SMOKE_CLASSES_DIR ${CMAKE_CURRENT_BINARY_DIR}/classes)
set(QE6502_JAVA_SMOKE_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/dist)
set(QE6502_JAVA_SMOKE_JAR ${QE6502_JAVA_SMOKE_OUTPUT_DIR}/qe6502-java-smoke.jar)
set(QE6502_JAVA_SMOKE_MANIFEST ${CMAKE_CURRENT_SOURCE_DIR}/MANIFEST.MF)

file(GLOB_RECURSE QE6502_JAVA_SMOKE_SOURCES CONFIGURE_DEPENDS
    ${QE6502_JAVA_SMOKE_SOURCE_DIR}/*.java
)

if(WIN32)
    set(QE6502_JAVA_SMOKE_NATIVE_LIBRARY_NAME "$<TARGET_FILE_NAME:qe6502_shared>")
else()
    set(QE6502_JAVA_SMOKE_NATIVE_LIBRARY_NAME "$<TARGET_LINKER_FILE_NAME:qe6502_shared>")
endif()

set(QE6502_JAVA_SMOKE_NATIVE_LIBRARY ${QE6502_JAVA_SMOKE_OUTPUT_DIR}/${QE6502_JAVA_SMOKE_NATIVE_LIBRARY_NAME})

add_custom_command(
    OUTPUT ${QE6502_JAVA_SMOKE_JAR}
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${QE6502_JAVA_SMOKE_CLASSES_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QE6502_JAVA_SMOKE_CLASSES_DIR}
    COMMAND ${Java_JAVAC_EXECUTABLE}
        --release 25
        -cp "${QE6502_JAVA_JAR}"
        -d ${QE6502_JAVA_SMOKE_CLASSES_DIR}
        ${QE6502_JAVA_SMOKE_SOURCES}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QE6502_JAVA_SMOKE_OUTPUT_DIR}
    COMMAND ${Java_JAR_EXECUTABLE}
        --create
        --file ${QE6502_JAVA_SMOKE_JAR}
        --manifest ${QE6502_JAVA_SMOKE_MANIFEST}
        -C ${QE6502_JAVA_SMOKE_CLASSES_DIR} .
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${QE6502_JAVA_JAR}
        ${QE6502_JAVA_SMOKE_OUTPUT_DIR}/qe6502-java.jar
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:qe6502_shared>
        ${QE6502_JAVA_SMOKE_NATIVE_LIBRARY}
    DEPENDS
        qe6502_java
        ${QE6502_JAVA_SMOKE_SOURCES}
        ${QE6502_JAVA_SMOKE_MANIFEST}
    COMMENT "Building qe6502 Java smoke harness"
    VERBATIM
)

add_custom_target(qe6502_java_smoke ALL
    DEPENDS ${QE6502_JAVA_SMOKE_JAR}
)

add_custom_target(qe6502_java_smoke_run
    COMMAND ${Java_JAVA_EXECUTABLE}
        --enable-native-access=ALL-UNNAMED
        -jar ${QE6502_JAVA_SMOKE_JAR}
    DEPENDS qe6502_java_smoke
    WORKING_DIRECTORY ${QE6502_JAVA_SMOKE_OUTPUT_DIR}
    COMMENT "Running qe6502 Java smoke harness"
    VERBATIM
)

add_test(NAME qe6502_java_smoke
    COMMAND ${Java_JAVA_EXECUTABLE}
        --enable-native-access=ALL-UNNAMED
        -jar ${QE6502_JAVA_SMOKE_JAR}
    WORKING_DIRECTORY ${QE6502_JAVA_SMOKE_OUTPUT_DIR}
)

set_tests_properties(qe6502_java_smoke PROPERTIES
    LABELS "java;smoke;functional"
    TIMEOUT 30
)
