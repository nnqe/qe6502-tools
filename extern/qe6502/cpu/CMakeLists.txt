# qe6502/CMakeLists.txt
# qe6502: no external dependencies

set(QE6502_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/qe6502/qe6502_version.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/qe6502/qe6502_abi.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/qe6502/qe6502.h
)

file(GLOB_RECURSE QE6502_PRIVATE_SOURCE CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

function(qe6502_configure_core_target target)
    target_include_directories(${target}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

    target_compile_definitions(${target}
        PUBLIC
            QE6502_VERBOS=$<BOOL:${QE6502_VERBOS}>
        PRIVATE
            QE6502_BUILD_WASM=$<BOOL:${QE6502_BUILD_WASM}>
    )

    if(QE6502_BUILD_WASM)
        target_compile_options(${target} PRIVATE -ffreestanding)
    endif()

    qe6502_configure_warnings(${target})
    target_compile_features(${target} PUBLIC c_std_11)
endfunction()

if(QE6502_BUILD_WASM)
    message(STATUS "QE6502 2 WASM Build")
else()
    message(STATUS "QE6502 2 Native Build")
endif()

add_library(qe6502_core_noabi_objects OBJECT
    ${QE6502_PRIVATE_SOURCE}
)

qe6502_configure_core_target(qe6502_core_noabi_objects)

target_compile_definitions(qe6502_core_noabi_objects
    PRIVATE
        QE6502_NO_ABI=1
)

set_target_properties(qe6502_core_noabi_objects PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    C_VISIBILITY_PRESET hidden
)

if(QE6502_BUILD_STATIC)
    add_library(qe6502_static_objects OBJECT
        ${QE6502_PRIVATE_SOURCE}
    )

    qe6502_configure_core_target(qe6502_static_objects)

    target_compile_definitions(qe6502_static_objects
        PRIVATE
            QE6502_STATIC=1
            QE6502_BUILD_SHARED=0
    )

    set_target_properties(qe6502_static_objects PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        C_VISIBILITY_PRESET hidden
    )

    add_library(qe6502_static STATIC
        ${QE6502_PUBLIC_HEADERS}
        $<TARGET_OBJECTS:qe6502_static_objects>
    )

    qe6502_configure_core_target(qe6502_static)

    target_compile_definitions(qe6502_static
        PUBLIC
            QE6502_STATIC=1
    )

    set_target_properties(qe6502_static PROPERTIES
        EXPORT_NAME static
        POSITION_INDEPENDENT_CODE ON
        C_VISIBILITY_PRESET hidden
    )

    add_library(qe6502 ALIAS qe6502_static)
    add_library(qe6502::static ALIAS qe6502_static)
endif()

if(QE6502_BUILD_SHARED)
    add_library(qe6502_shared_objects OBJECT
        ${QE6502_PRIVATE_SOURCE}
    )

    qe6502_configure_core_target(qe6502_shared_objects)

    target_compile_definitions(qe6502_shared_objects
        PRIVATE
            QE6502_BUILDING_LIBRARY=1
            QE6502_BUILD_SHARED=1
            QE6502_SHARED=1
    )

    set_target_properties(qe6502_shared_objects PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        C_VISIBILITY_PRESET hidden
    )

    if(QE6502_BUILD_WASM)
        add_executable(qe6502_shared
            ${QE6502_PUBLIC_HEADERS}
            $<TARGET_OBJECTS:qe6502_shared_objects>
        )
    else()
        add_library(qe6502_shared SHARED
            ${QE6502_PUBLIC_HEADERS}
            $<TARGET_OBJECTS:qe6502_shared_objects>
        )
    endif()

    qe6502_configure_core_target(qe6502_shared)

    target_compile_definitions(qe6502_shared
        PUBLIC
            QE6502_SHARED=1
    )

    set_target_properties(qe6502_shared PROPERTIES
        EXPORT_NAME shared
        OUTPUT_NAME qe6502
        C_VISIBILITY_PRESET hidden
    )

    if(WIN32)
        set_target_properties(qe6502_shared PROPERTIES
            PREFIX "lib"
        )
    endif()

    if(MINGW)
        target_link_options(qe6502_shared PRIVATE
            LINKER:--exclude-all-symbols
        )
    endif()

    if(NOT QE6502_BUILD_WASM)
        set_target_properties(qe6502_shared PROPERTIES
            VERSION ${PROJECT_VERSION}
            SOVERSION ${PROJECT_VERSION_MAJOR}
            MACOSX_RPATH ON
        )
    endif()

    if(QE6502_BUILD_WASM)
        set_target_properties(qe6502_shared PROPERTIES
            PREFIX ""
            SUFFIX ".wasm"
        )

        target_link_options(qe6502_shared PRIVATE
            -nostdlib
            LINKER:--no-entry
            ${QE6502_WASM_EXPORT_MEMORY_LINK_OPTION}
            LINKER:--export=qe6502abi_version
            LINKER:--export=qe6502abi_setup
            LINKER:--export=qe6502abi_restart
            LINKER:--export=qe6502abi_goto
            LINKER:--export=qe6502abi_irq_assert
            LINKER:--export=qe6502abi_is_irq_asserted
            LINKER:--export=qe6502abi_save
            LINKER:--export=qe6502abi_load
            LINKER:--export=qe6502abi_nmi_assert
            LINKER:--export=qe6502abi_is_nmi_asserted
            LINKER:--export=qe6502abi_tick
            LINKER:--export=qe6502abi_get_pc
            LINKER:--export=qe6502abi_set_pc
            LINKER:--export=qe6502abi_get_s
            LINKER:--export=qe6502abi_set_s
            LINKER:--export=qe6502abi_get_a
            LINKER:--export=qe6502abi_set_a
            LINKER:--export=qe6502abi_get_x
            LINKER:--export=qe6502abi_set_x
            LINKER:--export=qe6502abi_get_y
            LINKER:--export=qe6502abi_set_y
            LINKER:--export=qe6502abi_get_p
            LINKER:--export=qe6502abi_set_p
            LINKER:--export=qe6502abi_get_model
            LINKER:--export=qe6502abi_set_model
        )
    endif()

    if(QE6502_BUILD_WASM)
        add_executable(qe6502::shared ALIAS qe6502_shared)
        set(QE6502_RUNTIME_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})
    else()
        add_library(qe6502::shared ALIAS qe6502_shared)
        set(QE6502_RUNTIME_INSTALL_DIR ${CMAKE_INSTALL_BINDIR})
    endif()
endif()

if(QE6502_INSTALL)
    set(QE6502_CORE_INSTALL_TARGETS)

    if(TARGET qe6502_static)
        list(APPEND QE6502_CORE_INSTALL_TARGETS qe6502_static)
    endif()

    if(TARGET qe6502_shared)
        list(APPEND QE6502_CORE_INSTALL_TARGETS qe6502_shared)
    endif()

    if(QE6502_CORE_INSTALL_TARGETS)
        install(TARGETS ${QE6502_CORE_INSTALL_TARGETS}
            EXPORT qe6502Targets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${QE6502_RUNTIME_INSTALL_DIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    endif()

    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/qe6502
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()
