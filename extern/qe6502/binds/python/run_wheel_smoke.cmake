if(NOT DEFINED QE6502_PYTHON_EXECUTABLE OR QE6502_PYTHON_EXECUTABLE STREQUAL "")
    message(FATAL_ERROR "QE6502_PYTHON_EXECUTABLE is required")
endif()

if(NOT DEFINED QE6502_PYTHON_DIST_DIR OR QE6502_PYTHON_DIST_DIR STREQUAL "")
    message(FATAL_ERROR "QE6502_PYTHON_DIST_DIR is required")
endif()

if(NOT DEFINED QE6502_PYTHON_WHEEL_GLOB OR QE6502_PYTHON_WHEEL_GLOB STREQUAL "")
    message(FATAL_ERROR "QE6502_PYTHON_WHEEL_GLOB is required")
endif()

if(NOT DEFINED QE6502_PYTHON_SMOKE_DIR OR QE6502_PYTHON_SMOKE_DIR STREQUAL "")
    message(FATAL_ERROR "QE6502_PYTHON_SMOKE_DIR is required")
endif()

file(GLOB wheel_candidates "${QE6502_PYTHON_WHEEL_GLOB}")
list(LENGTH wheel_candidates wheel_count)
if(NOT wheel_count EQUAL 1)
    message(FATAL_ERROR "Expected exactly one qe6502 wheel in ${QE6502_PYTHON_DIST_DIR}, found ${wheel_count}: ${wheel_candidates}")
endif()
list(GET wheel_candidates 0 wheel_file)

file(REMOVE_RECURSE "${QE6502_PYTHON_SMOKE_DIR}")
file(MAKE_DIRECTORY "${QE6502_PYTHON_SMOKE_DIR}")

execute_process(
    COMMAND "${QE6502_PYTHON_EXECUTABLE}" -m venv "${QE6502_PYTHON_SMOKE_DIR}/venv"
    RESULT_VARIABLE venv_result
    OUTPUT_VARIABLE venv_stdout
    ERROR_VARIABLE venv_stderr
)
if(NOT venv_result EQUAL 0)
    message(FATAL_ERROR "Python venv creation failed (${venv_result})\nstdout:\n${venv_stdout}\nstderr:\n${venv_stderr}")
endif()

if(CMAKE_HOST_WIN32)
    set(venv_python "${QE6502_PYTHON_SMOKE_DIR}/venv/Scripts/python.exe")
else()
    set(venv_python "${QE6502_PYTHON_SMOKE_DIR}/venv/bin/python")
endif()

execute_process(
    COMMAND "${venv_python}" -m pip install --no-index --force-reinstall "${wheel_file}"
    RESULT_VARIABLE install_result
    OUTPUT_VARIABLE install_stdout
    ERROR_VARIABLE install_stderr
)
if(NOT install_result EQUAL 0)
    message(FATAL_ERROR "Python wheel install failed (${install_result})\nwheel: ${wheel_file}\nstdout:\n${install_stdout}\nstderr:\n${install_stderr}")
endif()

set(smoke_script "${QE6502_PYTHON_SMOKE_DIR}/wheel_consumer_smoke.py")
file(WRITE "${smoke_script}" [=[
import qe6502

ADDR = qe6502.TICK_ADDRESS_MASK
BUS_SHIFT = qe6502.TICK_BUS_SHIFT
WRITING = qe6502.TICK_WRITING


def run_bus(cpu, memory, bus_state, cycles):
    for _ in range(cycles):
        address = bus_state & ADDR
        if bus_state & WRITING:
            memory[address] = bus_state >> BUS_SHIFT
            bus_state = cpu.tick()
        else:
            bus_state = cpu.tick(memory[address])
    return bus_state


if qe6502.version() != qe6502.ABI_VERSION:
    raise AssertionError("unexpected ABI version")

cpu = qe6502.CPU(qe6502.MODEL_NMOS)
memory = bytearray(65536)
memory[0x8000:0x8006] = bytes([0xEE, 0x00, 0x02, 0x4C, 0x00, 0x80])

bus_state = cpu.jump_to(0x8000)
run_bus(cpu, memory, bus_state, 64)

if memory[0x0200] == 0:
    raise AssertionError("program did not update memory")

cpu.a = 0x42
snapshot = cpu.save()
if len(snapshot) != qe6502.SNAPSHOT_SIZE:
    raise AssertionError("unexpected snapshot size")

restored = qe6502.CPU(qe6502.MODEL_NES)
restored_tick = restored.load(snapshot)
if restored_tick != cpu.raw_tick or restored.a != 0x42:
    raise AssertionError("snapshot restore failed")

print("qe6502 Python wheel consumer smoke OK")
]=])

execute_process(
    COMMAND "${venv_python}" "${smoke_script}"
    RESULT_VARIABLE smoke_result
    OUTPUT_VARIABLE smoke_stdout
    ERROR_VARIABLE smoke_stderr
)
if(NOT smoke_result EQUAL 0)
    message(FATAL_ERROR "Python wheel consumer smoke failed (${smoke_result})\nstdout:\n${smoke_stdout}\nstderr:\n${smoke_stderr}")
endif()

message(STATUS "Python wheel consumer smoke passed for ${wheel_file}")
message(STATUS "${smoke_stdout}")
