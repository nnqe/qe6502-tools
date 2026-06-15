import { Model } from "./qe6502.js";
import { getGithubBaseUrlForModel, parseOpcodeText, sourceFromGithubFull, sourceFromGithubOpcode } from "./single_step_sources.js";

const MEMORY_SIZE = 0x10000;
const INSTRUCTION_CYCLE_LIMIT = 32;
const NMOS_KIL_OPCODES = new Set([0x02, 0x12, 0x22, 0x32, 0x42, 0x52, 0x62, 0x72, 0x92, 0xb2, 0xd2, 0xf2]);
const NMOS_KIL_CYCLES_TO_COMPARE = 2;
const ALL_OPCODE_VALUES = Object.freeze(Array.from({ length: 256 }, (_, index) => index));


export const SingleStepModels = Object.freeze({
  nmos: {
    model: Model.nmos,
    displayName: "NMOS 6502",
    directory: "6502/v1",
  },
  nes: {
    model: Model.nes,
    displayName: "NES 6502",
    directory: "nes6502/v1",
  },
  wdc: {
    model: Model.wdc,
    displayName: "WDC 65C02",
    directory: "wdc65c02/v1",
  },
  rw: {
    model: Model.rw,
    displayName: "Rockwell 65C02",
    directory: "rockwell65c02/v1",
  },
  st: {
    model: Model.st,
    displayName: "Synertek 65C02",
    directory: "synertek65c02/v1",
  },
});

function hex8(value) {
  return (value & 0xff).toString(16).padStart(2, "0").toUpperCase();
}

function jsonU8(value) {
  return Number(value) & 0xff;
}

function jsonU16(value) {
  return Number(value) & 0xffff;
}

function loadRam(memory, ram) {
  for (const entry of ram) {
    memory[jsonU16(entry[0])] = jsonU8(entry[1]);
  }
}

function setInitialState(cpu, initial) {
  cpu.setPc(jsonU16(initial.pc));
  cpu.setS(jsonU8(initial.s));
  cpu.setA(jsonU8(initial.a));
  cpu.setX(jsonU8(initial.x));
  cpu.setY(jsonU8(initial.y));
  cpu.setP(jsonU8(initial.p));
}

function formatCaseName(testCase) {
  return typeof testCase.name === "string" && testCase.name.length !== 0
    ? testCase.name
    : "<unnamed>";
}

function failCase(name, message) {
  throw new Error(`${name}: ${message}`);
}

function compareU8(name, actual, expected, testName) {
  if ((actual & 0xff) !== (expected & 0xff)) {
    failCase(
      testName,
      `${name} mismatch: got 0x${hex8(actual)} expected 0x${hex8(expected)}`,
    );
  }
}

function compareU16(name, actual, expected, testName) {
  if ((actual & 0xffff) !== (expected & 0xffff)) {
    failCase(
      testName,
      `${name} mismatch: got 0x${(actual & 0xffff).toString(16).padStart(4, "0").toUpperCase()} ` +
        `expected 0x${(expected & 0xffff).toString(16).padStart(4, "0").toUpperCase()}`,
    );
  }
}

function compareFinalState(cpu, memory, final, testName) {
  compareU16("PC", cpu.pc(), jsonU16(final.pc), testName);
  compareU8("S", cpu.s(), jsonU8(final.s), testName);
  compareU8("A", cpu.a(), jsonU8(final.a), testName);
  compareU8("X", cpu.x(), jsonU8(final.x), testName);
  compareU8("Y", cpu.y(), jsonU8(final.y), testName);
  compareU8("P", cpu.p(), jsonU8(final.p), testName);

  for (const entry of final.ram) {
    const address = jsonU16(entry[0]);
    const expected = jsonU8(entry[1]);
    if (memory[address] !== expected) {
      failCase(
        testName,
        `RAM[0x${address.toString(16).padStart(4, "0").toUpperCase()}] mismatch: ` +
          `got 0x${hex8(memory[address])} expected 0x${hex8(expected)}`,
      );
    }
  }
}

