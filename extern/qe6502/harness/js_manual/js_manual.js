const MANUAL_CACHE_BUSTER = new URL(import.meta.url).searchParams.get("v") ?? `${Date.now()}`;

function withCacheBuster(url) {
  const separator = url.includes("?") ? "&" : "?";
  return `${url}${separator}v=${encodeURIComponent(MANUAL_CACHE_BUSTER)}`;
}

const [qeModule, singleStepModule, singleStepSourcesModule] = await Promise.all([
  import(withCacheBuster("./qe6502.js")),
  import(withCacheBuster("./js_singlestep.js")),
  import(withCacheBuster("./single_step_sources.js")),
]);

const { loadQe6502Browser, Model, Status } = qeModule;
const { runSingleStepSuite } = singleStepModule;
const {
  getGithubBaseUrlForModel,
  parseOpcodeText,
  sourceFromGithubFull,
  sourceFromGithubOpcode,
  sourceFromLocalFile,
  sourceFromLocalFolder,
} = singleStepSourcesModule;

const WASM_URL = withCacheBuster("./qe6502_js.wasm");
const STANDARD_ROM_URL = withCacheBuster("./roms/6502_functional_test.hex");
const EXTENDED_ROM_URL = withCacheBuster("./roms/65C02_extended_opcodes_test.hex");

const STANDARD_SUCCESS_ADDRESS = 0x3469;
const STANDARD_EXPECTED_CYCLES = 30646176;
const EXTENDED_SUCCESS_ADDRESS = 0x24f1;
const EXTENDED_EXPECTED_CYCLES = 21986985;

const ADDRESS_MASK = 0xffff;
const STATUS_SHIFT = 16;
const BUS_SHIFT = 24;
const START_ADDRESS = 0x0400;
const MEMORY_SIZE = 0x10000;

const MODEL_INFO = Object.freeze({
  nmos: { model: Model.nmos, displayName: "NMOS 6502" },
  wdc: { model: Model.wdc, displayName: "WDC 65C02" },
  rw: { model: Model.rw, displayName: "Rockwell 65C02" },
  st: { model: Model.st, displayName: "Synertek 65C02" },
});

const DEFAULT_KLAUS_SUITE = Object.freeze([
  ["nmos", "standard"],
  ["wdc", "standard"],
  ["wdc", "extended"],
  ["rw", "standard"],
  ["rw", "extended"],
  ["st", "standard"],
]);

const output = document.querySelector("#output");
const statusLine = document.querySelector("#status");
const testSelect = document.querySelector("#test-select");
const runButton = document.querySelector("#run-button");
const clearButton = document.querySelector("#clear-button");
const singleStepControls = document.querySelector("#singlestep-controls");
const singleStepModel = document.querySelector("#singlestep-model");
const singleStepScope = document.querySelector("#singlestep-scope");
const singleStepSource = document.querySelector("#singlestep-source");
const singleStepBaseUrl = document.querySelector("#singlestep-base-url");
const singleStepLocalFile = document.querySelector("#singlestep-local-file");
const singleStepLocalFolder = document.querySelector("#singlestep-local-folder");
const singleStepOpcode = document.querySelector("#singlestep-opcode");
const singleStepCycles = document.querySelector("#singlestep-cycles");
const singleStepMaxCases = document.querySelector("#singlestep-max-cases");

let qePromise;
let romsPromise;

log(`Manual harness cache buster: ${MANUAL_CACHE_BUSTER}`);

function log(message = "") {
  output.textContent += `${message}\n`;
  output.scrollTop = output.scrollHeight;
}

function setStatus(message, kind = "") {
  statusLine.textContent = message;
  statusLine.className = kind;
}

function hex8(value) {
  return (value & 0xff).toString(16).padStart(2, "0").toUpperCase();
}

function hex16(value) {
  return (value & 0xffff).toString(16).padStart(4, "0").toUpperCase();
}

function assertEqual(actual, expected, label) {
  if (actual !== expected) {
    throw new Error(`${label}: expected ${expected}, got ${actual}`);
  }
}

function assertTrue(value, label) {
  if (!value) {
    throw new Error(`${label}: expected true`);
  }
}

