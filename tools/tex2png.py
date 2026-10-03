#!/usr/bin/env python3
"""Extract every texture in a Hover! .tex file to PNG (top mip level).

usage: tex2png.py mazes/text1.tex outdir/
"""
import os, struct, sys, zlib
import hoverfmt


def write_png(path, w, h, rgba):
    def chunk(t, b):
        return struct.pack('>I', len(b)) + t + b + struct.pack('>I', zlib.crc32(t + b))
    raw = b''.join(b'\0' + rgba[y * w * 4:(y + 1) * w * 4] for y in range(h))
    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n')
        f.write(chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0)))
        f.write(chunk(b'IDAT', zlib.compress(raw, 9)))
        f.write(chunk(b'IEND', b''))


def main(tex, outdir):
    os.makedirs(outdir, exist_ok=True)
    pal, textures, _, _ = hoverfmt.read_tex_file(tex)
    for t in textures:
        m = t['mips'][0]
        idx = hoverfmt.mip_to_indexed(m, 0)
        covered = bytearray(m['width'] * m['height'])
        for x, spans in enumerate(m['columns']):
            for y0, y1 in spans:
                for y in range(max(y0, 0), min(y1, m['height'] - 1) + 1):
                    covered[y * m['width'] + x] = 1
        rgba = bytearray()
        for i, c in zip(idx, covered):
            rgba += bytes(pal[i]) + (b'\xff' if c else b'\0')
        write_png(os.path.join(outdir, t['name'] + '.png'), m['width'], m['height'], bytes(rgba))
        print(f"{t['name']:12s} {m['width']}x{m['height']} mips={len(t['mips'])}")


if __name__ == '__main__':
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(*sys.argv[1:])
