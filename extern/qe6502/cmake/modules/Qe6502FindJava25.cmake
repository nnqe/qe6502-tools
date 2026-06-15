# Project-local Java 25 finder helper.
#
# This helper intentionally wraps CMake's built-in FindJava module instead of
# replacing it. It only adds practical JDK 25 hints for common macOS, Linux,
# and Windows install layouts, then calls find_package(Java ...).

macro(_qe6502_java25_export_result)
    foreach(_qe6502_java25_var IN ITEMS
            Java_FOUND
            Java_VERSION
            Java_VERSION_STRING
            Java_JAVA_EXECUTABLE
            Java_JAVAC_EXECUTABLE
            Java_JAR_EXECUTABLE
            Java_Runtime_FOUND
            Java_Development_FOUND)
        if(DEFINED ${_qe6502_java25_var})
            set(${_qe6502_java25_var} "${${_qe6502_java25_var}}" PARENT_SCOPE)
        endif()
    endforeach()
endmacro()

function(qe6502_find_java25)
    set(options REQUIRED)
    cmake_parse_arguments(QE6502_JAVA25 "${options}" "" "" ${ARGN})

    set(QE6502_JAVA25_MIN_VERSION 25)

    if(Java_JAVA_EXECUTABLE OR Java_JAVAC_EXECUTABLE OR Java_JAR_EXECUTABLE)
        if(QE6502_JAVA25_REQUIRED)
            find_package(Java ${QE6502_JAVA25_MIN_VERSION} REQUIRED COMPONENTS Runtime Development)
        else()
            find_package(Java ${QE6502_JAVA25_MIN_VERSION} QUIET COMPONENTS Runtime Development)
        endif()
        _qe6502_java25_export_result()
        if(QE6502_JAVA25_REQUIRED AND NOT Java_FOUND)
            message(FATAL_ERROR "Java 25+ is required but was not found")
        endif()
        return()
    endif()

    set(_qe6502_java_hints)

    if(DEFINED ENV{JAVA_HOME} AND NOT "$ENV{JAVA_HOME}" STREQUAL "")
        list(APPEND _qe6502_java_hints "$ENV{JAVA_HOME}")
    endif()

    if(APPLE)
        execute_process(
            COMMAND /usr/libexec/java_home -v ${QE6502_JAVA25_MIN_VERSION}
            OUTPUT_VARIABLE _qe6502_java_home
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        if(_qe6502_java_home)
            list(APPEND _qe6502_java_hints "${_qe6502_java_home}")
        endif()

        list(APPEND _qe6502_java_hints
            "/opt/homebrew/opt/openjdk@25"
            "/usr/local/opt/openjdk@25"
            "/opt/homebrew/opt/openjdk"
            "/usr/local/opt/openjdk"
        )
    elseif(WIN32)
        file(GLOB _qe6502_windows_jdks
            "C:/Program Files/Eclipse Adoptium/jdk-25*"
            "C:/Program Files/Java/jdk-25*"
            "C:/Program Files/Microsoft/jdk-25*"
            "C:/Program Files/Zulu/zulu-25*"
        )
        list(APPEND _qe6502_java_hints ${_qe6502_windows_jdks})
    else()
        list(APPEND _qe6502_java_hints
            "/usr/lib/jvm/java-25-openjdk"
            "/usr/lib/jvm/java-25-openjdk-amd64"
            "/usr/lib/jvm/temurin-25-jdk-amd64"
            "/usr/lib/jvm/temurin-25-jdk"
            "/usr/lib/jvm/jdk-25"
            "/usr/lib/jvm/openjdk-25"
        )
    endif()

    foreach(_qe6502_java_hint IN LISTS _qe6502_java_hints)
        if(WIN32)
            set(_qe6502_java_bin_suffix ".exe")
        else()
            set(_qe6502_java_bin_suffix "")
        endif()

        if(EXISTS "${_qe6502_java_hint}/bin/java${_qe6502_java_bin_suffix}"
                AND EXISTS "${_qe6502_java_hint}/bin/javac${_qe6502_java_bin_suffix}"
                AND EXISTS "${_qe6502_java_hint}/bin/jar${_qe6502_java_bin_suffix}")
            set(Java_JAVA_EXECUTABLE "${_qe6502_java_hint}/bin/java${_qe6502_java_bin_suffix}" CACHE FILEPATH "Java runtime" FORCE)
            set(Java_JAVAC_EXECUTABLE "${_qe6502_java_hint}/bin/javac${_qe6502_java_bin_suffix}" CACHE FILEPATH "Java compiler" FORCE)
            set(Java_JAR_EXECUTABLE "${_qe6502_java_hint}/bin/jar${_qe6502_java_bin_suffix}" CACHE FILEPATH "Java archive tool" FORCE)
            break()
        endif()
    endforeach()

    if(QE6502_JAVA25_REQUIRED)
        find_package(Java ${QE6502_JAVA25_MIN_VERSION} REQUIRED COMPONENTS Runtime Development)
    else()
        find_package(Java ${QE6502_JAVA25_MIN_VERSION} QUIET COMPONENTS Runtime Development)
    endif()
    _qe6502_java25_export_result()

    if(QE6502_JAVA25_REQUIRED AND NOT Java_FOUND)
        message(FATAL_ERROR "Java 25+ is required but was not found")
    endif()
endfunction()