function assertThrows(callback, expectedError, label) {
  try {
    callback();
  } catch (error) {
    if (error instanceof expectedError) {
      return;
    }

    throw new Error(`${label}: expected ${expectedError.name}, got ${error}`);
  }

  throw new Error(`${label}: expected ${expectedError.name}`);
}

async function qe6502() {
  if (qePromise === undefined) {
    qePromise = loadQe6502Browser(WASM_URL);
  }

  return qePromise;
}

async function fetchText(url) {
  const response = await fetch(url, { cache: "no-store" });

  if (!response.ok) {
    throw new Error(`Failed to fetch ${url}: ${response.status} ${response.statusText}`);
  }

  return response.text();
}

async function loadHexRom(url) {
  const text = await fetchText(url);
  const matches = text.match(/0x[0-9a-fA-F]+/g) ?? [];

  if (matches.length !== MEMORY_SIZE) {
    throw new Error(`${url} contains ${matches.length} bytes; expected ${MEMORY_SIZE}`);
  }

  const image = new Uint8Array(MEMORY_SIZE);
  for (let i = 0; i < matches.length; ++i) {
    image[i] = Number.parseInt(matches[i], 16) & 0xff;
  }

  return image;
}

async function klausRoms() {
  if (romsPromise === undefined) {
    romsPromise = Promise.all([
      loadHexRom(STANDARD_ROM_URL),
      loadHexRom(EXTENDED_ROM_URL),
    ]).then(([standard, extended]) => ({ standard, extended }));
  }

  return romsPromise;
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

function emulatedMhz(busTicks, milliseconds) {
  if (milliseconds <= 0) {
    return 0;
  }

  return busTicks / milliseconds / 1000;
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

async function runSmokeTest() {
  const qe = await qe6502();
  const cpu = qe.createCpu(Model.nmos);

  try {
    log("Running qe6502 browser smoke test...");
    const restartTick = cpu.restart();

    assertEqual(restartTick >>> 0, restartTick, "restart tick is uint32");
    assertEqual(cpu.cpuModel(), Model.nmos, "model");
    assertEqual(cpu.busAddress(), 0x00ff, "restart bus address");
    assertEqual(cpu.busData(), 0x00, "restart bus data");
    assertEqual(cpu.busStatus(), Status.internalReset, "restart bus status");
    assertEqual(cpu.isWrite(), false, "restart isWrite");
    assertEqual(cpu.isOpcodeFetch(), false, "restart isOpcodeFetch");
    assertEqual(cpu.isInternalReset(), true, "restart isInternalReset");
    assertEqual(cpu.isJammed(), false, "restart isJammed");

    assertEqual(cpu.isIrqAsserted(), false, "initial IRQ asserted state");
    cpu.irqAssert(true);
    assertEqual(cpu.isIrqAsserted(), true, "asserted IRQ state");
    cpu.irqAssert(false);
    assertEqual(cpu.isIrqAsserted(), false, "deasserted IRQ state");
    assertThrows(() => cpu.irqAssert(1), TypeError, "IRQ assert rejects non-boolean input");

    assertEqual(cpu.isNmiAsserted(), false, "initial NMI asserted state");
    cpu.nmiAssert(true);
    assertEqual(cpu.isNmiAsserted(), true, "asserted NMI state");
    cpu.nmiAssert(false);
    assertEqual(cpu.isNmiAsserted(), false, "deasserted NMI state");
    assertThrows(() => cpu.nmiAssert(1), TypeError, "NMI assert rejects non-boolean input");

    cpu.setPc(0x1234);
    cpu.setA(0x56);
    cpu.setX(0x78);
    cpu.setY(0x9a);
    cpu.setS(0xbc);
    cpu.setP(0xde);

    const snapshot = cpu.save();
    assertEqual(snapshot.byteLength, 64, "snapshot size");
    assertEqual(cpu.pc(), 0x1234, "PC before load");
    assertEqual(cpu.a(), 0x56, "A before load");
    assertEqual(cpu.x(), 0x78, "X before load");
    assertEqual(cpu.y(), 0x9a, "Y before load");
    assertEqual(cpu.s(), 0xbc, "S before load");
    assertEqual(cpu.p(), 0xde, "P before load");

    cpu.setPc(0);
    cpu.setA(0);
    cpu.setX(0);
    cpu.setY(0);
    cpu.setS(0);
    cpu.setP(0);

    const loadedTick = cpu.load(snapshot);
    assertEqual(loadedTick, restartTick, "loaded tick");
    assertEqual(cpu.pc(), 0x1234, "PC after load");
    assertEqual(cpu.a(), 0x56, "A after load");
    assertEqual(cpu.x(), 0x78, "X after load");
    assertEqual(cpu.y(), 0x9a, "Y after load");
    assertEqual(cpu.s(), 0xbc, "S after load");
    assertEqual(cpu.p(), 0xde, "P after load");

    cpu.jumpTo(0x2000);
    assertEqual(cpu.busAddress(), 0x2000, "jump bus address");
    assertTrue((cpu.busStatus() & Status.fetch) !== 0, "jump opcode fetch");

    log("qe6502 browser smoke test [PASS] OK");
    return true;
  } finally {
    cpu.dispose();
  }
}

function runKlausTest(qe, roms, modelName, testName) {
  const modelInfo = MODEL_INFO[modelName];
  const { image, successAddress, expectedCycles } = testConfig(testName, roms);
  const memory = new Uint8Array(image);
  const cpu = qe.createCpu(modelInfo.model);

  let pass = false;
  let resultMessage = "CPU Error";
  let opcodeCycles = 0;
  let busTicks = 0;
  let mhz = 0;
  let stop = 0;

  try {
    cpu.restart();
    let tick = cpu.jumpTo(START_ADDRESS);
    const start = performance.now();

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

    stop = performance.now();
    mhz = emulatedMhz(busTicks, stop - start);
  } finally {
    cpu.dispose();
  }

  const line =
    `${modelInfo.displayName} CPU ${testName} browser JS test ${pass ? "[PASS]" : "[FAIL]"} ` +
    `${resultMessage} (${mhz.toFixed(2)} MHz, ${busTicks} bus ticks, ${opcodeCycles} opcode cycles)`;

  log(line);
  return pass;
}

async function runKlausSuite() {
  log("Loading qe6502 WASM and Klaus2m5 ROM images...");
  const [qe, roms] = await Promise.all([qe6502(), klausRoms()]);

  let failures = 0;
  log("Running Klaus2m5 browser JS suite...");

  for (const [modelName, testName] of DEFAULT_KLAUS_SUITE) {
    if (!runKlausTest(qe, roms, modelName, testName)) {
      ++failures;
    }

    await new Promise((resolve) => setTimeout(resolve, 0));
  }

  if (failures !== 0) {
    log(`Klaus2m5 browser JS suite [FAIL] ${failures} failing test(s)`);
    return false;
  }

  log("Klaus2m5 browser JS suite [PASS] OK");
  return true;
}

function singleStepMaxCasesValue() {
  const value = Number.parseInt(singleStepMaxCases.value, 10);
  if (!Number.isFinite(value) || value < 0) {
    throw new RangeError("Max cases/opcode must be 0 or a positive integer");
  }
  return value;
}

function updateSingleStepBaseUrl() {
  singleStepBaseUrl.value = getGithubBaseUrlForModel(singleStepModel.value);
}

function updateSingleStepUi() {
  const singleStepSelected = testSelect.value === "singlestep";
  const source = singleStepSource.value;
  const opcodeMode = singleStepScope.value === "opcode";

  singleStepControls.hidden = !singleStepSelected;
  singleStepOpcode.disabled = !opcodeMode;
  singleStepBaseUrl.hidden = source !== "github";
  singleStepLocalFile.hidden = source !== "local-file";
  singleStepLocalFolder.hidden = source !== "local-folder";
}

function setControlsDisabled(disabled) {
  runButton.disabled = disabled;
  testSelect.disabled = disabled;
  singleStepModel.disabled = disabled;
  singleStepScope.disabled = disabled;
  singleStepSource.disabled = disabled;
  singleStepBaseUrl.disabled = disabled;
  singleStepLocalFile.disabled = disabled;
  singleStepLocalFolder.disabled = disabled;
  singleStepOpcode.disabled = disabled || singleStepScope.value !== "opcode";
  singleStepCycles.disabled = disabled;
  singleStepMaxCases.disabled = disabled;
}

function singleStepDescriptorsFromUi() {
  const allOpcodes = singleStepScope.value === "all";
  const opcode = parseOpcodeText(singleStepOpcode.value);

  if (singleStepSource.value === "github") {
    const baseUrl = singleStepBaseUrl.value;
    return {
      descriptors: allOpcodes
        ? sourceFromGithubFull(baseUrl)
        : sourceFromGithubOpcode(baseUrl, opcode),
      sourceLabel: allOpcodes ? `${baseUrl.replace(/\/+$/u, "")}/00..ff.json` : `${baseUrl.replace(/\/+$/u, "")}/${opcode.toString(16).padStart(2, "0")}.json`,
    };
  }

  if (singleStepSource.value === "local-file") {
    if (!singleStepLocalFile.files || singleStepLocalFile.files.length !== 1) {
      throw new Error("Choose one local opcode JSON file named like 00.json..ff.json");
    }

    return {
      descriptors: sourceFromLocalFile(singleStepLocalFile.files[0]),
      sourceLabel: singleStepLocalFile.files[0].name,
    };
  }

  if (singleStepSource.value === "local-folder") {
    if (!singleStepLocalFolder.files || singleStepLocalFolder.files.length === 0) {
      throw new Error("Choose a local SingleStepTests folder containing opcode JSON files");
    }

    let descriptors = sourceFromLocalFolder(singleStepLocalFolder.files);
    if (!allOpcodes) {
      descriptors = descriptors.filter((descriptor) => descriptor.opcode === opcode);
      if (descriptors.length === 0) {
        throw new Error(`Selected folder does not contain ${opcode.toString(16).padStart(2, "0")}.json`);
      }
    }

    return {
      descriptors,
      sourceLabel: allOpcodes ? "selected local folder" : descriptors[0].name,
    };
  }

  throw new Error(`Unknown SingleStep source: ${singleStepSource.value}`);
}

async function runSingleStepTest() {
  const qe = await qe6502();
  const { descriptors, sourceLabel } = singleStepDescriptorsFromUi();

  return runSingleStepSuite({
    qe,
    modelName: singleStepModel.value,
    opcode: singleStepOpcode.value,
    allOpcodes: singleStepScope.value === "all",
    compareCycles: singleStepCycles.checked,
    maxCases: singleStepMaxCasesValue(),
    descriptors,
    sourceLabel,
    log,
  });
}

async function runSelectedTest() {
  setControlsDisabled(true);
  setStatus("Running...");

  try {
    const startedAt = performance.now();
    let pass = false;

    if (testSelect.value === "smoke") {
      pass = await runSmokeTest();
    } else if (testSelect.value === "klaus") {
      pass = await runKlausSuite();
    } else {
      pass = await runSingleStepTest();
    }

    const elapsed = (performance.now() - startedAt) / 1000;
    setStatus(pass ? `PASS in ${elapsed.toFixed(2)} s` : `FAIL in ${elapsed.toFixed(2)} s`, pass ? "pass" : "fail");
  } catch (error) {
    log(error instanceof Error ? `${error.name}: ${error.message}` : String(error));
    if (error instanceof Error && error.stack !== undefined) {
      log(error.stack);
    }
    if (testSelect.value === "singlestep") {
      log("SingleStepTests can be loaded from GitHub raw URLs or from local opcode JSON files/folders. If browser fetch reports a network TypeError, switch Source to Local folder and choose the downloaded SingleStepTests model folder.");
    }
    setStatus("FAIL", "fail");
  } finally {
    setControlsDisabled(false);
    updateSingleStepUi();
  }
}

testSelect.addEventListener("change", updateSingleStepUi);
singleStepScope.addEventListener("change", updateSingleStepUi);
singleStepSource.addEventListener("change", updateSingleStepUi);
singleStepModel.addEventListener("change", () => {
  updateSingleStepBaseUrl();
  updateSingleStepUi();
});

runButton.addEventListener("click", () => {
  void runSelectedTest();
});

clearButton.addEventListener("click", () => {
  output.textContent = "";
  setStatus("Ready.");
});

updateSingleStepBaseUrl();
updateSingleStepUi();
log("Ready. Select a test and press Run selected test.");
