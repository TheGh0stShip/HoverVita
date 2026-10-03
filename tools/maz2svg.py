#!/usr/bin/env python3
"""Draw a Hover! maze top-down as SVG: walls, BSP splitters, named locations.

usage: maz2svg.py mazes/maze1.maz maze1.svg
"""
import sys
import hoverfmt

COLORS = {'HUMAN': '#2a7de1', 'ROBOT': '#d9412b', 'FLAG_HUMAN': '#7fb3ff',
          'FLAG_ROBOT': '#ff9a85', 'POD': '#2fae5a'}


def color_for(name):
    for prefix in sorted(COLORS, key=len, reverse=True):
        if name.startswith(prefix):
            return COLORS[prefix]
    return '#888'


def main(src, dst):
    world, _, _ = hoverfmt.read_maz_file(src)
    statics, _dyn, locations, bsp = world['arrays']
    x0, y0, x1, y1 = world['header']
    pad = 256
    out = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="{x0 - pad} {y0 - pad} '
           f'{x1 - x0 + 2 * pad} {y1 - y0 + 2 * pad}" width="1000" style="background:#111">']
    for n in bsp:
        out.append(f'<line x1="{n["x1"]}" y1="{n["y1"]}" x2="{n["x2"]}" y2="{n["y2"]}" '
                   'stroke="#333" stroke-width="16"/>')
    for w in statics:
        low = w['s'][0] > 0          # walls that don't reach the floor
        out.append(f'<line x1="{w["x1"]}" y1="{w["y1"]}" x2="{w["x2"]}" y2="{w["y2"]}" '
                   f'stroke="{"#c9a227" if low else "#ddd"}" stroke-width="40">'
                   f'<title>{w["textures"]} {w["s"]} {w["b"]}</title></line>')
    for loc in locations:
        x, y = loc['s'][0], loc['s'][1]
        out.append(f'<circle cx="{x}" cy="{y}" r="90" fill="{color_for(loc["name"])}">'
                   f'<title>{loc["name"]} {loc["s"]}</title></circle>')
    out.append('</svg>')
    open(dst, 'w').write('\n'.join(out))
    print(f'{src}: {len(statics)} walls, {len(bsp)} BSP nodes, {len(locations)} locations -> {dst}')


if __name__ == '__main__':
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(*sys.argv[1:])
