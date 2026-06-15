import { readFile } from "node:fs/promises";
import { fileURLToPath } from "node:url";

import { loadQe6502Node, Model, Status } from "../../binds/js/qe6502.js";

const STANDARD_ROM = "6502_functional_test.hex";
const EXTENDED_ROM = "65C02_extended_opcodes_test.hex";

const STANDARD_SUCCESS_ADDRESS = 0x3469;
const STANDARD_EXPECTED_CYCLES = 30646176;
const EXTENDED_SUCCESS_ADDRESS = 0x24f1;
const EXTENDED_EXPECTED_CYCLES = 21986985;

const ADDRESS_MASK = 0xffff;
const STATUS_SHIFT = 16;
const BUS_SHIFT = 24;
const START_ADDRESS = 0x0400;
const MEMORY_SIZE = 0x10000;

const defaultWasmUrl = new URL(
  "../../build/release_wasm/binds/js/qe6502_js.wasm",
  import.meta.url,
);
const defaultRomDirUrl = new URL("../klaus2m5/", import.meta.url);

const MODEL_INFO = Object.freeze({
  nmos: { model: Model.nmos, displayName: "NMOS 6502" },
  mos: { model: Model.nmos, displayName: "NMOS 6502" },
  wdc: { model: Model.wdc, displayName: "WDC 65C02" },
  rw: { model: Model.rw, displayName: "Rockwell 65C02" },
  rockwell: { model: Model.rw, displayName: "Rockwell 65C02" },
  st: { model: Model.st, displayName: "Synertek 65C02" },
  synertek: { model: Model.st, displayName: "Synertek 65C02" },
});

const DEFAULT_SUITE = Object.freeze([
  ["nmos", "standard"],
  ["wdc", "standard"],
  ["wdc", "extended"],
  ["rw", "standard"],
  ["rw", "extended"],
  ["st", "standard"],
]);

function usage(scriptName) {
  return (
    `Usage: node ${scriptName} <wasm> <rom-dir> [<model> <test>]\n` +
    `       node ${scriptName}\n\n` +
    `With no model/test arguments, runs the default v2 Klaus suite.\n\n` +
    `Models: nmos, wdc, rw, st\n` +
    `Tests: standard, extended\n`
  );
}

function parseModel(modelName) {
  const info = MODEL_INFO[modelName];

  if (info === undefined) {
    throw new Error(`Unknown model: ${modelName}`);
  }

  return info;
}

function parseTest(testName, modelInfo) {
  if (testName !== "standard" && testName !== "extended") {
    throw new Error(`Unknown test: ${testName}`);
  }

  if (
    testName === "extended" &&
    modelInfo.model !== Model.wdc &&
    modelInfo.model !== Model.rw
  ) {
    throw new Error(
      `Extended test is only valid for WDC/Rockwell 65C02 v2 models, not ${modelInfo.displayName}.`,
    );
  }
}

function romPath(romDir, fileName) {
  return new URL(fileName, romDir.endsWith("/") ? `file://${romDir}` : `file://${romDir}/`);
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

  return image;
}

function testConfig(testName, roms) {
  if (testName === "standard") {
    return {
      image: roms.standard,
      successAddress: STANDARD_SUCCESS_ADDRESS,
      expectedCycles: STANDARD_EXPECTED_CYCLES,
    };
  }

  return {
    image: roms.extended,
    successAddress: EXTENDED_SUCCESS_ADDRESS,
    expectedCycles: EXTENDED_EXPECTED_CYCLES,
  };
}

function secondsBetween(start, stop) {
  return Number(stop - start) / 1_000_000_000;
}

function emulatedMhz(busTicks, seconds) {
  if (seconds <= 0) {
    return 0;
  }

  return busTicks / seconds / 1_000_000;
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

function runKlausTest(qe, roms, modelName, testName) {
  const modelInfo = parseModel(modelName);
  parseTest(testName, modelInfo);

  const { image, successAddress, expectedCycles } = testConfig(testName, roms);
  const memory = new Uint8Array(image);
  const cpu = qe.createCpu(modelInfo.model);

  let pass = false;
  let resultMessage = "CPU Error";
  let opcodeCycles = 0;
  let busTicks = 0;
  let mhz = 0;

  try {
    cpu.restart();
    let tick = cpu.jumpTo(START_ADDRESS);
    const start = process.hrtime.bigint();

    for (;;) {
      const address = tickAddress(tick);
      const status = tickStatus(tick);
      let data = isWriteStatus(status) ? tickBus(tick) : memory[address];

      if (address === successAddress) {
        pass = true;
        resultMessage = "OK";
        break;
      }

      if (isWriteStatus(status)) {
        memory[address] = data;
      } else {
        data = memory[address];
      }

      tick = cpu.tick(data);
      ++busTicks;

      if (isOpcodeFetchStatus(tickStatus(tick))) {
        ++opcodeCycles;
        if (opcodeCycles > 2 * expectedCycles) {
          resultMessage = "Test fail, takes too many cycles!";
          break;
        }
      }
    }

    const stop = process.hrtime.bigint();
    mhz = emulatedMhz(busTicks, secondsBetween(start, stop));
  } finally {
    cpu.dispose();
  }

  console.log(
    `${modelInfo.displayName} CPU ${testName} JS test ${pass ? "[PASS]" : "[FAIL]"} ` +
      `${resultMessage} (${mhz.toFixed(2)} MHz, ${busTicks} bus ticks, ${opcodeCycles} opcode cycles)`,
  );

  return pass;
}

function parseCommandLine(argv) {
  const scriptName = argv[1] ?? "js_klaus2m5.mjs";
  const args = argv.slice(2);

  if (args.length === 0) {
    return {
      wasmPath: fileURLToPath(defaultWasmUrl),
      romDir: fileURLToPath(defaultRomDirUrl),
      suite: DEFAULT_SUITE,
    };
  }

  if (args.length === 2) {
    return {
      wasmPath: args[0],
      romDir: args[1],
      suite: DEFAULT_SUITE,
    };
  }

  if (args.length === 4) {
    return {
      wasmPath: args[0],
      romDir: args[1],
      suite: [[args[2], args[3]]],
    };
  }

  throw new Error(usage(scriptName));
}

try {
  const { wasmPath, romDir, suite } = parseCommandLine(process.argv);
  const qe = await loadQe6502Node(wasmPath);
  const roms = {
    standard: await loadHexRom(romPath(romDir, STANDARD_ROM)),
    extended: await loadHexRom(romPath(romDir, EXTENDED_ROM)),
  };

  let failures = 0;
  for (const [modelName, testName] of suite) {
    if (!runKlausTest(qe, roms, modelName, testName)) {
      ++failures;
    }
  }

  if (failures !== 0) {
    process.exitCode = 1;
  }
} catch (error) {
  console.error(error instanceof Error ? error.message : error);
  process.exitCode = 1;
}
