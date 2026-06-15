export const Model = Object.freeze({
  nmos: 0,
  nes: 1,
  wdc: 2,
  rw: 3,
  st: 4,
});

export const Flag = Object.freeze({
  c: 1 << 0,
  z: 1 << 1,
  i: 1 << 2,
  d: 1 << 3,
  b: 1 << 4,
  un: 1 << 5,
  v: 1 << 6,
  n: 1 << 7,
});

export const Status = Object.freeze({
  writing: 1 << 0,
  fetch: 1 << 1,
  internalReset: 1 << 6,
  cpuJammed: 1 << 7,
});

function configuredInteger(configuredValue) {
  const value = Number(configuredValue);
  return Number.isInteger(value) ? value : null;
}

const ABI_VERSION_MAJOR = configuredInteger("@QE6502_VERSION_MAJOR@");
const ABI_VERSION_MINOR = configuredInteger("@QE6502_VERSION_MINOR@");
const SNAPSHOT_SIZE = 64;

const TICK_ADDRESS_MASK = 0xffff;
const TICK_STATUS_SHIFT = 16;
const TICK_BUS_SHIFT = 24;

const REQUIRED_EXPORTS = [
  "qe6502js_context_pool_reset",
  "qe6502js_context_alloc",
  "qe6502js_context_free",
  "qe6502abi_version",
  "qe6502abi_setup",
  "qe6502abi_restart",
  "qe6502abi_goto",
  "qe6502abi_irq_assert",
  "qe6502abi_is_irq_asserted",
  "qe6502abi_save",
  "qe6502abi_load",
  "qe6502abi_nmi_assert",
  "qe6502abi_is_nmi_asserted",
  "qe6502abi_tick",
  "qe6502abi_get_pc",
  "qe6502abi_set_pc",
  "qe6502abi_get_s",
  "qe6502abi_set_s",
  "qe6502abi_get_a",
  "qe6502abi_set_a",
  "qe6502abi_get_x",
  "qe6502abi_set_x",
  "qe6502abi_get_y",
  "qe6502abi_set_y",
  "qe6502abi_get_p",
  "qe6502abi_set_p",
  "qe6502abi_get_model",
  "qe6502abi_set_model",
];

function bytesFromBufferSource(source) {
  if (source instanceof ArrayBuffer) {
    return source;
  }

  if (ArrayBuffer.isView(source)) {
    return new Uint8Array(source.buffer, source.byteOffset, source.byteLength);
  }

  throw new TypeError("Expected an ArrayBuffer or typed array");
}

function isUrl(value) {
  return typeof URL !== "undefined" && value instanceof URL;
}

function normalizeModel(value) {
  const model = Number(value);

  if (!Number.isInteger(model) || model < Model.nmos || model > Model.st) {
    throw new RangeError(`Invalid qe6502 model: ${value}`);
  }

  return model >>> 0;
}

function byte(value) {
  return Number(value) & 0xff;
}

function bool(value) {
  if (typeof value !== "boolean") {
    throw new TypeError("Expected a boolean");
  }

  return value ? 1 : 0;
}

function word(value) {
  return Number(value) & 0xffff;
}

function hex8(value) {
  return (value & 0xff).toString(16).padStart(2, "0").toUpperCase();
}

function hex16(value) {
  return (value & 0xffff).toString(16).padStart(4, "0").toUpperCase();
}

function snapshotBytes(value) {
  if (value instanceof ArrayBuffer) {
    return new Uint8Array(value);
  }

  if (ArrayBuffer.isView(value)) {
    return new Uint8Array(value.buffer, value.byteOffset, value.byteLength);
  }

  throw new TypeError("Snapshot must be an ArrayBuffer or typed array");
}

function requireExports(exports) {
  if (!(exports.memory instanceof WebAssembly.Memory)) {
    throw new Error("Missing required WASM export: memory");
  }

  for (const name of REQUIRED_EXPORTS) {
    if (typeof exports[name] !== "function") {
      throw new Error(`Missing required WASM export: ${name}`);
    }
  }
}

