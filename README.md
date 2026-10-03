# HoverVita

A native PS Vita port of **Microsoft Hover!** (1995), the bumper-car
capture-the-flag game from the Windows 95 CD, rebuilt from a reverse
engineering of the original `hover.exe`.

[![Build](https://github.com/TheGh0stShip/HoverVita/actions/workflows/build.yml/badge.svg)](https://github.com/TheGh0stShip/HoverVita/actions/workflows/build.yml)
![Status: early work in progress](https://img.shields.io/badge/status-early%20WIP-orange)
![License: GPL-3.0](https://img.shields.io/badge/license-GPL--3.0-blue)

> [!IMPORTANT]
> This project contains **no Microsoft code or game assets**. You need your own
> copy of Hover! to play it. See [Game data](#game-data).

## Status

The game is **not playable yet**. Progress so far:

| Area | State |
|---|---|
| Ghidra workflow, class map, shared symbol file | ✅ done |
| `.tex` texture sets (palette, mipmaps, sparse columns) | ✅ fully decoded, C loader + tests |
| `.muz` music (Windows MIDI stream → `.mid`) | ✅ converter |
| Vita + PC builds, VPK packaging, CI | ✅ |
| `.maz` maze files (`CMerlinStatic`, BSP, game objects) | 🔍 container decoded, records in progress |
| Software renderer | ⏳ |
| Physics, robots, pods, game rules | ⏳ |
| Audio mixer, MIDI playback | ⏳ |
| Menus, options, high scores | ⏳ |

The current build is a **texture viewer**. It checks the whole pipeline from
the memory card to the screen while the engine is being rebuilt. See
[docs/roadmap.md](docs/roadmap.md) for the plan.

## Game data

Copy these files from your Hover! install, or from the `HOVER` folder on the
Windows 95 CD, keeping the folder structure:

```
mazes/   maze1.maz maze2.maz maze3.maz small.maz text1.tex text2.tex text3.tex small.tex
sounds/  mixed/*.wav  unmixed/*.wav  music/*.muz
```

| Platform | Where to put it |
|---|---|
| PS Vita | `ux0:data/HoverVita/` (e.g. `ux0:data/HoverVita/mazes/maze1.maz`) |
| PC | `./hover/` next to where you run it, or set `HOVER_DATA=/path/to/hover` |

File names are matched case-insensitively.

## Installing on Vita

1. Download `HoverVita.vpk` from the [latest CI run](https://github.com/TheGh0stShip/HoverVita/actions/workflows/build.yml) or from Releases.
2. Install it with VitaShell.
3. Copy the game data to `ux0:data/HoverVita/`.

Viewer controls: **D-pad ←/→** cycles textures, **↑/↓** changes mip level,
**L/R** switches texture file, **Start** quits.

## Building

### PS Vita

Needs [VitaSDK](https://vitasdk.org) with SDL2 (`vdpm sdl2` if it is missing).

```bash
cmake -S . -B build-vita -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake
cmake --build build-vita
# -> build-vita/HoverVita.vpk
```

### PC (development build)

The PC build uses the same code through SDL2 and is the fastest way to
iterate and debug.

```bash
sudo apt install cmake libsdl2-dev    # or: brew install cmake sdl2
cmake -S . -B build-pc
cmake --build build-pc
./build-pc/HoverVita                   # reads ./hover/ by default
ctest --test-dir build-pc              # format tests (skipped without game data)
```

## Repository layout

```
src/engine/     portable game code, no OS or SDL dependencies
src/platform/   SDL2 platform layer (Vita + PC)
src/main.c      entry point
tests/          tests against the real data files
tools/          Python tools: format readers, tex2png, muz2mid, LiveArea art
re/             reverse-engineering workflow: Ghidra scripts, symbols.csv
docs/           file formats, RE notes, architecture, roadmap
sce_sys/        Vita LiveArea assets (original placeholder art)
```

## Contributing

Help is welcome, whether that's reverse engineering, C, Vita testing or art.
Start with [CONTRIBUTING.md](CONTRIBUTING.md), then
[docs/reverse-engineering.md](docs/reverse-engineering.md).

## Legal

Hover! is © Microsoft Corporation. This is an unofficial fan project, not
affiliated with or endorsed by Microsoft. This repository contains only
original code written from a study of how the game behaves. It does not
include the original executable, decompiler output, or game data, and
contributions must not add them.

Licensed under the [GNU GPL v3](LICENSE).
