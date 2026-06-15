import assert from "node:assert/strict";
import { fileURLToPath } from "node:url";

import { loadQe6502Node, Model, Status } from "../../binds/js/qe6502.js";

const defaultWasmUrl = new URL(
  "../../build/release_wasm/binds/js/qe6502_js.wasm",
  import.meta.url,
);
const wasmPath = process.argv[2] ?? fileURLToPath(defaultWasmUrl);

const qe = await loadQe6502Node(wasmPath);
const cpu = qe.createCpu(Model.nmos);

try {
  const restartTick = cpu.restart();

  assert.equal(restartTick >>> 0, restartTick);
  assert.equal(cpu.cpuModel(), Model.nmos);
  assert.equal(cpu.busAddress(), 0x00ff);
  assert.equal(cpu.busData(), 0x00);
  assert.equal(cpu.busStatus(), Status.internalReset);
  assert.equal(cpu.isWrite(), false);
  assert.equal(cpu.isOpcodeFetch(), false);
  assert.equal(cpu.isInternalReset(), true);
  assert.equal(cpu.isJammed(), false);

  assert.equal(cpu.isIrqAsserted(), false);
  cpu.irqAssert(true);
  assert.equal(cpu.isIrqAsserted(), true);
  cpu.irqAssert(false);
  assert.equal(cpu.isIrqAsserted(), false);
  assert.throws(() => cpu.irqAssert(1), TypeError);

  assert.equal(cpu.isNmiAsserted(), false);
  cpu.nmiAssert(true);
  assert.equal(cpu.isNmiAsserted(), true);
  cpu.nmiAssert(false);
  assert.equal(cpu.isNmiAsserted(), false);
  assert.throws(() => cpu.nmiAssert(1), TypeError);

  cpu.setPc(0x1234);
  cpu.setA(0x56);
  cpu.setX(0x78);
  cpu.setY(0x9a);
  cpu.setS(0xbc);
  cpu.setP(0xde);

  const snapshot = cpu.save();

  assert.equal(snapshot.byteLength, 64);
  assert.equal(cpu.pc(), 0x1234);
  assert.equal(cpu.a(), 0x56);
  assert.equal(cpu.x(), 0x78);
  assert.equal(cpu.y(), 0x9a);
  assert.equal(cpu.s(), 0xbc);
  assert.equal(cpu.p(), 0xde);

  cpu.setPc(0);
  cpu.setA(0);
  cpu.setX(0);
  cpu.setY(0);
  cpu.setS(0);
  cpu.setP(0);

  const loadedTick = cpu.load(snapshot);

  assert.equal(loadedTick, restartTick);
  assert.equal(cpu.pc(), 0x1234);
  assert.equal(cpu.a(), 0x56);
  assert.equal(cpu.x(), 0x78);
  assert.equal(cpu.y(), 0x9a);
  assert.equal(cpu.s(), 0xbc);
  assert.equal(cpu.p(), 0xde);

  cpu.jumpTo(0x2000);
  assert.equal(cpu.busAddress(), 0x2000);
  assert.equal((cpu.busStatus() & Status.fetch) !== 0, true);

  console.log("qe6502 JS smoke OK");
} finally {
  cpu.dispose();
}