async function instantiateQe6502(bytes) {
  const { instance } = await WebAssembly.instantiate(bytes, {});
  const exports = instance.exports;

  requireExports(exports);

  const version = exports.qe6502abi_version() >>> 0;
  const major = (version >>> 16) & 0xffff;
  const minor = version & 0xffff;

  const expectedMajor = ABI_VERSION_MAJOR === null ? major : ABI_VERSION_MAJOR;
  const expectedMinor = ABI_VERSION_MINOR === null ? minor : ABI_VERSION_MINOR;

  if (major !== expectedMajor || minor < expectedMinor) {
    throw new Error(
      `Unsupported qe6502 ABI version: 0x${version.toString(16).padStart(8, "0")}; ` +
        `expected major ${expectedMajor} with minor >= ${expectedMinor}`,
    );
  }

  exports.qe6502js_context_pool_reset();

  return new Qe6502(exports);
}

export async function loadQe6502Browser(source) {
  if (typeof source === "string" || isUrl(source)) {
    const response = await fetch(source, { cache: "no-store" });

    if (!response.ok) {
      throw new Error(
        `Failed to fetch WASM module: ${response.status} ${response.statusText}`,
      );
    }

    return instantiateQe6502(await response.arrayBuffer());
  }

  return instantiateQe6502(bytesFromBufferSource(source));
}

export async function loadQe6502Node(
  source = new URL("./qe6502_js.wasm", import.meta.url),
) {
  if (typeof source === "string" || isUrl(source)) {
    const { readFile } = await import("node:fs/promises");
    return instantiateQe6502(await readFile(source));
  }

  return instantiateQe6502(bytesFromBufferSource(source));
}

export class Qe6502 {
  #runtime;

  constructor(exports) {
    requireExports(exports);
    this.#runtime = Object.freeze({
      exports,
      memory: exports.memory,
    });
  }

  createCpu(cpuModel = Model.nmos) {
    const ptr = this.#runtime.exports.qe6502js_context_alloc() >>> 0;

    if (ptr === 0) {
      throw new Error("qe6502js_context_alloc returned null");
    }

    try {
      return new Qe6502Cpu(this.#runtime, ptr, normalizeModel(cpuModel));
    } catch (error) {
      this.#runtime.exports.qe6502js_context_free(ptr);
      throw error;
    }
  }
}

export class Qe6502Cpu {
  #runtime;
  #ptr;
  #tick = 0;
  #disposed = false;

  constructor(runtime, ptr, cpuModel) {
    this.#runtime = runtime;
    this.#ptr = ptr >>> 0;
    this.#runtime.exports.qe6502abi_setup(this.#ptr, cpuModel);
  }

  restart() {
    this.#assertAlive();
    return this.#storeTick(this.#runtime.exports.qe6502abi_restart(this.#ptr));
  }

