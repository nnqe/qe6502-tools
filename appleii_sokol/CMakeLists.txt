option(QEAII_BUILD_SOKOL_APP "Build the minimal Sokol Apple II runner" ON)

if(NOT QEAII_BUILD_SOKOL_APP)
    return()
endif()

set(QEAII_SOKOL_APP_AVAILABLE ON)

if(UNIX AND NOT APPLE)
    include(CheckIncludeFile)
    check_include_file("GL/gl.h" QEAII_HAVE_GL_GL_H)
    if(NOT QEAII_HAVE_GL_GL_H)
        message(WARNING "Skipping qeaii_sokol_appleii: GL/gl.h was not found")
        set(QEAII_SOKOL_APP_AVAILABLE OFF)
    endif()

    find_package(X11 QUIET)
    if(NOT X11_FOUND)
        message(WARNING "Skipping qeaii_sokol_appleii: X11 development files were not found")
        set(QEAII_SOKOL_APP_AVAILABLE OFF)
    endif()
endif()

if(NOT QEAII_SOKOL_APP_AVAILABLE)
    return()
endif()

if(APPLE)
    # sokol_app.h includes Cocoa/AppKit on macOS, so the implementation
    # translation unit must be compiled as Objective-C even though the
    # source is intentionally kept as plain C-compatible code.
    enable_language(OBJC)
    set_source_files_properties(src/appleii_sokol.c PROPERTIES LANGUAGE OBJC)
endif()

add_executable(qeaii_sokol_appleii
    src/appleii_sokol.c
    src/dsk2nib.c
)

target_compile_features(qeaii_sokol_appleii PRIVATE c_std_11)

target_include_directories(qeaii_sokol_appleii PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/../appleii/roms"
)

target_compile_definitions(qeaii_sokol_appleii PRIVATE SOKOL_GLCORE)

target_link_libraries(qeaii_sokol_appleii PRIVATE
    qeaii::qeaii
    qe6502_tools_sokol
)

if(APPLE)
    target_link_libraries(qeaii_sokol_appleii PRIVATE
        "-framework Cocoa"
        "-framework QuartzCore"
        "-framework OpenGL"
        "-framework IOKit"
    )
elseif(WIN32)
    target_link_libraries(qeaii_sokol_appleii PRIVATE
        opengl32
        gdi32
        user32
        shell32
    )
else()
    find_library(QEAII_GL_LIBRARY NAMES GL libGL.so.1)
    if(QEAII_GL_LIBRARY)
        target_link_libraries(qeaii_sokol_appleii PRIVATE ${QEAII_GL_LIBRARY})
    else()
        target_link_libraries(qeaii_sokol_appleii PRIVATE GL)
    endif()

    find_package(Threads REQUIRED)
    target_link_libraries(qeaii_sokol_appleii PRIVATE
        Threads::Threads
        ${X11_LIBRARIES}
        Xi
        Xcursor
        dl
        m
    )
endif()
