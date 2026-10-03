# Roadmap

Milestones are roughly in order. Each one should leave the Vita build working.

### M1: Pipeline ✅
- [x] Ghidra workflow, runtime-class map, shared `symbols.csv`
- [x] `.tex` decoded (Python and C), with tests
- [x] `.muz` → MIDI converter
- [x] CMake build for Vita (VPK) and PC, CI
- [x] Texture viewer on hardware

### M2: Mazes
- [ ] Decode `CMerlinWorld` / `CMerlinStatic` / `CMerlinBSP` / `CMerlinLine` / `CMerlinLocation`
- [ ] `tools/maz2svg.py` top-down map dump for checking
- [ ] C maze loader + tests

### M3: Renderer
- [ ] Reverse the span/column renderer (walls, floor, ceiling, sky, sprites)
- [ ] 8-bit framebuffer → SDL texture presenter
- [ ] Free-fly camera through a maze on Vita at 30+ fps

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
