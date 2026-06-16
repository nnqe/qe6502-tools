set(QEAII_SOURCE_FILES
    src/qeaii.c
    src/qeaiiIIhelpers.c
)

add_library(qeaii STATIC ${QEAII_SOURCE_FILES})
add_library(qeaii::qeaii ALIAS qeaii)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(qeaii PRIVATE -O3 -DNDEBUG)
endif()

target_compile_features(qeaii PUBLIC c_std_11)

target_link_libraries(qeaii PUBLIC qe6502::static)

target_include_directories(qeaii PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

