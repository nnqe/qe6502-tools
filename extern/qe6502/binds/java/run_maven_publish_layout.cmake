if(POLICY CMP0007)
    cmake_policy(SET CMP0007 NEW)
endif()

foreach(required_var IN ITEMS
        QE6502_JAVA_PACKAGE_DIR
        QE6502_JAVA_MAVEN_PUBLISH_DIR
        QE6502_JAVA_MAVEN_GROUP_ID
        QE6502_JAVA_MAVEN_ARTIFACT_ID
        QE6502_JAVA_MAVEN_VERSION)
    if(NOT DEFINED ${required_var} OR "${${required_var}}" STREQUAL "")
        message(FATAL_ERROR "${required_var} is required")
    endif()
endforeach()

set(package_jar "${QE6502_JAVA_PACKAGE_DIR}/qe6502-java.jar")
set(package_sources_jar "${QE6502_JAVA_PACKAGE_DIR}/qe6502-java-sources.jar")
set(package_javadoc_jar "${QE6502_JAVA_PACKAGE_DIR}/qe6502-java-javadoc.jar")
set(package_pom "${QE6502_JAVA_PACKAGE_DIR}/pom.xml")

foreach(required_file IN ITEMS
        "${package_jar}"
        "${package_sources_jar}"
        "${package_javadoc_jar}"
        "${package_pom}")
    if(NOT EXISTS "${required_file}")
        message(FATAL_ERROR "Required Java Maven publish input does not exist: ${required_file}")
    endif()
endforeach()

string(REPLACE "." "/" group_path "${QE6502_JAVA_MAVEN_GROUP_ID}")
set(output_dir "${QE6502_JAVA_MAVEN_PUBLISH_DIR}/${group_path}/${QE6502_JAVA_MAVEN_ARTIFACT_ID}/${QE6502_JAVA_MAVEN_VERSION}")
set(output_base "${output_dir}/${QE6502_JAVA_MAVEN_ARTIFACT_ID}-${QE6502_JAVA_MAVEN_VERSION}")

file(REMOVE_RECURSE "${QE6502_JAVA_MAVEN_PUBLISH_DIR}")
file(MAKE_DIRECTORY "${output_dir}")

configure_file("${package_jar}" "${output_base}.jar" COPYONLY)
configure_file("${package_sources_jar}" "${output_base}-sources.jar" COPYONLY)
configure_file("${package_javadoc_jar}" "${output_base}-javadoc.jar" COPYONLY)
configure_file("${package_pom}" "${output_base}.pom" COPYONLY)

set(expected_files
    "${output_base}.jar"
    "${output_base}-sources.jar"
    "${output_base}-javadoc.jar"
    "${output_base}.pom"
)

foreach(expected_file IN LISTS expected_files)
    if(NOT EXISTS "${expected_file}")
        message(FATAL_ERROR "Failed to create Java Maven publish artifact: ${expected_file}")
    endif()
endforeach()

message(STATUS "Java Maven publish layout staged at ${output_dir}")
