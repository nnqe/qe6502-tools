if(NOT DEFINED QE6502_GPG_EXECUTABLE OR QE6502_GPG_EXECUTABLE STREQUAL "")
    message(FATAL_ERROR "QE6502_GPG_EXECUTABLE is required")
endif()

if(NOT DEFINED QE6502_JAVA_MAVEN_PUBLISH_DIR OR QE6502_JAVA_MAVEN_PUBLISH_DIR STREQUAL "")
    message(FATAL_ERROR "QE6502_JAVA_MAVEN_PUBLISH_DIR is required")
endif()

if(NOT IS_DIRECTORY "${QE6502_JAVA_MAVEN_PUBLISH_DIR}")
    message(FATAL_ERROR "Java Maven publish directory does not exist: ${QE6502_JAVA_MAVEN_PUBLISH_DIR}")
endif()

file(GLOB_RECURSE publish_files
    "${QE6502_JAVA_MAVEN_PUBLISH_DIR}/*.jar"
    "${QE6502_JAVA_MAVEN_PUBLISH_DIR}/*.pom"
)
list(SORT publish_files)

if(NOT publish_files)
    message(FATAL_ERROR "No Java Maven artifacts found to sign under ${QE6502_JAVA_MAVEN_PUBLISH_DIR}")
endif()

foreach(publish_file IN LISTS publish_files)
    file(REMOVE "${publish_file}.asc")
    execute_process(
        COMMAND "${QE6502_GPG_EXECUTABLE}"
            --batch
            --yes
            --armor
            --detach-sign
            "${publish_file}"
        RESULT_VARIABLE sign_result
        OUTPUT_VARIABLE sign_stdout
        ERROR_VARIABLE sign_stderr
    )
    if(NOT sign_result EQUAL 0)
        message(FATAL_ERROR "Failed to sign Java Maven artifact ${publish_file} (${sign_result})\nstdout:\n${sign_stdout}\nstderr:\n${sign_stderr}")
    endif()
    if(NOT EXISTS "${publish_file}.asc")
        message(FATAL_ERROR "GPG did not create signature: ${publish_file}.asc")
    endif()
endforeach()

message(STATUS "Signed Java Maven artifacts under ${QE6502_JAVA_MAVEN_PUBLISH_DIR}")
