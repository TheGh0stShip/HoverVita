#!/usr/bin/env python3
"""Find MFC class vtables in hover.exe and emit symbols.csv rows.

Slot 0 of every CObject vtable is GetRuntimeClass(), compiled as
`mov eax, offset classXXX; ret` (B8 imm32 C3). Finding pointers to such stubs
in .rdata gives the vtable, and slot 2 is Serialize.

usage: find_vtables.py hover.exe symbols.csv   (prints new rows)
"""
import csv, struct, sys
sys.path.insert(0, __file__.rsplit('/', 1)[0])
from find_runtime_classes import load


def main(exe, symbols):
    d, secs = load(exe)
    def va2off(v):
        for va, raw, sz in secs:
            if va <= v < va + sz:
                return raw + v - va
    rtc = {}
    for row in csv.reader(l for l in open(symbols) if not l.startswith('#')):
        if len(row) == 3 and row[2].endswith('::classRuntime'):
            rtc[int(row[0], 16)] = row[2].split('::')[0]
    text = secs[0]
    stubs = {}
    for o in range(text[1], text[1] + text[2] - 6):
        if d[o] == 0xB8 and d[o + 5] == 0xC3:
            target = struct.unpack_from('<I', d, o + 1)[0]
            if target in rtc:
                stubs[text[0] + o - text[1]] = rtc[target]
    print("address,kind,name")
    for stub, cls in sorted(stubs.items(), key=lambda kv: kv[1]):
        print(f"{stub:08x},func,{cls}::GetRuntimeClass")
        p = struct.pack('<I', stub)
        for va, raw, sz in secs[1:]:
            o = d.find(p, raw, raw + sz)
            while o >= 0:
                vt = va + o - raw
                ser = struct.unpack_from('<I', d, o + 8)[0]
                print(f"{vt:08x},data,{cls}::vftable")
                if va2off(ser) is not None and secs[0][0] <= ser < secs[0][0] + secs[0][2]:
                    print(f"{ser:08x},func,{cls}::Serialize")
                o = d.find(p, o + 4, raw + sz)


if __name__ == '__main__':
    main(*sys.argv[1:3])
