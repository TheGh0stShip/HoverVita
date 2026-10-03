# Architecture

```
┌──────────────────────────── src/main.c ────────────────────────────┐
│ app loop: input → game tick → render → present                     │
├───────────────── src/engine/ (portable C99) ───────────────────────┤
│ archive   CArchive reader          texture   CMerlinTexture sets   │
│ maze*     level loading            render*   software renderer     │
│ game*     objects, physics, AI     audio*    mixer, music          │
├───────────────── src/platform/ (SDL2) ─────────────────────────────┤
│ files (case-insensitive), logging, window/framebuffer, input, audio│
└──────────────── PS Vita (VitaSDK)   │   PC (Linux/macOS/Windows) ───┘
                                     * = planned
```

## Rules

- **The engine does not include SDL or OS headers.** It talks to the outside
  world only through `src/platform/platform.h` and `src/engine/log.h`. This
  keeps it testable and makes other ports easy.
- **Render like the original.** Hover! draws into an 8-bit palettised
  framebuffer. We keep that (it's faithful and cheap), and the platform layer
  converts and scales it to 960×544 with one texture upload per frame.
- **Fixed tick.** The original runs its simulation from a multimedia timer.
  We'll run a fixed-rate tick that matches it, decoupled from the display
  refresh, so physics behaves the same on every device.
- **PC build first.** Everything has to run on the PC build, where debugging
  is easy. The Vita build uses the same code, plus Vita-specific bits behind
  `#ifdef __vita__` in the platform layer only.

## Data directory

`platform_data_dir()` is `ux0:data/HoverVita/` on Vita and `$HOVER_DATA` or
`./hover/` on PC. Paths from the original exe (`mazes\TEXT1.tex`) work
as-is: separators are normalised and matching ignores case.
