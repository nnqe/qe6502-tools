file(GLOB_RECURSE APPLEII_SOURCE_FILES  ./*.c)

add_library(appleII STATIC ${APPLEII_SOURCE_FILES})

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  target_compile_options(appleII PRIVATE -O3 -DNDEBUG)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fno-inline -fno-omit-frame-pointer")

target_link_libraries(appleII PRIVATE qe6502)

target_include_directories(appleII PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

