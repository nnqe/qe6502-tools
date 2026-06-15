import assert from "node:assert/strict";
import { fileURLToPath } from "node:url";
import { loadQe6502Node, Model, Status } from "../../binds/js/qe6502.js";

const START_ADDRESS = 0x8000;
const SUCCESS_ADDRESS = 0x80ff;
const CHECKPOINT_INTERVAL = 3;
const MAX_OPCODE_CYCLES = 200n;
const MEMORY_SIZE = 0x10000;
const ADDRESS_MASK = 0xffff;
const STATUS_SHIFT = 16;
const BUS_SHIFT = 24;
const defaultWasmUrl = new URL("../../build/release_wasm/binds/js/qe6502_js.wasm", import.meta.url);

function usage(scriptName) {
  return (
    `Usage: node ${scriptName} [<qe6502_shared.wasm>]\n\n` +
    `Runs the NES functional save/load checkpoint test. The checkpointed run ` +
    `saves and reloads CPU state every ${CHECKPOINT_INTERVAL} bus ticks.`
  );
}

function parseCommandLine(argv) {
  const scriptName = argv[1] ?? "js_save_load_functional.mjs";
  const args = argv.slice(2);

  if (args.length === 0) {
    return { wasmPath: fileURLToPath(defaultWasmUrl) };
  }

  if (args.length === 1) {
    return { wasmPath: args[0] };
  }

  throw new Error(usage(scriptName));
}

function tickAddress(tick) {
  return tick & ADDRESS_MASK;
}

function tickBus(tick) {
  return (tick >>> BUS_SHIFT) & 0xff;
}

function tickStatus(tick) {
  return (tick >>> STATUS_SHIFT) & 0xff;
}

function isWriteStatus(status) {
  return (status & Status.writing) !== 0;
}

function isOpcodeFetchStatus(status) {
  return (status & Status.fetch) !== 0;
}

function installNesFunctionalProgram(memory) {
  // SED is intentional. On the NES CPU, decimal arithmetic is disabled, so
  // 0x15 + 0x27 must produce binary 0x3C rather than BCD 0x42.
  const program = [
    0xf8,             // SED
    0xa9, 0x15,       // LDA #$15
    0x18,             // CLC
    0x69, 0x27,       // ADC #$27
    0x8d, 0x00, 0x02, // STA $0200
    0xd8,             // CLD
    0xa2, 0x05,       // LDX #$05
    0xca,             // loop: DEX
    0x8e, 0x01, 0x02, // STX $0201
    0xd0, 0xfa,       // BNE loop
    0x08,             // PHP
    0x68,             // PLA
    0x8d, 0x02, 0x02, // STA $0202
    0x4c, 0xff, 0x80, // JMP $80FF
  ];

  memory.set(program, START_ADDRESS);
}

function captureState(cpu) {
  return Object.freeze({
    model: cpu.cpuModel(),
    pc: cpu.pc(),
    s: cpu.s(),
    a: cpu.a(),
    x: cpu.x(),
    y: cpu.y(),
    p: cpu.p(),
    tickAddress: cpu.busAddress(),
    tickBus: cpu.busData(),
    tickStatus: cpu.busStatus(),
  });
}

function runNesFunctional(qe, initialMemory, checkpointed) {
  const memory = new Uint8Array(initialMemory);
  let cpu = qe.createCpu(Model.nes);
  let pass = false;
  let message = "CPU Error";
  let opcodeCycles = 0n;
  let busTicks = 0n;
  let finalState = null;

  try {
    cpu.restart();
    let tick = cpu.jumpTo(START_ADDRESS);

    for (;;) {
      const address = tickAddress(tick);
      const status = tickStatus(tick);
      let data = isWriteStatus(status) ? tickBus(tick) : memory[address];

      if (address === SUCCESS_ADDRESS) {
        pass = true;
        message = "OK";
        break;
      }

      if (isWriteStatus(status)) {
        memory[address] = data;
      } else {
        data = memory[address];
      }

      tick = cpu.tick(data);
      ++busTicks;

      if (checkpointed && busTicks % BigInt(CHECKPOINT_INTERVAL) === 0n) {
        const snapshot = new Uint8Array(cpu.save());
        cpu.dispose();
        cpu = qe.createCpu(Model.nes);
        tick = cpu.load(snapshot);
      }

      if (isOpcodeFetchStatus(tickStatus(tick))) {
        ++opcodeCycles;
        if (opcodeCycles > MAX_OPCODE_CYCLES) {
          message = "Test fail, takes too many cycles!";
          break;
        }
      }
    }

    finalState = captureState(cpu);
  } finally {
    cpu.dispose();
  }

  return {
    pass,
    message,
    busTicks,
    opcodeCycles,
    finalState,
    memory,
  };
}

function compareMemory(expected, actual) {
  assert.equal(actual.length, expected.length, "memory length");
  for (let i = 0; i < expected.length; ++i) {
    if (actual[i] !== expected[i]) {
      assert.fail(
        `memory mismatch at 0x${i.toString(16).padStart(4, "0")}: ` +
          `expected 0x${expected[i].toString(16).padStart(2, "0")}, ` +
          `got 0x${actual[i].toString(16).padStart(2, "0")}`,
      );
    }
  }
}

function compareRuns(reference, checkpointed) {
  assert.equal(reference.pass, true, `reference run failed: ${reference.message}`);
  assert.equal(checkpointed.pass, true, `checkpointed run failed: ${checkpointed.message}`);
  assert.deepEqual(checkpointed.finalState, reference.finalState, "final CPU/tick state");
  assert.equal(checkpointed.busTicks, reference.busTicks, "bus tick count");
  assert.equal(checkpointed.opcodeCycles, reference.opcodeCycles, "opcode cycle count");
  compareMemory(reference.memory, checkpointed.memory);
  assert.equal(checkpointed.memory[0x0200], 0x3c, "NES ADC must ignore decimal mode");
  assert.equal(checkpointed.memory[0x0201], 0x00, "NES loop result");
}

const { wasmPath } = parseCommandLine(process.argv);
const qe = await loadQe6502Node(wasmPath);
const initialMemory = new Uint8Array(MEMORY_SIZE);
installNesFunctionalProgram(initialMemory);

const reference = runNesFunctional(qe, initialMemory, false);
const checkpointed = runNesFunctional(qe, initialMemory, true);
compareRuns(reference, checkpointed);

console.log(
  `NES functional JS save/load checkpoint test [PASS] OK ` +
    `(${checkpointed.busTicks} bus ticks, ${checkpointed.opcodeCycles} opcode cycles, ` +
    `checkpoint every ${CHECKPOINT_INTERVAL} ticks)`,
);
