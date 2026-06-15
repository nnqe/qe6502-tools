if(POLICY CMP0007)
    cmake_policy(SET CMP0007 NEW)
endif()

if(NOT DEFINED QE6502_JAVA_JAR_EXECUTABLE OR QE6502_JAVA_JAR_EXECUTABLE STREQUAL "")
    message(FATAL_ERROR "QE6502_JAVA_JAR_EXECUTABLE is required")
endif()

if(NOT DEFINED QE6502_JAVA_BASE_PACKAGE_DIR OR QE6502_JAVA_BASE_PACKAGE_DIR STREQUAL "")
    message(FATAL_ERROR "QE6502_JAVA_BASE_PACKAGE_DIR is required")
endif()

if(NOT DEFINED QE6502_JAVA_RUNTIME_ROOT OR QE6502_JAVA_RUNTIME_ROOT STREQUAL "")
    message(FATAL_ERROR "QE6502_JAVA_RUNTIME_ROOT is required")
endif()

if(NOT DEFINED QE6502_JAVA_AGGREGATE_DIR OR QE6502_JAVA_AGGREGATE_DIR STREQUAL "")
    message(FATAL_ERROR "QE6502_JAVA_AGGREGATE_DIR is required")
endif()

if(NOT DEFINED QE6502_JAVA_PACKAGE_VERSION OR QE6502_JAVA_PACKAGE_VERSION STREQUAL "")
    message(FATAL_ERROR "QE6502_JAVA_PACKAGE_VERSION is required")
endif()

set(base_jar "${QE6502_JAVA_BASE_PACKAGE_DIR}/qe6502-java.jar")
set(base_sources_jar "${QE6502_JAVA_BASE_PACKAGE_DIR}/qe6502-java-sources.jar")
set(base_javadoc_jar "${QE6502_JAVA_BASE_PACKAGE_DIR}/qe6502-java-javadoc.jar")
set(base_readme "${QE6502_JAVA_BASE_PACKAGE_DIR}/README.md")
set(base_license "${QE6502_JAVA_BASE_PACKAGE_DIR}/LICENSE")
set(base_pom "${QE6502_JAVA_BASE_PACKAGE_DIR}/pom.xml")

if(NOT EXISTS "${base_jar}")
    message(FATAL_ERROR "Base Java package jar not found: ${base_jar}")
endif()
if(NOT EXISTS "${base_sources_jar}")
    message(FATAL_ERROR "Base Java package sources jar not found: ${base_sources_jar}")
endif()
if(NOT EXISTS "${base_javadoc_jar}")
    message(FATAL_ERROR "Base Java package javadoc jar not found: ${base_javadoc_jar}")
endif()
if(NOT EXISTS "${base_readme}")
    message(FATAL_ERROR "Base Java package README not found: ${base_readme}")
endif()
if(NOT EXISTS "${base_license}")
    message(FATAL_ERROR "Base Java package LICENSE not found: ${base_license}")
endif()
if(NOT EXISTS "${base_pom}")
    message(FATAL_ERROR "Base Java package POM not found: ${base_pom}")
endif()
if(NOT IS_DIRECTORY "${QE6502_JAVA_RUNTIME_ROOT}/qe6502/native")
    message(FATAL_ERROR "Java runtime root must contain qe6502/native: ${QE6502_JAVA_RUNTIME_ROOT}")
endif()

set(output_package_dir "${QE6502_JAVA_AGGREGATE_DIR}/qe6502-java-${QE6502_JAVA_PACKAGE_VERSION}")
set(output_jar "${output_package_dir}/qe6502-java.jar")
set(output_file_list "${output_package_dir}/qe6502-java-${QE6502_JAVA_PACKAGE_VERSION}-files.txt")

file(REMOVE_RECURSE "${output_package_dir}")
file(MAKE_DIRECTORY "${output_package_dir}")
configure_file("${base_jar}" "${output_jar}" COPYONLY)
configure_file("${base_sources_jar}" "${output_package_dir}/qe6502-java-sources.jar" COPYONLY)
configure_file("${base_javadoc_jar}" "${output_package_dir}/qe6502-java-javadoc.jar" COPYONLY)
configure_file("${base_readme}" "${output_package_dir}/README.md" COPYONLY)
configure_file("${base_license}" "${output_package_dir}/LICENSE" COPYONLY)
configure_file("${base_pom}" "${output_package_dir}/pom.xml" COPYONLY)

execute_process(
    COMMAND "${QE6502_JAVA_JAR_EXECUTABLE}"
        --update
        --file "${output_jar}"
        -C "${QE6502_JAVA_RUNTIME_ROOT}" .
    RESULT_VARIABLE jar_update_result
)
if(NOT jar_update_result EQUAL 0)
    message(FATAL_ERROR "Failed to update Java package jar with multi-platform native runtime assets")
endif()

execute_process(
    COMMAND "${QE6502_JAVA_JAR_EXECUTABLE}" --list --file "${output_jar}"
    RESULT_VARIABLE jar_list_result
    OUTPUT_VARIABLE jar_list_output
)
if(NOT jar_list_result EQUAL 0)
    message(FATAL_ERROR "Failed to list Java package jar contents")
endif()

string(REPLACE "\r\n" "\n" jar_list_output "${jar_list_output}")
string(REPLACE "\r" "\n" jar_list_output "${jar_list_output}")
string(REPLACE "\n" ";" jar_entries "${jar_list_output}")

set(expected_entries
    qe6502/native/linux-x64/libqe6502.so
    qe6502/native/linux-arm64/libqe6502.so
    qe6502/native/osx-x64/libqe6502.dylib
    qe6502/native/osx-arm64/libqe6502.dylib
    qe6502/native/win-x64/libqe6502.dll
    qe6502/native/win-arm64/libqe6502.dll
)

set(missing_entries "")
foreach(expected_entry IN LISTS expected_entries)
    list(FIND jar_entries "${expected_entry}" found_index)
    if(found_index EQUAL -1)
        list(APPEND missing_entries "${expected_entry}")
    endif()
endforeach()

if(missing_entries)
    foreach(missing_entry IN LISTS missing_entries)
        message(SEND_ERROR "Missing Java package runtime asset: ${missing_entry}")
    endforeach()
    message(FATAL_ERROR "Java multi-platform package layout verification failed")
endif()

file(WRITE "${output_file_list}" "${jar_list_output}")
message(STATUS "Java multi-platform package staged at ${output_package_dir}")
message(STATUS "Java multi-platform package layout verified")
