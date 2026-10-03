#!/usr/bin/env python3
"""Generate the placeholder LiveArea images in sce_sys/.

The Vita needs 8-bit indexed PNGs here, so this writes palette PNGs directly.
Replace the output with real art whenever someone makes some; this script
only exists so the repo contains no third-party artwork.

usage: tools/make_livearea.py [repo_root]
"""
import os, struct, sys, zlib

# 5x7 bitmap glyphs for the few letters we need
FONT = {
    'H': ["10001", "10001", "10001", "11111", "10001", "10001", "10001"],
    'O': ["01110", "10001", "10001", "10001", "10001", "10001", "01110"],
    'V': ["10001", "10001", "10001", "10001", "01010", "01010", "00100"],
    'E': ["11111", "10000", "10000", "11110", "10000", "10000", "11111"],
    'R': ["11110", "10001", "10001", "11110", "10100", "10010", "10001"],
    'I': ["11111", "00100", "00100", "00100", "00100", "00100", "11111"],
    'T': ["11111", "00100", "00100", "00100", "00100", "00100", "00100"],
    'A': ["01110", "10001", "10001", "11111", "10001", "10001", "10001"],
    '!': ["00100", "00100", "00100", "00100", "00100", "00000", "00100"],
    'S': ["01111", "10000", "10000", "01110", "00001", "00001", "11110"],
    ' ': ["00000"] * 7,
}

# palette: 0..63 background gradient (deep blue -> teal), 64 = text, 65 = shadow
PAL = [(int(8 + 10 * t / 63), int(16 + 70 * t / 63), int(48 + 90 * t / 63)) for t in range(64)]
PAL += [(255, 214, 64), (10, 10, 24)]
TEXT, SHADOW = 64, 65


def write_indexed_png(path, w, h, px):
    def chunk(t, b):
        return struct.pack('>I', len(b)) + t + b + struct.pack('>I', zlib.crc32(t + b))
    raw = b''.join(b'\0' + bytes(px[y * w:(y + 1) * w]) for y in range(h))
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n')
        f.write(chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 3, 0, 0, 0)))
        f.write(chunk(b'PLTE', b''.join(bytes(c) for c in PAL)))
        f.write(chunk(b'IDAT', zlib.compress(raw, 9)))
        f.write(chunk(b'IEND', b''))


def canvas(w, h):
    # vertical gradient with a perspective floor grid, a nod to the game's mazes
    px = []
    horizon = h * 0.45
    for y in range(h):
        for x in range(w):
            c = int(63 * y / max(h - 1, 1))
            if y > horizon:
                depth = (y - horizon) / (h - horizon)
                gx = (x - w / 2) / max(depth, 0.05)
                if int(gx) % 40 == 0 or int(1 / max(depth, 0.02) * 6) % 6 == 0:
                    c = min(63, c + 20)
            px.append(c)
    return px


def text(px, w, s, x0, y0, scale):
    for dx, dy, col in ((max(1, scale // 3),) * 2 + (SHADOW,), (0, 0, TEXT)):
        for i, ch in enumerate(s):
            for r, row in enumerate(FONT[ch]):
                for c, bit in enumerate(row):
                    if bit == '1':
                        for yy in range(scale):
                            for xx in range(scale):
                                X = x0 + dx + (i * 6 + c) * scale + xx
                                Y = y0 + dy + r * scale + yy
                                px[Y * w + X] = col


def centered(w, h, s, scale, y=None):
    px = canvas(w, h)
    tw = (len(s) * 6 - 1) * scale
    text(px, w, s, (w - tw) // 2, y if y is not None else (h - 7 * scale) // 2, scale)
    return px


def main(root):
    sce = os.path.join(root, 'sce_sys')
    write_indexed_png(os.path.join(sce, 'icon0.png'), 128, 128, centered(128, 128, 'HOVER', 4, 30))
    write_indexed_png(os.path.join(sce, 'livearea/contents/bg.png'), 840, 500,
                      centered(840, 500, 'HOVER!', 16, 90))
    write_indexed_png(os.path.join(sce, 'livearea/contents/startup.png'), 280, 158,
                      centered(280, 158, 'START', 6))


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..'))
