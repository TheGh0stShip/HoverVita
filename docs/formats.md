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
| u16 | unknown, always 0 so far |

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
    i16  extra_len
    u8   extra[extra_len]     // unknown; the game skips it
```

The pixels of each column follow each other directly. For each span there
are `(bottom >> level) - (top >> level) + 1` pixels. Rows not covered by a
span are transparent, which is how the decals and sprites get holes.

At load time the original can drop the largest mip levels to save memory
(global `0x461cfc`, "detail" setting). It reads and discards them.

Implementations: `src/engine/texture.c`, `tools/hoverfmt.py`, `tools/tex2png.py`.

## `.maz`: mazes (work in progress)

Files: `maze1.maz`, `maze2.maz`, `maze3.maz`, `small.maz`. These are CArchive
streams with a short header before the first object. Known classes:

| Class | Runtime class | Size | Notes |
|---|---|---|---|
| CMerlinWorld | 0x4c51e0 | 0x64 | top-level world? |
| CMerlinStatic | 0x4c5220 | 0xc4 | wall/floor segment, references textures by name (`CBASE`, `FBASE`, `BACKGRND`...) |
| CMerlinBSP | 0x4c5660 | 0x68 | BSP node |
| CMerlinLine | 0x4c5248 | 0x44 | 2D line |
| CMerlinLocation | 0x4c5638 | 0x1c | point / spawn location |
| CMerlinDynamic | 0x4c5608 | 0x70 | moving object |

The record layout is not documented yet. This is the next RE task.

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
