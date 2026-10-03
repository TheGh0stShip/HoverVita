#!/usr/bin/env python3
"""Convert Hover! .muz music (RIFF 'MIDS' MIDI stream) to a Standard MIDI File.

usage: muz2mid.py music1.muz music1.mid

MIDS layout: 'fmt ' = {dwTimeFormat (ticks/qn), cbMaxBuffer, [dwFlags]};
'data' = {cBuffers, then per buffer: dwTickStart, cbBuffer, MIDIEVENTs}.
Each MIDIEVENT is {dwDeltaTime, [dwStreamID unless dwFlags & 1], dwEvent}.
dwEvent high byte: 0x00 short MIDI message, 0x01 tempo, 0x02 nop.
"""
import struct, sys

MEVT_SHORTMSG, MEVT_TEMPO, MEVT_NOP = 0, 1, 2


def parse_mids(data):
    assert data[:4] == b'RIFF' and data[8:12] == b'MIDS', 'not a MIDS file'
    pos, chunks = 12, {}
    while pos + 8 <= len(data):
        cid, size = struct.unpack_from('<4sI', data, pos)
        chunks[cid] = data[pos + 8:pos + 8 + size]
        pos += 8 + size + (size & 1)
    fmt = chunks[b'fmt '] + bytes(12)    # dwFlags is optional (Hover omits it)
    division, _maxbuf, flags = struct.unpack_from('<III', fmt)
    d = chunks[b'data']
    nbuf = struct.unpack_from('<I', d)[0]
    ev_size = 8 if flags & 1 else 12
    events, p = [], 4
    for _ in range(nbuf):
        _tick, cb = struct.unpack_from('<II', d, p)
        p += 8
        end = p + cb
        while p < end:
            delta = struct.unpack_from('<I', d, p)[0]
            ev = struct.unpack_from('<I', d, p + ev_size - 4)[0]
            events.append((delta, ev))
            p += ev_size
    return division, events


def varlen(n):
    out = [n & 0x7F]
    n >>= 7
    while n:
        out.append(0x80 | (n & 0x7F))
        n >>= 7
    return bytes(reversed(out))


def to_smf(division, events):
    trk, pending = bytearray(), 0
    for delta, ev in events:
        pending += delta
        kind, param = ev >> 24 & 0x7F, ev & 0xFFFFFF
        if kind == MEVT_TEMPO:
            msg = b'\xff\x51\x03' + param.to_bytes(3, 'big')
        elif kind == MEVT_SHORTMSG:
            status = param & 0xFF
            n = 2 if (status & 0xF0) in (0xC0, 0xD0) else 3
            msg = bytes([status, param >> 8 & 0x7F, param >> 16 & 0x7F])[:n]
        else:
            continue
        trk += varlen(pending) + msg
        pending = 0
    trk += varlen(pending) + b'\xff\x2f\x00'
    return (b'MThd' + struct.pack('>IHHH', 6, 0, 1, division) +
            b'MTrk' + struct.pack('>I', len(trk)) + bytes(trk))


def main(src, dst):
    division, events = parse_mids(open(src, 'rb').read())
    open(dst, 'wb').write(to_smf(division, events))
    print(f'{src}: {len(events)} events, {division} ticks/qn -> {dst}')


if __name__ == '__main__':
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(*sys.argv[1:])
