#!/usr/bin/env python3
"""datacmp.py UNIT SEC [--ctx N] [--words] [--all] [--addr HEX]

Compares a data section of a unit (our out.o) with the bytes of the original at the address
placed by the data gate, with the relocations resolved. Reads the JSON files of the last gate:
DJ (default build/datacheck.json) and CJ (default build/check.json); for a scoped gate pass
DJ=build/datacheck-u_X.json CJ=build/check-u_X.json."""
import sys, os, re, json, struct
from bisect import bisect_right
ROOT=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, ROOT+'/tools'); os.chdir(ROOT)
import datacheck as dc
unit, sec = sys.argv[1], sys.argv[2]
ctx = int(sys.argv[sys.argv.index('--ctx')+1]) if '--ctx' in sys.argv else 0
words = '--words' in sys.argv; showall = '--all' in sys.argv
J = json.load(open(os.environ.get('DJ','build/datacheck.json')))
U = J['units'][unit]; r = U[sec]
addr = r.get('addr')
if '--addr' in sys.argv: addr = int(sys.argv[sys.argv.index('--addr')+1],16)
if addr is None: sys.exit(f'{unit} {sec}: block not placed ({r.get("state")}): pass --addr HEX')
base = re.sub(r'[^A-Za-z0-9_]', '_', unit)
o = dc.Elf(f'build/check/{base}/out.o')
s = o.by_name[sec]
ours = o.bytes_of(s)
orig = dc.Orig()
theirs = orig.bytes(addr - ctx, s['size'] + 2*ctx)
syms = o.symbols()
secidx = dict(enumerate(o.secs))
cj = json.load(open(os.environ.get('CJ','build/check.json')))
cj = cj.get('results', cj) if isinstance(cj, dict) else cj
local_addr = {}
for sym, rr in cj.items():
    if isinstance(rr, dict) and rr.get('unit') == unit and rr.get('ok') and rr.get('local'):
        local_addr[rr['local']] = orig.funcs[sym][0]
text_funcs = sorted((v, sz, n) for n, v, sz, t, b, sh in syms if t == 2 and sz and sh in secidx and secidx[sh]['nm'] == '.text')
sec_delta = {k: v['addr'] for k, v in U.items() if 'addr' in v}
rel = {off: (si, t) for off, si, t in o.relocs(sec)}
objs = {v: n for n, v, sz, t, b, sh in syms if sh in secidx and secidx[sh]['nm'] == sec and t == 1}
print(f'{unit} {sec} size {len(ours)} orig 0x{addr:08X} status {r.get("state")}')
def resolve(off):
    si, rt = rel[off]
    name, value, size, typ, bind, shndx = syms[si]
    word = struct.unpack_from('<I', ours, off)[0]
    b = None; lab = name
    if typ == 3 and shndx in secidx:
        tsec = secidx[shndx]['nm']; lab = tsec
        if tsec in sec_delta: b = sec_delta[tsec]
        elif tsec == '.text':
            k = bisect_right([f[0] for f in text_funcs], word) - 1
            if k >= 0:
                fs, fz, fn = text_funcs[k]; lab = fn
                oa = orig.funcs[fn][0] if (fn in orig.funcs and cj.get(fn, {}).get('ok')) else local_addr.get(fn)
                if oa is not None: b = oa - fs
    elif name in orig.funcs: b = orig.funcs[name][0]
    elif name in orig.objects: b = orig.objects[name][0]
    if b is not None and rt == 12: b -= dc.GP
    return (None if b is None else (b + word) & 0xffffffff), f'{lab}+{word:x}'
if words:
    for off in range(0, len(ours), 4):
        w = struct.unpack_from('<I', ours, off)[0]
        e = struct.unpack_from('<I', theirs, off+ctx)[0]
        lab = ''
        if off in rel:
            v, lab = resolve(off)
            w = v if v is not None else w
            lab = ('MASK ' if v is None else '') + lab
        mark = '' if w == e else '*'
        if showall or mark or off in objs:
            print(f'+{off:04x} {objs.get(off,""):>22} ours {w:08x} {lab[:40]:40} orig {e:08x} {mark}')
else:
    def strs(b, start):
        out=[]; i=0
        while i < len(b):
            j=b.find(b'\0', i)
            if j<0: j=len(b)
            if j>i: out.append((start+i, b[i:j]))
            i=j+1
        return out
    A = strs(ours, 0); B = strs(theirs, -ctx)
    print('--- OURS strings'); [print(f'  +{off:04x} {b!r}') for off, b in A]
    print('--- ORIG strings'); [print(f'  +{off:04x} {b!r}') for off, b in B]
