# qe6502 for JavaScript

`qe6502` is a lightweight bus-cycle 6502 CPU emulator for JavaScript and WebAssembly. The npm package includes the JavaScript API and the compiled `qe6502_js.wasm` module.

## Requirements

- Node.js 22 or newer for Node.js consumers.
- A modern browser with WebAssembly support for browser consumers.

## Official 1.0 release package

The `qe6502` 1.0.0 GitHub Release provides `qe6502-wasm-1.0.0.zip`, which contains the npm package tarball `qe6502-1.0.0.tgz`. The release workflow uploads this package as a GitHub Release asset; it does not automatically publish it to npm.

Until the package is published to npm, unzip the release asset and install from the local tarball:

```sh
npm install ./qe6502-1.0.0.tgz
```

After an npm publication is announced, the normal registry install command is:

```sh
npm install qe6502@1.0.0
```

## Node.js

In Node.js, the loader finds the bundled WebAssembly module automatically:

```js
import { loadQe6502Node, Model } from "qe6502";

const qe = await loadQe6502Node();
const cpu = qe.createCpu(Model.nmos);

cpu.dispose();
```

A custom WebAssembly path or URL can still be supplied when needed:

```js
const qe = await loadQe6502Node("/custom/path/qe6502_js.wasm");
```

## Browser

Browser consumers should pass the WebAssembly module URL explicitly. This works better with static hosting, CDNs, bundlers, and asset pipelines that rename or relocate `.wasm` files.

```js
import { loadQe6502Browser, Model } from "qe6502";

const qe = await loadQe6502Browser("/assets/qe6502_js.wasm");
const cpu = qe.createCpu(Model.nmos);

cpu.dispose();
```

The package also exports the bundled WebAssembly file as a subpath:

```js
import wasmUrl from "qe6502/qe6502_js.wasm";
```

Whether that import returns a usable URL depends on the bundler or runtime.

## Minimal bus loop

`qe6502` is bus-cycle oriented. The caller observes the current bus request and supplies memory data on read cycles. On write cycles, the caller stores the CPU data byte.

```js
import { loadQe6502Node, Model } from "qe6502";

const qe = await loadQe6502Node();
const cpu = qe.createCpu(Model.nmos);
const memory = new Uint8Array(65536);

cpu.jumpTo(0x8000);

for (let i = 0; i < 1_000_000; ++i) {
  if (cpu.isWrite()) {
    memory[cpu.busAddress()] = cpu.busData();
    cpu.tick();
  } else {
    cpu.tick(memory[cpu.busAddress()]);
  }
}

cpu.dispose();
```

## CPU models

```js
const cpu = qe.createCpu(Model.nmos);
```

Available models:

- `Model.nmos` - NMOS 6502
- `Model.nes` - NES / RP2A03 CPU
- `Model.wdc` - WDC 65C02
- `Model.rw` - Rockwell 65C02
- `Model.st` - Synertek 65C02

## Bus state

After `jumpTo`, `restart`, `load`, or `tick`, the CPU exposes the last native bus tick:

```js
cpu.busAddress();
cpu.busData();
cpu.busStatus();
cpu.isWrite();
cpu.isOpcodeFetch();
cpu.isInternalReset();
cpu.isJammed();
```

The raw tick value is returned by `tick`, `jumpTo`, `restart`, and `load`.

## Registers and flags

The wrapper exposes CPU registers:

```js
cpu.pc();
cpu.setPc(0x8000);
cpu.s();
cpu.setS(0xff);
cpu.a();
cpu.setA(0x42);
cpu.x();
cpu.setX(0x00);
cpu.y();
cpu.setY(0x00);
cpu.p();
cpu.setP(0x24);
```

Status flag constants are available through `Flag`:

```js
import { Flag } from "qe6502";

cpu.setP(cpu.p() | Flag.i);
```

## Interrupt inputs

Use `nmiAssert` and `irqAssert` to control the logical interrupt input state:

```js
cpu.nmiAssert(true);
cpu.tick(memory[cpu.busAddress()]);
cpu.nmiAssert(false);
```

```js
cpu.irqAssert(true);
cpu.tick(memory[cpu.busAddress()]);
cpu.irqAssert(false);
```

## Snapshots

`save` returns a portable 64-byte CPU snapshot, including the last tick:

```js
const snapshot = cpu.save();
```

Restore it with:

```js
cpu.load(snapshot);
```

## Lifetime

Each CPU object owns a native WebAssembly-side context. Call `dispose()` when a CPU is no longer needed:

```js
cpu.dispose();
```

Calling methods on a disposed CPU throws an error.

## Package notes

- The npm package is ESM-only.
- The package has no runtime dependencies.
- Node.js loading uses the bundled `qe6502_js.wasm` by default.
- Browser loading intentionally uses an explicit WebAssembly URL/source.

## License

MIT
