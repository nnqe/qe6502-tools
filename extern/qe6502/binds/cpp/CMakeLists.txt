enable_language(CXX)

set(QE6502_CPP_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/qe6502/cpu.hpp
)

set(QE6502_CPP_INSTALL_TARGETS)

if(QE6502_BUILD_STATIC)
    add_library(qe6502_cpp_static STATIC
        ${QE6502_CPP_PUBLIC_HEADERS}
        src/cpu.cpp
    )

    target_link_libraries(qe6502_cpp_static
        PUBLIC qe6502::static
    )

    target_include_directories(qe6502_cpp_static
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

    qe6502_configure_warnings(qe6502_cpp_static)
    target_compile_features(qe6502_cpp_static PUBLIC cxx_std_17)

    if(QE6502_BUILD_SHARED)
        set_target_properties(qe6502_cpp_static PROPERTIES
            EXPORT_NAME cpp_static
        )
    else()
        set_target_properties(qe6502_cpp_static PROPERTIES
            EXPORT_NAME cpp
        )
    endif()

    add_library(qe6502_cpp ALIAS qe6502_cpp_static)
    add_library(qe6502::cpp ALIAS qe6502_cpp_static)
    add_library(qe6502::cpp_static ALIAS qe6502_cpp_static)

    list(APPEND QE6502_CPP_INSTALL_TARGETS qe6502_cpp_static)
endif()

if(TARGET qe6502_core_noabi_objects AND QE6502_BUILD_SHARED)
    add_library(qe6502_cpp_shared SHARED
        ${QE6502_CPP_PUBLIC_HEADERS}
        src/cpu.cpp
        $<TARGET_OBJECTS:qe6502_core_noabi_objects>
    )

    target_include_directories(qe6502_cpp_shared
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/cpu/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

    target_compile_definitions(qe6502_cpp_shared
        PUBLIC
            QE6502_CPP_SHARED=1
    )

    qe6502_configure_warnings(qe6502_cpp_shared)
    target_compile_features(qe6502_cpp_shared PUBLIC cxx_std_17)

    if(QE6502_BUILD_STATIC)
        set(QE6502_CPP_SHARED_EXPORT_NAME cpp_shared)
    else()
        set(QE6502_CPP_SHARED_EXPORT_NAME cpp)
    endif()

    set_target_properties(qe6502_cpp_shared PROPERTIES
        EXPORT_NAME ${QE6502_CPP_SHARED_EXPORT_NAME}
        OUTPUT_NAME qe6502_cpp
        CXX_VISIBILITY_PRESET hidden
        C_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )

    if(WIN32)
        set_target_properties(qe6502_cpp_shared PROPERTIES
            PREFIX "lib"
        )
    endif()

    if(MINGW)
        target_link_options(qe6502_cpp_shared PRIVATE
            LINKER:--exclude-all-symbols
        )
    endif()

    set_target_properties(qe6502_cpp_shared PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        MACOSX_RPATH ON
    )

    add_library(qe6502::cpp_shared ALIAS qe6502_cpp_shared)

    if(NOT TARGET qe6502::cpp)
        add_library(qe6502::cpp ALIAS qe6502_cpp_shared)
    endif()

    list(APPEND QE6502_CPP_INSTALL_TARGETS qe6502_cpp_shared)
endif()

if(QE6502_INSTALL)
    if(QE6502_CPP_INSTALL_TARGETS)
        install(TARGETS ${QE6502_CPP_INSTALL_TARGETS}
            EXPORT qe6502Targets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    endif()

    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/qe6502
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()
