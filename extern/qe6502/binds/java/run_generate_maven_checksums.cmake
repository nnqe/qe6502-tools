if(NOT DEFINED QE6502_JAVA_MAVEN_PUBLISH_DIR OR QE6502_JAVA_MAVEN_PUBLISH_DIR STREQUAL "")
    message(FATAL_ERROR "QE6502_JAVA_MAVEN_PUBLISH_DIR is required")
endif()

if(NOT IS_DIRECTORY "${QE6502_JAVA_MAVEN_PUBLISH_DIR}")
    message(FATAL_ERROR "Java Maven publish directory does not exist: ${QE6502_JAVA_MAVEN_PUBLISH_DIR}")
endif()

file(GLOB_RECURSE maven_files
    "${QE6502_JAVA_MAVEN_PUBLISH_DIR}/*.jar"
    "${QE6502_JAVA_MAVEN_PUBLISH_DIR}/*.pom"
    "${QE6502_JAVA_MAVEN_PUBLISH_DIR}/*.asc"
)
list(SORT maven_files)

if(NOT maven_files)
    message(FATAL_ERROR "No Java Maven artifacts found under ${QE6502_JAVA_MAVEN_PUBLISH_DIR}")
endif()

foreach(maven_file IN LISTS maven_files)
    if(maven_file MATCHES "\\.(md5|sha1|sha256|sha512)$")
        continue()
    endif()

    file(MD5 "${maven_file}" md5_digest)
    file(SHA1 "${maven_file}" sha1_digest)
    file(WRITE "${maven_file}.md5" "${md5_digest}\n")
    file(WRITE "${maven_file}.sha1" "${sha1_digest}\n")
endforeach()

message(STATUS "Generated Java Maven MD5/SHA1 checksums under ${QE6502_JAVA_MAVEN_PUBLISH_DIR}")