function compareCycle(cpu, memory, expectedCycle, cycleIndex, testName) {
  const expectedAddress = jsonU16(expectedCycle[0]);
  const expectedData = jsonU8(expectedCycle[1]);
  const expectedType = String(expectedCycle[2]);
  const expectedWrite = expectedType === "write";

  if (cpu.busAddress() !== expectedAddress) {
    failCase(
      testName,
      `cycle ${cycleIndex} address mismatch: got 0x${cpu.busAddress().toString(16).padStart(4, "0").toUpperCase()} ` +
        `expected 0x${expectedAddress.toString(16).padStart(4, "0").toUpperCase()}`,
    );
  }

  if (cpu.isWrite() !== expectedWrite) {
    failCase(
      testName,
      `cycle ${cycleIndex} rw mismatch: got ${cpu.isWrite() ? "write" : "read"} expected ${expectedType}`,
    );
  }

  const actualData = cpu.isWrite() ? cpu.busData() : memory[cpu.busAddress()];
  if (actualData !== expectedData) {
    failCase(
      testName,
      `cycle ${cycleIndex} data mismatch: got 0x${hex8(actualData)} expected 0x${hex8(expectedData)}`,
    );
  }
}

function tickOnce(cpu, memory) {
  const bus = cpu.isWrite() ? cpu.busData() : memory[cpu.busAddress()];
  if (cpu.isWrite()) {
    memory[cpu.busAddress()] = bus;
  }
  cpu.tick(bus);
}

function isNmosKilOpcode(modelInfo, opcode) {
  return (modelInfo.model === Model.nmos || modelInfo.model === Model.nes) && NMOS_KIL_OPCODES.has(opcode & 0xff);
}

function runCase(qe, testCase, modelInfo, opcode, compareCycles) {
  const memory = new Uint8Array(MEMORY_SIZE);
  const cpu = qe.createCpu(modelInfo.model);
  const testName = formatCaseName(testCase);

  try {
    const initial = testCase.initial;
    const final = testCase.final;
    const cycles = testCase.cycles;

    loadRam(memory, initial.ram);
    setInitialState(cpu, initial);
    cpu.jumpTo(cpu.pc());

    let cycleIndex = 0;
    const nmosKilOpcode = isNmosKilOpcode(modelInfo, opcode);
    const cyclesToRun = compareCycles
      ? (nmosKilOpcode ? Math.min(cycles.length, NMOS_KIL_CYCLES_TO_COMPARE) : cycles.length)
      : 0;

    for (;;) {
      if (compareCycles) {
        if (cycleIndex >= cyclesToRun) {
          break;
        }
        compareCycle(cpu, memory, cycles[cycleIndex], cycleIndex, testName);
      }

      tickOnce(cpu, memory);
      ++cycleIndex;

      if (!compareCycles && nmosKilOpcode && cycleIndex >= NMOS_KIL_CYCLES_TO_COMPARE) {
        break;
      }
      if (!compareCycles && cpu.isOpcodeFetch()) {
        break;
      }
      if (!compareCycles && cycleIndex > INSTRUCTION_CYCLE_LIMIT) {
        failCase(testName, "instruction cycle limit exceeded");
      }
    }

    compareFinalState(cpu, memory, final, testName);
    return { busTicks: cycleIndex };
  } finally {
    cpu.dispose();
  }
}

function describeDescriptor(descriptor) {
  return descriptor.url ?? descriptor.name;
}

async function loadDescriptorJson(descriptor) {
  const text = await descriptor.loadText();

  if (text === undefined) {
    return undefined;
  }

  if (String(text).trim().length === 0) {
    return [];
  }

  try {
    return JSON.parse(text);
  } catch (error) {
    const detail = error instanceof Error ? error.message : String(error);
    throw new Error(`${describeDescriptor(descriptor)}: invalid JSON: ${detail}`);
  }
}

