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


def read_merlin_object(r):
    # CMerlinObject::Serialize: name, then a WORD (meaning TBD)
    return {'name': r.cstring(), 'objflags': r.u16()}


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
        m['extra'] = r.bytes(r.i16())
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
