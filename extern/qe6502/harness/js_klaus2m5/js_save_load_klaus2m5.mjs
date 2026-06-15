import assert from "node:assert/strict";
import { fileURLToPath, pathToFileURL } from "node:url";
import { readFile } from "node:fs/promises";
import { loadQe6502Node, Model, Status } from "../../binds/js/qe6502.js";

const START_ADDRESS = 0x0400;
const CHECKPOINT_INTERVAL = 256;
const EXTENDED_ROM = "65C02_extended_opcodes_test.hex";
const EXTENDED_SUCCESS_ADDRESS = 0x24f1;
const EXTENDED_EXPECTED_CYCLES = 21986985n;
const MEMORY_SIZE = 0x10000;
const ADDRESS_MASK = 0xffff;
const STATUS_SHIFT = 16;
const BUS_SHIFT = 24;
const defaultWasmUrl = new URL("../../build/release_wasm/binds/js/qe6502_js.wasm", import.meta.url);
const defaultRomDirUrl = new URL("../klaus2m5/", import.meta.url);

function usage(scriptName) {
  return (
    `Usage: node ${scriptName} [<qe6502_shared.wasm> <klaus-rom-dir>]\n\n` +
    `Runs the RW extended Klaus2m5 save/load checkpoint test. The checkpointed ` +
    `run saves and reloads CPU state every ${CHECKPOINT_INTERVAL} bus ticks.`
  );
}

function parseCommandLine(argv) {
  const scriptName = argv[1] ?? "js_save_load_klaus2m5.mjs";
  const args = argv.slice(2);

  if (args.length === 0) {
    return {
      wasmPath: fileURLToPath(defaultWasmUrl),
      romDir: fileURLToPath(defaultRomDirUrl),
    };
  }

  if (args.length === 2) {
    return {
      wasmPath: args[0],
      romDir: args[1],
    };
  }

  throw new Error(usage(scriptName));
}

function romPath(romDir, fileName) {
  const base = pathToFileURL(romDir.endsWith("/") ? romDir : `${romDir}/`);
  return new URL(fileName, base);
}

async function loadHexRom(fileUrl) {
  const text = await readFile(fileUrl, "utf8");
  const matches = text.match(/0x[0-9a-fA-F]+/g) ?? [];

  if (matches.length !== MEMORY_SIZE) {
    throw new Error(
      `${fileURLToPath(fileUrl)} contains ${matches.length} bytes; expected ${MEMORY_SIZE}`,
    );
  }

  const image = new Uint8Array(MEMORY_SIZE);
  for (let i = 0; i < matches.length; ++i) {
    image[i] = Number.parseInt(matches[i], 16) & 0xff;
  }

  return {
    image,
    successAddress: EXTENDED_SUCCESS_ADDRESS,
    expectedCycles: EXTENDED_EXPECTED_CYCLES,
  };
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

function runKlausRwExtended(qe, rom, checkpointed) {
  const memory = new Uint8Array(rom.image);
  let cpu = qe.createCpu(Model.rw);
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

      if (address === rom.successAddress) {
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
        cpu = qe.createCpu(Model.rw);
        tick = cpu.load(snapshot);
      }

      if (isOpcodeFetchStatus(tickStatus(tick))) {
        ++opcodeCycles;
        if (opcodeCycles > 2n * rom.expectedCycles) {
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
}

const { wasmPath, romDir } = parseCommandLine(process.argv);
const qe = await loadQe6502Node(wasmPath);
const rom = await loadHexRom(romPath(romDir, EXTENDED_ROM));

const reference = runKlausRwExtended(qe, rom, false);
const checkpointed = runKlausRwExtended(qe, rom, true);
compareRuns(reference, checkpointed);

console.log(
  `Rockwell 65C02 extended JS save/load checkpoint test [PASS] OK ` +
    `(${checkpointed.busTicks} bus ticks, ${checkpointed.opcodeCycles} opcode cycles, ` +
    `checkpoint every ${CHECKPOINT_INTERVAL} ticks)`,
);
