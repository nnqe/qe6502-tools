foreach(required_var IN ITEMS QE6502_JAVA_MAVEN_PUBLISH_DIR QE6502_JAVA_MAVEN_CENTRAL_BUNDLE)
    if(NOT DEFINED ${required_var} OR "${${required_var}}" STREQUAL "")
        message(FATAL_ERROR "${required_var} is required")
    endif()
endforeach()

if(NOT IS_DIRECTORY "${QE6502_JAVA_MAVEN_PUBLISH_DIR}")
    message(FATAL_ERROR "Java Maven publish directory does not exist: ${QE6502_JAVA_MAVEN_PUBLISH_DIR}")
endif()

get_filename_component(bundle_dir "${QE6502_JAVA_MAVEN_CENTRAL_BUNDLE}" DIRECTORY)
file(MAKE_DIRECTORY "${bundle_dir}")
file(REMOVE "${QE6502_JAVA_MAVEN_CENTRAL_BUNDLE}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar cf "${QE6502_JAVA_MAVEN_CENTRAL_BUNDLE}" --format=zip -- .
    WORKING_DIRECTORY "${QE6502_JAVA_MAVEN_PUBLISH_DIR}"
    RESULT_VARIABLE zip_result
    OUTPUT_VARIABLE zip_stdout
    ERROR_VARIABLE zip_stderr
)

if(NOT zip_result EQUAL 0)
    message(FATAL_ERROR "Failed to package Java Maven Central bundle (${zip_result})\nstdout:\n${zip_stdout}\nstderr:\n${zip_stderr}")
endif()

if(NOT EXISTS "${QE6502_JAVA_MAVEN_CENTRAL_BUNDLE}")
    message(FATAL_ERROR "Java Maven Central bundle was not created: ${QE6502_JAVA_MAVEN_CENTRAL_BUNDLE}")
endif()

message(STATUS "Packaged Java Maven Central bundle: ${QE6502_JAVA_MAVEN_CENTRAL_BUNDLE}")
