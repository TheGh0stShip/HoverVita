# Roadmap

Milestones are roughly in order. Each one should leave the Vita build working.

### M1: Pipeline ✅
- [x] Ghidra workflow, runtime-class map, shared `symbols.csv`
- [x] `.tex` decoded (Python and C), with tests
- [x] `.muz` → MIDI converter
- [x] CMake build for Vita (VPK) and PC, CI
- [x] Texture viewer on hardware

### M2: Mazes ✅
- [x] Decode `CMerlinWorld` / `CMerlinStatic` / `CMerlinBSP` / `CMerlinLine` / `CMerlinLocation`
- [x] `tools/maz2svg.py` top-down map dump for checking
- [x] C maze loader + tests

### M3: Renderer (60 fps)
- [x] GPU world renderer: walls, floor, sky, mipmaps, alpha-tested decals
- [x] Fixed 20 Hz sim + interpolated 60 fps rendering, frame meter
- [x] Free-fly camera through all three mazes (PC verified)
- [ ] **Measure on Vita hardware**: confirm locked 60 fps, record numbers in docs/architecture.md
- [ ] Reverse the original renderer for exact placement: texture slots, units per texel, eye height, FOV, sky
- [ ] Ceiling (`CBASE`), steps, raised panels, pads
- [ ] Upload the file's own mip chain on Vita (check vitaGL level uploads)
- [ ] Sprites: pods, flags, robots (`POD0x`, `FLG0x`, `DRONE`)

### M4: Gameplay
- [ ] Hovercraft physics and wall collisions (`CCollider`, `CRegionMatrix`)
- [ ] Flags, beacons, pods, pads
- [ ] Robot AI (`CRobotPlayer`)
- [ ] Scoring, timer, level progression

### M5: Sound
- [ ] Software mixer for the WAV effects (replaces WaveMix/waveOut)
- [ ] Music: MIDI synthesis (e.g. TinySoundFont + a small SoundFont the user supplies)

### M6: Front end
- [ ] Title, menus, options, controls, high scores (replace the Win32 dialogs)
- [ ] Map overlay, HUD
- [ ] Vita niceties: sticks, rear touch, save data in `ux0:data/HoverVita/`

### M7: Release
- [ ] Performance pass on hardware
- [ ] Real LiveArea art
- [ ] 1.0 VPK