async function runOpcode(qe, modelInfo, descriptor, options, counts) {
  const opcode = descriptor.opcode & 0xff;
  const sourceName = describeDescriptor(descriptor);
  options.log(`Loading ${sourceName}`);
  const tests = await loadDescriptorJson(descriptor);

  if (tests === undefined) {
    ++counts.filesSkipped;
    options.log(`SKIP ${hex8(opcode)} (missing)`);
    return;
  }

  if (!Array.isArray(tests)) {
    throw new Error(`${sourceName}: root JSON value is not an array`);
  }

  let localCases = 0;
  for (const testCase of tests) {
    if (options.maxCases !== 0 && localCases >= options.maxCases) {
      break;
    }
    try {
      const result = runCase(qe, testCase, modelInfo, opcode, options.compareCycles);
      counts.busTicks += result.busTicks;
    } catch (error) {
      ++counts.casesFailed;
      throw new Error(
        `FAIL opcode ${hex8(opcode)} case #${localCases}: ` +
          (error instanceof Error ? error.message : String(error)),
      );
    }

    ++localCases;
    ++counts.casesRun;
  }

  ++counts.filesRun;
  options.log(`PASS ${hex8(opcode)} (${localCases} cases)`);
}

function emulatedMhz(busTicks, elapsedMs) {
  if (elapsedMs <= 0) {
    return 0;
  }
  return busTicks / elapsedMs / 1000;
}

async function yieldToBrowser() {
  await new Promise((resolve) => setTimeout(resolve, 0));
}

function normalizeOpcode(value) {
  const text = String(value).trim();
  if (!/^[0-9a-fA-F]{2}$/.test(text)) {
    throw new RangeError(`Opcode must be exactly two hex digits, got '${value}'`);
  }
  return Number.parseInt(text, 16) & 0xff;
}

export async function runSingleStepSuite({
  qe,
  modelName,
  opcode,
  allOpcodes,
  compareCycles,
  maxCases,
  descriptors,
  sourceLabel,
  log,
}) {
  const modelInfo = SingleStepModels[modelName];
  if (modelInfo === undefined) {
    throw new RangeError(`Unknown SingleStep model: ${modelName}`);
  }

  const counts = {
    filesRun: 0,
    filesSkipped: 0,
    casesRun: 0,
    casesFailed: 0,
    busTicks: 0,
  };
  let activeDescriptors = descriptors;
  let activeSourceLabel = sourceLabel;

  if (activeDescriptors === undefined) {
    const baseUrl = getGithubBaseUrlForModel(modelName);
    activeDescriptors = allOpcodes
      ? sourceFromGithubFull(baseUrl)
      : sourceFromGithubOpcode(baseUrl, parseOpcodeText(opcode));
    activeSourceLabel = allOpcodes ? `${baseUrl}00..ff.json` : activeDescriptors[0].url;
  }

  if (!Array.isArray(activeDescriptors) || activeDescriptors.length === 0) {
    throw new Error("No SingleStep opcode file descriptors were provided");
  }

  const options = {
    compareCycles,
    maxCases,
    log,
  };
  const startedAt = performance.now();

  log(`Running SingleStepTests for ${modelInfo.displayName} (${modelInfo.directory})...`);
  log(`Source: ${activeSourceLabel ?? "selected local file(s)"}`);
  log(compareCycles ? "Cycle comparison: enabled" : "Cycle comparison: disabled");
  if (maxCases !== 0) {
    log(`Max cases per opcode: ${maxCases}`);
  }

  if (allOpcodes && activeDescriptors.length === ALL_OPCODE_VALUES.length) {
    log("Full package mode: loading 00..FF opcode descriptors; missing GitHub raw files will be skipped");
  }

  for (const descriptor of activeDescriptors) {
    await runOpcode(qe, modelInfo, descriptor, options, counts);
    await yieldToBrowser();
  }

  if (counts.filesRun === 0) {
    throw new Error(
      `No opcode JSON files were loaded for ${modelInfo.displayName}; check GitHub/network access or use the local file/folder source`,
    );
  }

  const elapsedMs = performance.now() - startedAt;
  const mhz = emulatedMhz(counts.busTicks, elapsedMs);
  log(
    `SUMMARY SingleStep ${modelInfo.displayName} [PASS] ` +
      `files=${counts.filesRun} skipped=${counts.filesSkipped} cases=${counts.casesRun} failed=${counts.casesFailed} ` +
      `busTicks=${counts.busTicks} ` +
      `time=${(elapsedMs / 1000).toFixed(2)} s ` +
      `${mhz.toFixed(2)} MHz`,
  );
  return true;
}
