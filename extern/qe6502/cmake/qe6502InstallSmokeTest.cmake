if(NOT DEFINED QE6502_BUILD_DIR)
    message(FATAL_ERROR "QE6502_BUILD_DIR is required")
endif()

get_filename_component(QE6502_BUILD_DIR "${QE6502_BUILD_DIR}" ABSOLUTE)
set(qe6502_smoke_root "${QE6502_BUILD_DIR}/install-smoke")
set(qe6502_install_prefix "${qe6502_smoke_root}/prefix")
set(qe6502_consumer_src "${qe6502_smoke_root}/consumer")
set(qe6502_consumer_build "${qe6502_smoke_root}/build")

file(REMOVE_RECURSE "${qe6502_smoke_root}")
file(MAKE_DIRECTORY "${qe6502_consumer_src}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${QE6502_BUILD_DIR}" --prefix "${qe6502_install_prefix}"
    RESULT_VARIABLE qe6502_install_result
)
if(NOT qe6502_install_result EQUAL 0)
    message(FATAL_ERROR "cmake --install failed")
endif()

file(WRITE "${qe6502_consumer_src}/CMakeLists.txt" [=[
cmake_minimum_required(VERSION 3.15)
project(qe6502_install_smoke LANGUAGES C CXX)

find_package(qe6502 CONFIG REQUIRED COMPONENTS C Static ABI CXX)

add_executable(use_static use_static.c)
target_link_libraries(use_static PRIVATE qe6502::static)

add_executable(use_shared use_shared.c)
target_link_libraries(use_shared PRIVATE qe6502::shared)

add_executable(use_cpp use_cpp.cpp)
target_link_libraries(use_cpp PRIVATE qe6502::cpp)
]=])

file(WRITE "${qe6502_consumer_src}/use_static.c" [=[
#include <qe6502/qe6502.h>

int main(void)
{
    qe6502_t cpu = qe6502_setup(qe6502_model_nmos);
    (void)qe6502_restart(&cpu);
    return 0;
}
]=])

file(WRITE "${qe6502_consumer_src}/use_shared.c" [=[
#include <qe6502/qe6502_abi.h>

int main(void)
{
    qe6502abi_context_t cpu;
    qe6502abi_setup(&cpu, QE6502_ABI_MODEL_NMOS);
    return qe6502abi_version() == QE6502_VERSION ? 0 : 1;
}
]=])

file(WRITE "${qe6502_consumer_src}/use_cpp.cpp" [=[
#include <qe6502/cpu.hpp>

int main()
{
    qe6502::cpu cpu(qe6502::model::nmos);
    cpu.restart();
    return cpu.cpu_model() == qe6502::model::nmos ? 0 : 1;
}
]=])

execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${qe6502_consumer_src}" -B "${qe6502_consumer_build}" "-Dqe6502_DIR=${qe6502_install_prefix}/lib/cmake/qe6502"
    RESULT_VARIABLE qe6502_config_result
)
if(NOT qe6502_config_result EQUAL 0)
    message(FATAL_ERROR "install smoke configure failed")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${qe6502_consumer_build}"
    RESULT_VARIABLE qe6502_build_result
)
if(NOT qe6502_build_result EQUAL 0)
    message(FATAL_ERROR "install smoke build failed")
endif()

execute_process(
    COMMAND "${qe6502_consumer_build}/use_static"
    RESULT_VARIABLE qe6502_static_result
)
if(NOT qe6502_static_result EQUAL 0)
    message(FATAL_ERROR "install smoke static executable failed")
endif()

execute_process(
    COMMAND "${qe6502_consumer_build}/use_shared"
    RESULT_VARIABLE qe6502_shared_result
)
if(NOT qe6502_shared_result EQUAL 0)
    message(FATAL_ERROR "install smoke shared executable failed")
endif()

execute_process(
    COMMAND "${qe6502_consumer_build}/use_cpp"
    RESULT_VARIABLE qe6502_cpp_result
)
if(NOT qe6502_cpp_result EQUAL 0)
    message(FATAL_ERROR "install smoke C++ executable failed")
endif()

message(STATUS "Install smoke test passed")
