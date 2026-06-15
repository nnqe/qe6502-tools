foreach(required_var IN ITEMS
        QE6502_JAVA_MAVEN_PUBLISH_DIR
        QE6502_JAVA_MAVEN_GROUP_ID
        QE6502_JAVA_MAVEN_ARTIFACT_ID
        QE6502_JAVA_MAVEN_VERSION)
    if(NOT DEFINED ${required_var} OR "${${required_var}}" STREQUAL "")
        message(FATAL_ERROR "${required_var} is required")
    endif()
endforeach()

if(NOT IS_DIRECTORY "${QE6502_JAVA_MAVEN_PUBLISH_DIR}")
    message(FATAL_ERROR "Java Maven publish directory does not exist: ${QE6502_JAVA_MAVEN_PUBLISH_DIR}")
endif()

string(REPLACE "." "/" group_path "${QE6502_JAVA_MAVEN_GROUP_ID}")
set(artifact_dir "${QE6502_JAVA_MAVEN_PUBLISH_DIR}/${group_path}/${QE6502_JAVA_MAVEN_ARTIFACT_ID}/${QE6502_JAVA_MAVEN_VERSION}")
set(output_base "${artifact_dir}/${QE6502_JAVA_MAVEN_ARTIFACT_ID}-${QE6502_JAVA_MAVEN_VERSION}")

set(primary_artifacts
    "${output_base}.jar"
    "${output_base}-sources.jar"
    "${output_base}-javadoc.jar"
    "${output_base}.pom"
)

foreach(primary_artifact IN LISTS primary_artifacts)
    if(NOT EXISTS "${primary_artifact}")
        message(FATAL_ERROR "Missing Maven Central primary artifact: ${primary_artifact}")
    endif()

    file(SIZE "${primary_artifact}" artifact_size)
    if(artifact_size EQUAL 0)
        message(FATAL_ERROR "Maven Central primary artifact is empty: ${primary_artifact}")
    endif()

    foreach(required_suffix IN ITEMS .asc .md5 .sha1 .asc.md5 .asc.sha1)
        if(NOT EXISTS "${primary_artifact}${required_suffix}")
            message(FATAL_ERROR "Missing Maven Central sidecar: ${primary_artifact}${required_suffix}")
        endif()
    endforeach()

    file(MD5 "${primary_artifact}" expected_md5)
    file(READ "${primary_artifact}.md5" actual_md5)
    string(STRIP "${actual_md5}" actual_md5)
    if(NOT actual_md5 STREQUAL expected_md5)
        message(FATAL_ERROR "MD5 checksum mismatch for ${primary_artifact}")
    endif()

    file(SHA1 "${primary_artifact}" expected_sha1)
    file(READ "${primary_artifact}.sha1" actual_sha1)
    string(STRIP "${actual_sha1}" actual_sha1)
    if(NOT actual_sha1 STREQUAL expected_sha1)
        message(FATAL_ERROR "SHA1 checksum mismatch for ${primary_artifact}")
    endif()

    file(MD5 "${primary_artifact}.asc" expected_asc_md5)
    file(READ "${primary_artifact}.asc.md5" actual_asc_md5)
    string(STRIP "${actual_asc_md5}" actual_asc_md5)
    if(NOT actual_asc_md5 STREQUAL expected_asc_md5)
        message(FATAL_ERROR "ASC MD5 checksum mismatch for ${primary_artifact}.asc")
    endif()

    file(SHA1 "${primary_artifact}.asc" expected_asc_sha1)
    file(READ "${primary_artifact}.asc.sha1" actual_asc_sha1)
    string(STRIP "${actual_asc_sha1}" actual_asc_sha1)
    if(NOT actual_asc_sha1 STREQUAL expected_asc_sha1)
        message(FATAL_ERROR "ASC SHA1 checksum mismatch for ${primary_artifact}.asc")
    endif()
endforeach()

file(READ "${output_base}.pom" pom_text)
foreach(expected_xml IN ITEMS
        "<groupId>${QE6502_JAVA_MAVEN_GROUP_ID}</groupId>"
        "<artifactId>${QE6502_JAVA_MAVEN_ARTIFACT_ID}</artifactId>"
        "<version>${QE6502_JAVA_MAVEN_VERSION}</version>")
    if(NOT pom_text MATCHES "${expected_xml}")
        message(FATAL_ERROR "POM does not contain expected coordinate element: ${expected_xml}")
    endif()
endforeach()

message(STATUS "Verified Java Maven Central bundle layout under ${artifact_dir}")
