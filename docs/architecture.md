# Architecture

```
┌──────────────────────────── src/main.c ────────────────────────────┐
│ app loop: input → game tick → render → present                     │
├───────────────── src/engine/ (portable C99) ───────────────────────┤
│ engine/   archive, texture, maze   (data formats, no platform deps)│
│ game/     timing; objects, physics, AI*                            │
│ render/   GPU renderer (fixed-function GL subset)                  │
├──────────────────────── src/platform/ ─────────────────────────────┤
│ platform_files.c   data dir, case-insensitive file loading         │
│ platform_vita.c    vitaGL, sceCtrl, scePower        (Vita)         │
│ platform_pc.c      SDL2 window + OpenGL, keyboard/gamepad  (PC)    │
└────────────────────────────────────────────────────────────────────┘
                                     * = planned
```

## Rules

- **The engine does not include SDL or OS headers.** It talks to the outside
  world only through `src/platform/platform.h` and `src/engine/log.h`. This
  keeps it testable and makes other ports easy.
- **60 fps at native 960×544.** See below.
- **Fixed 20 Hz simulation.** See below.
- **PC build first.** Everything has to run on the PC build, where debugging
  is easy. The Vita build uses the same code, plus Vita-specific bits behind
  `#ifdef __vita__` in the platform layer only.

## Frame rate: 60 fps target

The original renders in software and is capped at 20 fps by its timer.
On the Vita we aim for a locked **60 fps at 960×544**, and the design is
built around that:

- **The GPU renders the world.** At load time the maze becomes vertex
  batches, one per texture, so a frame is about 20–40 draw calls and under
  1,000 triangles. Textures are expanded from the 8-bit palette to RGBA
  once. Nearest filtering with mipmaps keeps the original's look, and alpha
  test handles the sparse transparent spans, so nothing needs sorting.
- **Fixed 20 Hz simulation with interpolation.** `hover.exe` runs one game
  frame per 50 ms `timeSetEvent` tick (callback 0x408aa0), and every gameplay
  constant assumes that step. We keep it exactly (`src/game/timing.h`) and
  render every vsync, interpolating between the last two simulation states.
  Gameplay matches the original, but motion is 60 fps smooth.
- **Vsync paces the loop.** `vglWaitVblankStart(GL_TRUE)` on Vita and swap
  interval 1 on PC. The Vita runs at maximum clocks
  (444/222/222 MHz CPU/bus/GPU).
- **Budget meter.** A bar in the top-left shows CPU time per frame against
  the 16.7 ms budget: green under 75%, yellow under 100%, red over.
  Select/Tab toggles it. The log prints fps and the worst frame every 5 s.
- **No per-frame allocation**, and nothing is loaded from disk during play.

Current numbers: about 0.3–2 ms of CPU per frame on PC. Vita hardware
numbers are still needed (see the roadmap).

## Simulation vs. rendering

```
every display frame (60 Hz):
    poll input            (button edges are kept until the next tick uses them)
    while accumulated time >= 50 ms:   sim_step()        <- gameplay, 20 Hz
    render(lerp(prev_state, cur_state, accumulated / 50 ms))
    swap (vsync)
```

## Data directory

`platform_data_dir()` is `ux0:data/HoverVita/` on Vita and `$HOVER_DATA` or
`./hover/` on PC. Paths from the original exe (`mazes\TEXT1.tex`) work
as-is: separators are normalised and matching ignores case.
