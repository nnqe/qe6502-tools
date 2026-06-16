# qe6502-tools

## Minimal Apple II Sokol runner

`qeaii_sokol_appleii` is a deliberately small Apple II Plus runner. It accepts exactly one command-line argument: a `.dsk` or `.nib` disk image path. `.dsk` images are converted in memory with the Dsk2Nib port before boot; `.nib` images are mounted directly.

The runner embeds the Apple II Plus ROM and video/font ROM hex files copied from the deprecated `qe6502-old` project. It opens a Sokol window, runs the existing `qeaii` computer core, uploads the 280x192 video frame, and maps host keyboard input to the Apple II high-bit keyboard latch codes used by the old core:

- printable keys are uppercased and sent as `ASCII | 0x80`
- Enter is `0x8D`
- Escape is `0x9B`
- Backspace/Delete/Left are `0x88`
- Right is `0x95`
- Tab is `0x89`
- Ctrl+A through Ctrl+Z are sent as Apple II control codes

Build and run:

```sh
cmake -S . -B build
cmake --build build --target qeaii_sokol_appleii
./build/appleii_sokol/qeaii_sokol_appleii /path/to/disk.dsk
```

On Linux the target needs OpenGL and X11 development headers. If those headers are missing, CMake leaves the rest of the project buildable and prints a warning that the Sokol Apple II target was skipped.
