# Hover! file formats

Everything here was worked out from `hover.exe` (see
[reverse-engineering.md](reverse-engineering.md)) and checked against the
shipped data. Addresses are virtual addresses in the original `hover.exe`.

All multi-byte values are little-endian.

## MFC CArchive (container for `.tex` and `.maz`)

Hover! is an MFC application (internal codename **Bumper**, engine **Merlin**).
Levels and textures are written with `CArchive` / `CObject::Serialize`, so
objects appear in the standard MFC object stream:

| Tag (u16) | Meaning |
|---|---|
| `0x0000` | NULL object |
| `0xFFFF` | new class: `u16 schema`, `u16 len`, `char name[len]`, then the object |
| `0x8000 \| n` | object of an already seen class, where `n` is the class's load index |
| `0x7FFF` | a `u32` tag follows (large archives) |
| other `n` | back-reference to the object with load index `n` |

Load indices start at 1 and count both classes and objects in the order they
first appear. Other primitives:

- `CString`: `u8 len` (`0xFF` means a `u16 len` follows), then bytes. No terminator.
- Counts (`CArchive::ReadCount`): `u16`, with `0xFFFF` meaning a `u32` follows.

Implementations: `src/engine/archive.c`, `tools/hoverfmt.py`.

### CMerlinObject (base of every Merlin class)

`CMerlinObject::Serialize` (0x411b10):

| Type | Field |
|---|---|
| CString | name |
| i16 n, u8[n] | extension block, skipped |

## `.tex`: texture sets

Files: `mazes/text1.tex`, `text2.tex` and `text3.tex` (one per maze), and `small.tex`.

```
RGBQUAD palette[256]          // B, G, R, reserved; shared by all textures
count   n                     // CArchive::ReadCount
CMerlinTexture objects[n]
u16     trailer               // 0, purpose unknown
```

### CMerlinTexture

`CMerlinTexture::Serialize` at 0x412980 (vtable 0x4bbd20, runtime class 0x4c51b0).

```
CMerlinObject base
u16  flags
i16  mip_count                // 6 for 128px, 7 for 256px textures
repeat mip_count:
    i16  width
    i16  width - 1
    i16  height
    i16  height - 1
    i16  shift                // log2(level-0 size), e.g. 7 for 128
    u32  pixel_bytes
    u8   pixels[pixel_bytes]  // palette indices, column-major, opaque spans only
    u32  total_spans
    repeat width:             // one entry per column
        i16  span_count
        repeat span_count:
            i16 top, bottom   // inclusive, in LEVEL-0 rows: shift right by the mip level
    extension block           // i16 n + n bytes, skipped
```

The pixels of each column follow each other directly. For each span there
are `(bottom >> level) - (top >> level) + 1` pixels. Rows not covered by a
span are transparent, which is how the decals and sprites get holes.

At load time the original can drop the largest mip levels to save memory
(global `0x461cfc`, "detail" setting). It reads and discards them.

Implementations: `src/engine/texture.c`, `tools/hoverfmt.py`, `tools/tex2png.py`.

## `.maz`: mazes

Files: `maze1.maz`, `maze2.maz`, `maze3.maz` (paired with `text1..3.tex`) and
`small.maz`. A maze is a flat 2D map, Doom-style 2.5D: wall segments with a
z range, a 2D BSP, and named points.

### Extension blocks

Every Merlin `Serialize` ends with `i16 n` followed by `n` bytes, which the
loader skips. This is how the format stayed forward compatible.
`CMerlinStatic` is the only class that reads into its block (see below).

### CMerlinWorld (file root)

`CMerlinWorld::Serialize` (0x418dc0). The file has no object tag at the top:
it is the world's fields directly.

```
i16 minx, miny, maxx, maxy        // world bounds (maze1: 512,768 .. 22528,21248)
CObArray statics                  // CMerlinStatic: walls
CObArray dynamics                 // CMerlinDynamic: empty in every shipped maze
CObArray locations                // CMerlinLocation
CObArray bsp                      // CMerlinBSP
```

Each `CObArray` is `count` followed by that many objects (CArchive tags).

| Maze | Walls | BSP nodes | Locations |
|---|---|---|---|
| maze1 | 542 | 544 | 125 |
| maze2 | 377 | 382 | 118 |
| maze3 | 676 | 676 | 128 |
| small | 28 | 28 | 1 |

### CMerlinLine (base of walls and BSP nodes)

`CMerlinLine::Serialize` (0x414740):

```
CMerlinObject base                // name is always empty here
i16 x1, y1, x2, y2
extension block
```

After loading, `0x4010c0` derives the bounding box, `dx`, `dy`, the squared
length, and flags (1 = point, 2 = spans x, 4 = spans y).

### CMerlinStatic (wall)

`CMerlinStatic::Serialize` (0x419e00):

```
CMerlinLine base
CString texture[6]                // looked up by name in the .tex set
i16 bottom, top                   // z range: 0..768 is a full wall, 128..384 a raised panel
i16 unk_a8, unk_aa
u8  flags[3]
i16 n                             // extension block, read as:
    if n >= 5: u8 ext_b, i16 ext_s[2]; n -= 5
    u8 skip[n]
```

Texture slot use (from statistics, **not yet confirmed**): slot 2 is the
face texture (`DECAL_xx`, `WBASE_xx`, `SLED`, `HOLD`, `FLAG`); slot 3 is
usually the same or `FBASE_xx`; slots 0, 1, 4 and 5 hold `STEPS`, `FBASE`
and `CBASE` names on walls that step up or down. Slots whose texture name
isn't found get a null pointer.

### CMerlinBSP

`CMerlinBSP::Serialize` (0x41bc40):

```
CMerlinLine base                  // splitter
i16 v[5]                          // TODO(re): indices / children (e.g. 8, 9, 107, 88, 0)
f64 d[2]                          // 0.0 in the shipped files
extension block
```

### CMerlinLocation

`CMerlinLocation::Serialize` (0x41b940):

```
CMerlinObject base                // name: HUMAN_nn, ROBOT_nn, FLAG_HUMAN_nn, FLAG_ROBOT_nn, POD_RANDOM_nn ...
i16 x, y, z
i16 r                             // 96 for every spawn; probably a radius
extension block
```

### CMerlinDynamic

`CMerlinDynamic::Serialize` (0x42cc40): `CMerlinLine`, `CString texture`,
`i16[5]`, `u8`, extension block. No shipped maze uses it.

Implementations: `src/engine/maze.c`, `tools/hoverfmt.py`, `tools/maz2svg.py`.

## `.muz`: music

A Windows **MIDI stream** file (`RIFF` / `MIDS`), the format played through
`midiStreamOut`.

```
'fmt ' : u32 time_format (ticks per quarter note, 192), u32 max_buffer, [u32 flags]
'data' : u32 buffer_count
         repeat: u32 tick_start, u32 byte_count, MIDIEVENT events[]
MIDIEVENT: u32 delta, u32 stream_id (absent if flags & 1), u32 event
event >> 24: 0x00 = short MIDI message in the low 24 bits, 0x01 = tempo (µs/qn), 0x02 = nop
```

Hover!'s files have no `flags` field, so every event is 12 bytes.
`tools/muz2mid.py` converts them to Standard MIDI Files.

## `.wav`: sound effects

Standard PCM WAV files. `sounds/mixed/` and `sounds/unmixed/` hold the same
effects. The game picks one set depending on whether its software mixer
(WaveMix) is in use.