  jumpTo(address) {
    this.#assertAlive();
    return this.#storeTick(
      this.#runtime.exports.qe6502abi_goto(this.#ptr, word(address)),
    );
  }

  irqAssert(assertIrq) {
    this.#assertAlive();
    this.#runtime.exports.qe6502abi_irq_assert(this.#ptr, bool(assertIrq));
  }

  isIrqAsserted() {
    this.#assertAlive();
    return (this.#runtime.exports.qe6502abi_is_irq_asserted(this.#ptr) & 0xff) !== 0;
  }

  nmiAssert(assertNmi) {
    this.#assertAlive();
    this.#runtime.exports.qe6502abi_nmi_assert(this.#ptr, bool(assertNmi));
  }

  isNmiAsserted() {
    this.#assertAlive();
    return (this.#runtime.exports.qe6502abi_is_nmi_asserted(this.#ptr) & 0xff) !== 0;
  }

  tick(input = 0) {
    this.#assertAlive();
    return this.#storeTick(
      this.#runtime.exports.qe6502abi_tick(this.#ptr, byte(input)),
    );
  }

  busAddress() {
    this.#assertAlive();
    return this.#tick & TICK_ADDRESS_MASK;
  }

  busData() {
    this.#assertAlive();
    return (this.#tick >>> TICK_BUS_SHIFT) & 0xff;
  }

  busStatus() {
    this.#assertAlive();
    return (this.#tick >>> TICK_STATUS_SHIFT) & 0xff;
  }

  isWrite() {
    this.#assertAlive();
    return (this.busStatus() & Status.writing) !== 0;
  }

  isOpcodeFetch() {
    this.#assertAlive();
    return (this.busStatus() & Status.fetch) !== 0;
  }

  isInternalReset() {
    this.#assertAlive();
    return (this.busStatus() & Status.internalReset) !== 0;
  }

  isJammed() {
    this.#assertAlive();
    return (this.busStatus() & Status.cpuJammed) !== 0;
  }

  save() {
    this.#assertAlive();

    const scratch = this.#allocSnapshotScratch();

    try {
      this.#runtime.exports.qe6502abi_save(this.#ptr, this.#tick, scratch);
      return new Uint8Array(
        new Uint8Array(this.#runtime.memory.buffer, scratch, SNAPSHOT_SIZE),
      );
    } finally {
      this.#runtime.exports.qe6502js_context_free(scratch);
    }
  }

  load(snapshot) {
    this.#assertAlive();

    const source = snapshotBytes(snapshot);
    if (source.byteLength !== SNAPSHOT_SIZE) {
      throw new RangeError(`Snapshot must be exactly ${SNAPSHOT_SIZE} bytes`);
    }

    const scratch = this.#allocSnapshotScratch();

    try {
      new Uint8Array(this.#runtime.memory.buffer, scratch, SNAPSHOT_SIZE).set(
        source,
      );
      return this.#storeTick(
        this.#runtime.exports.qe6502abi_load(this.#ptr, scratch),
      );
    } finally {
      this.#runtime.exports.qe6502js_context_free(scratch);
    }
  }

  cpuModel() {
    this.#assertAlive();
    return this.#runtime.exports.qe6502abi_get_model(this.#ptr) & 0xff;
  }

  pc() {
    this.#assertAlive();
    return this.#runtime.exports.qe6502abi_get_pc(this.#ptr) & 0xffff;
  }

  setPc(value) {
    this.#assertAlive();
    this.#runtime.exports.qe6502abi_set_pc(this.#ptr, word(value));
  }

  s() {
    this.#assertAlive();
    return this.#runtime.exports.qe6502abi_get_s(this.#ptr) & 0xff;
  }

  setS(value) {
    this.#assertAlive();
    this.#runtime.exports.qe6502abi_set_s(this.#ptr, byte(value));
  }

  a() {
    this.#assertAlive();
    return this.#runtime.exports.qe6502abi_get_a(this.#ptr) & 0xff;
  }

  setA(value) {
    this.#assertAlive();
    this.#runtime.exports.qe6502abi_set_a(this.#ptr, byte(value));
  }

  x() {
    this.#assertAlive();
    return this.#runtime.exports.qe6502abi_get_x(this.#ptr) & 0xff;
  }

  setX(value) {
    this.#assertAlive();
    this.#runtime.exports.qe6502abi_set_x(this.#ptr, byte(value));
  }

  y() {
    this.#assertAlive();
    return this.#runtime.exports.qe6502abi_get_y(this.#ptr) & 0xff;
  }

  setY(value) {
    this.#assertAlive();
    this.#runtime.exports.qe6502abi_set_y(this.#ptr, byte(value));
  }

  p() {
    this.#assertAlive();
    return this.#runtime.exports.qe6502abi_get_p(this.#ptr) & 0xff;
  }

  setP(value) {
    this.#assertAlive();
    this.#runtime.exports.qe6502abi_set_p(this.#ptr, byte(value));
  }

  dispose() {
    if (this.#disposed) {
      return;
    }

    this.#runtime.exports.qe6502js_context_free(this.#ptr);
    this.#ptr = 0;
    this.#tick = 0;
    this.#disposed = true;
  }

  toString() {
    this.#assertAlive();

    const mode = this.isWrite() ? "W" : "R";
    return (
      `PC=${hex16(this.pc())} ` +
      `A=${hex8(this.a())} ` +
      `X=${hex8(this.x())} ` +
      `Y=${hex8(this.y())} ` +
      `S=${hex8(this.s())} ` +
      `P=${hex8(this.p())} ` +
      `BUS=${mode} ${hex16(this.busAddress())} ` +
      `DATA=${hex8(this.busData())} ` +
      `STATUS=${hex8(this.busStatus())}`
    );
  }

  #allocSnapshotScratch() {
    const scratch = this.#runtime.exports.qe6502js_context_alloc() >>> 0;

    if (scratch === 0) {
      throw new Error("qe6502js_context_alloc returned null");
    }

    return scratch;
  }

  #storeTick(value) {
    this.#tick = value >>> 0;
    return this.#tick;
  }

  #assertAlive() {
    if (this.#disposed) {
      throw new Error("Qe6502Cpu has been disposed");
    }
  }
}
