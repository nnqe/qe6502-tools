include(Qe6502FindJava25)

if(NOT QE6502_BUILD_JAVA)
    message(STATUS "QE6502_BUILD_JAVA is OFF; skipping qe6502 Java Klaus2m5 harness")
    return()
endif()

qe6502_find_java25()

if(NOT Java_FOUND)
    message(STATUS "Java 25+ development tools not found; skipping qe6502 Java Klaus2m5 harness")
    return()
endif()

if(NOT TARGET qe6502_java)
    message(STATUS "qe6502_java target is not available; skipping qe6502 Java Klaus2m5 harness")
    return()
endif()

set(QE6502_JAVA_KLAUS2M5_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(QE6502_JAVA_KLAUS2M5_CLASSES_DIR ${CMAKE_CURRENT_BINARY_DIR}/classes)
set(QE6502_JAVA_KLAUS2M5_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/dist)
set(QE6502_JAVA_KLAUS2M5_ROM_OUTPUT_DIR ${QE6502_JAVA_KLAUS2M5_OUTPUT_DIR}/klaus2m5)
set(QE6502_JAVA_KLAUS2M5_JAR ${QE6502_JAVA_KLAUS2M5_OUTPUT_DIR}/qe6502-java-klaus2m5.jar)
set(QE6502_JAVA_KLAUS2M5_MANIFEST ${CMAKE_CURRENT_SOURCE_DIR}/MANIFEST.MF)

file(GLOB_RECURSE QE6502_JAVA_KLAUS2M5_SOURCES CONFIGURE_DEPENDS
    ${QE6502_JAVA_KLAUS2M5_SOURCE_DIR}/*.java
)

if(WIN32)
    set(QE6502_JAVA_KLAUS2M5_NATIVE_LIBRARY_NAME "$<TARGET_FILE_NAME:qe6502_shared>")
else()
    set(QE6502_JAVA_KLAUS2M5_NATIVE_LIBRARY_NAME "$<TARGET_LINKER_FILE_NAME:qe6502_shared>")
endif()

set(QE6502_JAVA_KLAUS2M5_NATIVE_LIBRARY ${QE6502_JAVA_KLAUS2M5_OUTPUT_DIR}/${QE6502_JAVA_KLAUS2M5_NATIVE_LIBRARY_NAME})
set(QE6502_JAVA_KLAUS2M5_STANDARD_ROM ${CMAKE_SOURCE_DIR}/harness/klaus2m5/6502_functional_test.hex)
set(QE6502_JAVA_KLAUS2M5_EXTENDED_ROM ${CMAKE_SOURCE_DIR}/harness/klaus2m5/65C02_extended_opcodes_test.hex)

add_custom_command(
    OUTPUT ${QE6502_JAVA_KLAUS2M5_JAR}
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${QE6502_JAVA_KLAUS2M5_CLASSES_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QE6502_JAVA_KLAUS2M5_CLASSES_DIR}
    COMMAND ${Java_JAVAC_EXECUTABLE}
        --release 25
        -cp "${QE6502_JAVA_JAR}"
        -d ${QE6502_JAVA_KLAUS2M5_CLASSES_DIR}
        ${QE6502_JAVA_KLAUS2M5_SOURCES}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QE6502_JAVA_KLAUS2M5_OUTPUT_DIR}
    COMMAND ${Java_JAR_EXECUTABLE}
        --create
        --file ${QE6502_JAVA_KLAUS2M5_JAR}
        --manifest ${QE6502_JAVA_KLAUS2M5_MANIFEST}
        -C ${QE6502_JAVA_KLAUS2M5_CLASSES_DIR} .
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${QE6502_JAVA_JAR}
        ${QE6502_JAVA_KLAUS2M5_OUTPUT_DIR}/qe6502-java.jar
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:qe6502_shared>
        ${QE6502_JAVA_KLAUS2M5_NATIVE_LIBRARY}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QE6502_JAVA_KLAUS2M5_ROM_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${QE6502_JAVA_KLAUS2M5_STANDARD_ROM}
        ${QE6502_JAVA_KLAUS2M5_ROM_OUTPUT_DIR}/6502_functional_test.hex
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${QE6502_JAVA_KLAUS2M5_EXTENDED_ROM}
        ${QE6502_JAVA_KLAUS2M5_ROM_OUTPUT_DIR}/65C02_extended_opcodes_test.hex
    DEPENDS
        qe6502_java
        ${QE6502_JAVA_KLAUS2M5_SOURCES}
        ${QE6502_JAVA_KLAUS2M5_MANIFEST}
        ${QE6502_JAVA_KLAUS2M5_STANDARD_ROM}
        ${QE6502_JAVA_KLAUS2M5_EXTENDED_ROM}
    COMMENT "Building qe6502 Java Klaus2m5 harness"
    VERBATIM
)

add_custom_target(qe6502_java_klaus2m5 ALL
    DEPENDS ${QE6502_JAVA_KLAUS2M5_JAR}
)

add_custom_target(qe6502_java_klaus2m5_run
    COMMAND ${Java_JAVA_EXECUTABLE}
        --enable-native-access=ALL-UNNAMED
        -jar ${QE6502_JAVA_KLAUS2M5_JAR}
    DEPENDS qe6502_java_klaus2m5
    WORKING_DIRECTORY ${QE6502_JAVA_KLAUS2M5_OUTPUT_DIR}
    COMMENT "Running qe6502 Java Klaus2m5 harness"
    VERBATIM
)

set(QE6502_JAVA_KLAUS2M5_TESTS
    nmos standard
    wdc standard
    wdc extended
    rw standard
    rw extended
    st standard
)

list(LENGTH QE6502_JAVA_KLAUS2M5_TESTS QE6502_JAVA_KLAUS2M5_TEST_COUNT)
math(EXPR QE6502_JAVA_KLAUS2M5_LAST_INDEX "${QE6502_JAVA_KLAUS2M5_TEST_COUNT} - 1")

foreach(index RANGE 0 ${QE6502_JAVA_KLAUS2M5_LAST_INDEX} 2)
    list(GET QE6502_JAVA_KLAUS2M5_TESTS ${index} model)
    math(EXPR test_index "${index} + 1")
    list(GET QE6502_JAVA_KLAUS2M5_TESTS ${test_index} test_name)

    add_test(NAME qe6502.java.klaus2m5.${model}.${test_name}
        COMMAND ${Java_JAVA_EXECUTABLE}
            --enable-native-access=ALL-UNNAMED
            -jar ${QE6502_JAVA_KLAUS2M5_JAR} ${model} ${test_name}
        WORKING_DIRECTORY ${QE6502_JAVA_KLAUS2M5_OUTPUT_DIR}
    )

    set_tests_properties(qe6502.java.klaus2m5.${model}.${test_name} PROPERTIES
        LABELS "java;v2;klaus;functional;performance"
        TIMEOUT 180
    )
endforeach()
