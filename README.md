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
| `.maz` mazes (walls, BSP, spawn/flag/pod locations) | ✅ fully decoded, C loader + tests |
| 60 fps GPU renderer + fixed 20 Hz simulation loop | 🚧 walls, floor and sky render; exact placement in progress |
| Physics, robots, pods, game rules | ⏳ |
| Audio mixer, MIDI playback | ⏳ |
| Menus, options, high scores | ⏳ |

The current build is a **maze fly-through**. You can move around the three
original mazes rendered from your game data, targeting 60 fps on Vita. See
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

Controls:

| Action | Vita | PC |
|---|---|---|
| Move | left stick | WASD |
| Look | right stick / D-pad | arrow keys |
| Down / up | L / R | PageUp / PageDown |
| Next maze | Triangle | E |
| Frame-time meter | Select | Tab |
| Quit | Start | Esc |

## Building

### PS Vita

Needs [VitaSDK](https://vitasdk.org) with vitaGL (`vdpm vitagl` if it is missing).
The Vita build doesn't use SDL.

```bash
cmake -S . -B build-vita -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake
cmake --build build-vita
# -> build-vita/HoverVita.vpk
```

### PC (development build)

> [!TIP]
> On **WSL**, Mesa defaults to software rendering (llvmpipe), which won't
> reach 60 fps. Use your GPU with `export GALLIUM_DRIVER=d3d12`. The game logs
> its GL renderer at startup.


The PC build uses the same code through SDL2 and is the fastest way to
iterate and debug.

```bash
sudo apt install cmake libsdl2-dev libgl-dev    # or: brew install cmake sdl2
cmake -S . -B build-pc
cmake --build build-pc
./build-pc/HoverVita                   # reads ./hover/ by default
./build-pc/HoverVita --level 3 --screenshot shot.ppm   # render one frame to a file
ctest --test-dir build-pc              # format tests (skipped without game data)
```

## Repository layout

```
src/engine/     data formats (CArchive, textures, mazes), no platform deps
src/game/       game logic and timing
src/render/     GPU renderer (GL subset shared by vitaGL and desktop GL)
src/platform/   Vita (vitaGL/sce*) and PC (SDL2/OpenGL) backends
src/main.c      entry point
tests/          tests against the real data files
tools/          Python tools: format readers, tex2png, maz2svg, muz2mid, LiveArea art
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
