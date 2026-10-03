#!/usr/bin/env python3
"""Scan hover.exe for MFC CRuntimeClass records and emit symbols.csv rows.

CRuntimeClass (MFC 3.x): { LPCSTR name; int size; UINT schema;
                           CObject* (*CreateObject)(); CRuntimeClass* base; ... }
"""
import struct, sys

IMAGE = 0x400000

def load(path):
    d = open(path, 'rb').read()
    pe = struct.unpack_from('<I', d, 0x3c)[0]
    nsec = struct.unpack_from('<H', d, pe + 6)[0]
    opt = struct.unpack_from('<H', d, pe + 20)[0]
    secs = []
    for i in range(nsec):
        o = pe + 24 + opt + i * 40
        vsz, va, rsz, raw = struct.unpack_from('<IIII', d, o + 8)
        secs.append((IMAGE + va, raw, min(vsz, rsz)))
    return d, secs

def main(path):
    d, secs = load(path)
    def va2off(v):
        for va, raw, sz in secs:
            if va <= v < va + sz:
                return raw + v - va
    def cstr(v):
        o = va2off(v)
        if o is None: return None
        e = d.find(b'\0', o, o + 64)
        s = d[o:e]
        return s.decode() if e > o and s[:1] == b'C' and s.isalnum() else None

    found = {}
    for va, raw, sz in secs:
        for o in range(raw, raw + sz - 20, 4):
            name, size, schema, create, base = struct.unpack_from('<IiIII', d, o)
            n = cstr(name)
            if n and 0 < size < 0x10000 and (base == 0 or va2off(base) is not None):
                found[va + o - raw] = (n, size, schema, create)
    print("address,kind,name")
    for rtc, (n, size, schema, create) in sorted(found.items()):
        print(f"{rtc:08x},data,{n}::classRuntime")
        if create:
            print(f"{create:08x},func,{n}::CreateObject")

if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '../hover/hover.exe')
