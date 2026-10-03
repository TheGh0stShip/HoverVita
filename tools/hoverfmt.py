"""Readers for Hover! data files (MFC CArchive based).

Reverse-engineered from hover.exe; see docs/formats.md.
"""
import struct


class ArchiveReader:
    """Minimal MFC CArchive object reader (ReadObject / class tags)."""

    NULL_TAG = 0x0000
    NEW_CLASS_TAG = 0xFFFF
    CLASS_TAG = 0x8000
    BIG_OBJECT_TAG = 0x7FFF

    def __init__(self, data, pos=0):
        self.d = data
        self.pos = pos
        self.loaded = [None]          # index 0 == NULL
        self.classes = {}             # class name -> reader function

    def u8(self):
        v = self.d[self.pos]; self.pos += 1; return v

    def _unpack(self, fmt):
        v = struct.unpack_from(fmt, self.d, self.pos)
        self.pos += struct.calcsize(fmt)
        return v[0] if len(v) == 1 else v

    def i16(self): return self._unpack('<h')
    def u16(self): return self._unpack('<H')
    def i32(self): return self._unpack('<i')
    def u32(self): return self._unpack('<I')

    def bytes(self, n):
        v = self.d[self.pos:self.pos + n]; self.pos += n; return v

    def cstring(self):
        n = self.u8()
        if n == 0xFF:
            n = self.u16()
        return self.bytes(n).decode('latin-1')

    def count(self):
        """CArchive::ReadCount (WORD, 0xFFFF escape to DWORD)."""
        n = self.u16()
        return self.u32() if n == 0xFFFF else n

    def read_object(self):
        tag = self.u16()
        if tag == self.BIG_OBJECT_TAG:
            tag = self.u32()
        if tag == self.NULL_TAG:
            return None
        if tag == self.NEW_CLASS_TAG:
            schema = self.u16()
            name = self.bytes(self.u16()).decode('latin-1')
            self.loaded.append(('class', name, schema))
        elif tag & self.CLASS_TAG:
            entry = self.loaded[tag & 0x7FFF]
            assert entry[0] == 'class', f'bad class ref {tag:#x}'
            _, name, schema = entry
        else:
            return self.loaded[tag]       # back-reference to an object
        reader = self.classes.get(name)
        if reader is None:
            raise NotImplementedError(f'no reader for {name} at {self.pos:#x}')
        slot = len(self.loaded)
        self.loaded.append(None)
        obj = reader(self)
        obj['class'] = name
        self.loaded[slot] = obj
        return obj


def read_palette(r):
    """256 RGBQUAD entries -> list of (r, g, b)."""
    pal = []
    for _ in range(256):
        b, g, rr, _x = r.bytes(4)
        pal.append((rr, g, b))
    return pal


def skip_extension(r):
    """Every Merlin Serialize ends with i16 n + n bytes the loader ignores."""
    n = r.i16()
    if n > 0:
        r.bytes(n)


def read_merlin_object(r):
    """CMerlinObject::Serialize (0x411b10): name + extension block."""
    o = {'name': r.cstring()}
    skip_extension(r)
    return o


def read_merlin_line(r):
    """CMerlinLine::Serialize (0x414740): 2D segment in world units."""
    o = read_merlin_object(r)
    o['x1'], o['y1'], o['x2'], o['y2'] = r.i16(), r.i16(), r.i16(), r.i16()
    skip_extension(r)
    return o


def read_merlin_static(r):
    """CMerlinStatic::Serialize (0x419e00): a wall segment."""
    o = read_merlin_line(r)
    o['textures'] = [r.cstring() for _ in range(6)]
    o['s'] = [r.i16() for _ in range(4)]
    o['b'] = [r.u8() for _ in range(3)]
    n = r.i16()
    if n >= 5:
        o['b2'] = r.u8()
        o['s2'] = [r.i16(), r.i16()]
        n -= 5
    if n > 0:
        r.bytes(n)
    return o


def read_merlin_bsp(r):
    """CMerlinBSP::Serialize (0x41bc40): BSP node (splitter line + doubles)."""
    o = read_merlin_line(r)
    o['s'] = [r.i16() for _ in range(5)]
    o['d'] = [r._unpack('<d'), r._unpack('<d')]
    skip_extension(r)
    return o


def read_merlin_location(r):
    """CMerlinLocation::Serialize (0x41b940): named point."""
    o = read_merlin_object(r)
    o['s'] = [r.i16() for _ in range(4)]
    skip_extension(r)
    return o


def read_merlin_dynamic(r):
    """CMerlinDynamic::Serialize (0x42cc40)."""
    o = read_merlin_line(r)
    o['texture'] = r.cstring()
    o['s'] = [r.i16() for _ in range(5)]
    o['b'] = r.u8()
    skip_extension(r)
    return o


def read_maz_file(path):
    """CMerlinWorld::Serialize (0x418dc0): 4 x i16 header, then 4 CObArrays."""
    d = open(path, 'rb').read()
    r = ArchiveReader(d)
    r.classes.update({
        'CMerlinStatic': read_merlin_static, 'CMerlinBSP': read_merlin_bsp,
        'CMerlinLocation': read_merlin_location, 'CMerlinDynamic': read_merlin_dynamic,
        'CMerlinLine': read_merlin_line,
    })
    world = {'header': [r.i16() for _ in range(4)]}
    world['arrays'] = [[r.read_object() for _ in range(r.count())] for _ in range(4)]
    return world, r.pos, len(d)


def read_merlin_texture(r):
    """CMerlinTexture::Serialize (hover.exe 0x412980), load path, all mips kept."""
    t = read_merlin_object(r)
    t['flags'] = r.u16()
    mips = []
    for _ in range(r.i16()):
        m = {}
        m['width'] = r.i16()
        m['xmax'] = r.i16()
        m['height'] = r.i16()
        m['ymax'] = r.i16()
        m['shift'] = r.i16()
        size = r.u32()
        m['pixels'] = r.bytes(size)
        m['span_total'] = r.u32()
        cols = []
        for _ in range(m['width']):
            cols.append([(r.i16(), r.i16()) for _ in range(r.i16())])
        m['columns'] = cols
        skip_extension(r)
        mips.append(m)
    t['mips'] = mips
    return t


def mip_to_indexed(m, mip_level=0):
    """Expand column spans into a width x height index buffer (0 = transparent).

    Span coordinates are stored in level-0 space and shifted down per mip level.
    Pixels are column-major, one run per span.
    """
    w, h = m['width'], m['height']
    out = bytearray(w * h)
    src = 0
    for x, spans in enumerate(m['columns']):
        for y0, y1 in spans:
            y0 >>= mip_level; y1 >>= mip_level
            for y in range(y0, y1 + 1):
                if 0 <= y < h:
                    out[y * w + x] = m['pixels'][src]
                src += 1
    return bytes(out)


def read_tex_file(path):
    d = open(path, 'rb').read()
    r = ArchiveReader(d)
    r.classes['CMerlinTexture'] = read_merlin_texture
    pal = read_palette(r)
    textures = [r.read_object() for _ in range(r.count())]
    return pal, textures, r.pos, len(d)
