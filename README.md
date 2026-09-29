# CHIP-8 Emulator

![C++](https://img.shields.io/badge/C++-17-blue.svg)
![SDL2](https://img.shields.io/badge/SDL2-Graphics%20%26%20Audio-red.svg)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Mac%20%7C%20Linux-lightgrey.svg)
![License](https://img.shields.io/badge/License-MIT-blue.svg)
[![CMake Build CI](https://github.com/Furkan5E/CHIP-8-Emulator/actions/workflows/build.yaml/badge.svg)](https://github.com/Furkan5E/CHIP-8-Emulator/actions/workflows/build.yaml)

A cross platform CHIP-8 emulator written in C++17 using SDL2 for graphics, input, and audio.

![Chip8 Demo](assets/demo.gif)

[![Download Latest Release](https://img.shields.io/github/v/release/Furkan5E/CHIP-8-Emulator?style=for-the-badge&label=Download&color=success)](https://github.com/Furkan5E/CHIP-8-Emulator/releases/latest)

## Features

* **Complete CHIP-8 CPU:** Decodes and executes the full original CHIP-8 instruction set at a configurable speed (instructions per frame), with timers running at 60 Hz.
* **Hardware Audio:** Generates an authentic retro square wave tone via SDL2 audio callbacks.
* **Reduced Flicker:** Pixels fade out over a few frames instead of switching off instantly, smoothing over the flicker CHIP-8 games get from erasing and redrawing sprites.
* **Any Keyboard Layout:** The keypad is mapped by key position, so it works the same on QWERTY, AZERTY, QWERTZ and other layouts.
* **Quality of Life Controls:** Built in support for pausing and instantly rebooting ROMs.

## Quick Start

1. Download the archive for your platform from the [latest release](https://github.com/Furkan5E/CHIP-8-Emulator/releases/latest) (`windows.zip`, `linux.tar.gz` or `macos.tar.gz`) and extract it.
2. Get a CHIP-8 ROM (see [Where to Get ROMs](#where-to-get-roms)).
3. Open a terminal in the extracted folder and run the emulator, passing the ROM path:

```bash
./chip8 path/to/rom.ch8          # Linux / macOS
.\chip8.exe path\to\rom.ch8      # Windows
```

> **macOS:** the executable isn't signed, so Gatekeeper may block it the first time. Allow it under *System Settings > Privacy & Security*, or run `xattr -d com.apple.quarantine chip8` in the extracted folder.

## Usage

```bash
chip8 <ROM_FILE_PATH> [--scale <1-100>] [--speed <1-1000>] [--fade <0-30>]
```

| Option | Range | Default | Description |
| :--- | :--- | :--- | :--- |
| `--scale` | 1-100 | 10 | Window size multiplier (the display is 64x32 pixels) |
| `--speed` | 1-1000 | 10 | CPU instructions executed per frame (60 frames per second) |
| `--fade` | 0-30 | 4 | Frames a pixel takes to fade out after turning off, `0` disables fading |

Example:

```bash
./chip8 roms/tetris.ch8 --scale 15 --speed 30
```

> **Tip:** Games run at different speeds, so tune `--speed` per game. If a game flickers a lot, a higher speed usually helps; Tetris looks best at around `--speed 30`.

## Controls

**Standard Keypad**

Keys are mapped by position, so the layout below stays the same on AZERTY, QWERTZ and other keyboard layouts.

| Original | Mapped To |
| :--- | :--- |
| `1 2 3 C` | `1 2 3 4` |
| `4 5 6 D` | `Q W E R` |
| `7 8 9 E` | `A S D F` |
| `A 0 B F` | `Z X C V` |

**System Controls**
* **Pause/Resume:** `P` (the window title shows `[Paused]` while paused)
* **Reset ROM:** `ESC`

## Building from Source

### Prerequisites

* [CMake](https://cmake.org/) 3.14 or newer
* A C++17 compiler (GCC, Clang, or MSVC)
* Git (CMake uses it to download SDL2)

SDL2 is downloaded and built automatically through CMake's `FetchContent`, so there's no manual dependency management or DLL configuration. The first build compiles SDL2 from source and takes a few minutes; later builds are fast.

**Linux:** building SDL2 from source needs the X11 and audio development packages. On Debian/Ubuntu:

```bash
sudo apt install build-essential cmake git libx11-dev libxext-dev libasound2-dev libpulse-dev
```

**macOS:** install the Xcode command line tools with `xcode-select --install`.

### Build

```bash
git clone https://github.com/Furkan5E/CHIP-8-Emulator.git
cd CHIP-8-Emulator
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

**Windows without Visual Studio:** if you use MinGW (e.g. from MSYS2), tell CMake to use it when configuring:

```bash
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
```

The executable is written to `build/chip8` (`build/chip8.exe` on Windows). With Visual Studio or Xcode generators it is in `build/Release/` instead.

## Where to Get ROMs

ROMs are not included in this repository. Some good places to find them:

* [Timendus/chip8-test-suite](https://github.com/Timendus/chip8-test-suite): test ROMs that check an emulator's instructions, flags and quirks.
* [kripod/chip8-roms](https://github.com/kripod/chip8-roms): a collection of classic CHIP-8 games, demos and programs.

## Compatibility

CHIP-8 interpreters have historically disagreed on a few instructions, so games written for one can misbehave on another. This emulator uses the common modern behaviour:

* `8XY6` / `8XYE` shift `VX` in place (they don't copy `VY` into `VX` first).
* `FX55` / `FX65` leave the index register `I` unchanged.
* `BNNN` jumps to `NNN + V0`.
* Sprites drawn past the edge of the screen are clipped rather than wrapped around.

**Known behaviour:** in Tetris, a single pixel briefly scans along a row when a piece lands. That's the game checking whether the row is full, not an emulator bug; a higher `--speed` makes it mostly disappear.

## Roadmap

* Configurable quirk profiles (CHIP-8, SUPER-CHIP, XO-CHIP)
* SUPER-CHIP high resolution mode
* Debugger overlay with registers, disassembly and single stepping
* Save and load states

## Resources

* [Cowgod's CHIP-8 Technical Reference](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM)
* [Timendus/chip8-test-suite](https://github.com/Timendus/chip8-test-suite)

## License

This project is licensed under the MIT License, see [LICENSE](LICENSE) for details.
