import assert from "node:assert/strict";
import { pathToFileURL } from "node:url";

const modulePath = process.argv[2];
if (!modulePath) {
  throw new Error("Usage: js_smoke_node_default.mjs <qe6502.js>");
}

const { loadQe6502Node, Model, Status } = await import(
  pathToFileURL(modulePath).href,
);

const qe = await loadQe6502Node();
const cpu = qe.createCpu(Model.nmos);

try {
  const restartTick = cpu.restart();

  assert.equal(restartTick >>> 0, restartTick);
  assert.equal(cpu.cpuModel(), Model.nmos);
  assert.equal(cpu.busAddress(), 0x00ff);
  assert.equal(cpu.busData(), 0x00);
  assert.equal(cpu.busStatus(), Status.internalReset);
  assert.equal(cpu.isInternalReset(), true);

  console.log("qe6502 JS Node default WASM smoke OK");
} finally {
  cpu.dispose();
}
