import assert from "node:assert/strict";

import { loadQe6502Node, Model, Status } from "qe6502";

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

  console.log("qe6502 JS npm package consumer smoke OK");
} finally {
  cpu.dispose();
}
