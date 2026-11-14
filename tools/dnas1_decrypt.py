#!/usr/bin/env python3
"""DNAS1: survey + full static decryption, with exclusion lists.

Writes build/dnas/bn-jpn.dec1.elf and build/dnas/regions1.txt (plus the
committed copy config/dnas1_regions.txt), and regenerates
config/dnas1_selfmod.txt. If build/dnas/bn-jpn.dec.elf exists (the DNAS2
image produced by tools/dnas2_decrypt.py) it also writes
build/dnas/bn-jpn.dec-all.elf with both families decrypted.

Usage: python3 tools/dnas1_decrypt.py [--verify]
       --verify disassembles every decrypted region again with
       mipsel-linux-gnu-objdump (inside the open-bn-env container) and
       checks that it is 100% valid MIPS and that branch/jal targets fall
       inside .text.
"""
import struct
import sys
import bisect
import os
import re
import subprocess

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from dnas1_crypt import (DATA, SECTIONS, A2O, read_words, transform,   # noqa
                         recipe, BANK_SCRAMBLE, BANK_DESCRAMBLE)

GP = 0x100d9910
SHIM = 0x5c5030            # _sce_dnas_pwrap_main
INFO_TBL = 0x5d9a00        # _sce_dnas_pwrap_info_tbl (via -23080(gp))

TEXT = [s for s in SECTIONS if s['nm'] == '.text'][0]
TEXT_LO, TEXT_HI = TEXT['addr'], TEXT['addr'] + TEXT['size']
NW = TEXT['size'] // 4
W = struct.unpack_from('<%dI' % NW, DATA, TEXT['offset'])


def rd32(a):
    o = A2O(a)
    return None if o is None else struct.unpack_from('<I', DATA, o)[0]


def s16(v):
    return v - 0x10000 if v & 0x8000 else v


# ------------------------------------------------ exclusion table
# same layout as DNAS2: [n] then n pairs (idw, off), then the records
# (shift, count, list[count]) at base+off. Read by <gap 0x5c5220>.
def load_table(tp):
    n = rd32(tp)
    base = tp + 4 + n * 8
    out = {}
    for i in range(n):
        idw = rd32(tp + 8 * i + 4)
        t3 = rd32(tp + 8 * i + 8)
        rec = base + t3
        shift = rd32(rec)
        cnt = rd32(rec + 4)
        lst = [rd32(rec + 8 + 4 * k) for k in range(cnt)]
        out[idw >> 1] = (shift, lst)
    return out


TABLE = load_table(INFO_TBL)

# ------------------------------------------------ call site -> region
GOT = [s for s in SECTIONS if s['nm'] == '.got'][0]
GV = struct.unpack_from('<%dI' % (GOT['size'] // 4), DATA, GOT['offset'])
SHIM_OFFS = [(GOT['addr'] + 4 * i) - GP for i, v in enumerate(GV) if v == SHIM]

ranges = []
for line in open('config/symbol_addrs.txt'):
    if 'type:func' not in line:
        continue
    nm = line.split('=')[0].strip()
    a = int(line.split('=')[1].split(';')[0].strip(), 16)
    sz = int(line.split('size:')[1].split()[0], 16)
    if sz:
        ranges.append((a, a + sz, nm))
ranges.sort()
starts = [r[0] for r in ranges]


def owner(a):
    i = bisect.bisect_right(starts, a) - 1
    return ranges[i][2] if i >= 0 and ranges[i][0] <= a < ranges[i][1] else '<gap>'


def desc0(start):
    """Descriptor around the region (same as DNAS2):
         start-20 guard | start-16 code0 | start-12 id<<1 | start-8 len^code0
         start-4  pad(0)
         start+len+32 code1 | +36 (id<<1)|1 | +40 len^code1
       Unlike DNAS2, len == 0 is accepted too: the binary has one degenerate
       region of zero length (0x5e994c, call sites 0x5e9908/0x5e9998)."""
    g, c0, i0, lc0, pad = (rd32(start - 20), rd32(start - 16), rd32(start - 12),
                           rd32(start - 8), rd32(start - 4))
    if None in (g, c0, i0, lc0, pad) or g or pad or (i0 & 1):
        return None
    ln = (c0 ^ lc0) & 0xffffffff
    if ln % 4 or ln > 0x10000:
        return None
    c1, i1, lc1 = rd32(start + ln + 32), rd32(start + ln + 36), rd32(start + ln + 40)
    if None in (c1, i1, lc1) or i1 != (i0 | 1) or ((c1 ^ lc1) & 0xffffffff) != ln:
        return None
    return ln, c0, i0 >> 1, c1


regions = {}
n_sites = 0
for off in SHIM_OFFS:
    pat = 0x8f990000 | (off & 0xffff)
    for i, w in enumerate(W):
        if w != pat:
            continue
        n_sites += 1
        C = TEXT['addr'] + 4 * i
        a0 = None
        for k in range(1, 16):
            j = i - k
            if j < 1:
                break
            if (W[j] & 0xffff0000) == 0x24840000 and (W[j - 1] & 0xffff0000) == 0x8f840000:
                a0 = (rd32(GP + s16(W[j - 1] & 0xffff)) + s16(W[j] & 0xffff)) & 0xffffffff
                break
        assert a0 is not None, 'call site 0x%x without an a0 setup' % C
        d = desc0(a0)
        if d:                                   # dir=0: a0 is the start
            start = a0
        else:                                   # dir=1: a0 is the end
            c1, lc1 = rd32(a0 + 32), rd32(a0 + 40)
            start = a0 - ((c1 ^ lc1) & 0xffffffff)
            d = desc0(start)
        assert d, 'call site 0x%x: unrecognized descriptor (a0=0x%x)' % (C, a0)
        regions.setdefault(start, (d, []))[1].append(C)

# ------------------------------------------------ decrypt + checks
buf = bytearray(DATA)
rows = []
n_excl = n_excl_words = oob = 0
n_id_in_tbl = 0
for start, ((ln, code0, ident, code1), cs) in sorted(regions.items()):
    shift, lst = TABLE.get(ident, (0, []))
    if ident in TABLE:
        n_id_in_tbl += 1
    excl = set(e - shift for e in lst)
    bad = [e for e in excl if not (0 <= e < ln) or e % 4]
    oob += len(bad)
    if excl:
        n_excl += 1
        n_excl_words += len(excl)
    n = ln // 4
    w = read_words(start, n)
    dec = transform(w, code0, BANK_DESCRAMBLE, 0, excl)
    assert transform(dec, code1, BANK_SCRAMBLE, 0, excl) == w, 'roundtrip failed at 0x%x' % start
    if n:
        struct.pack_into('<%dI' % n, buf, A2O(start), *dec)
    rows.append((start, ln, ident, recipe(code0)[1], len(excl), bad, cs, owner(start)))

VERBOSE = os.environ.get('VERBOSE', '') not in ('', '0') or '-v' in sys.argv[1:]
if VERBOSE:
    print('call sites of the _sce_dnas_pwrap_main shim: %d' % n_sites)
    print('regions: %d  bytes: %d' % (len(rows), sum(r[1] for r in rows)))
    print('ids present in _sce_dnas_pwrap_info_tbl: %d / %d' % (n_id_in_tbl, len(rows)))
    print('regions with an exclusion list: %d  excluded words: %d' % (n_excl, n_excl_words))
if oob:
    print('warning: %d exclusion offsets outside the region or misaligned' % oob)

os.makedirs('build/dnas', exist_ok=True)
open('build/dnas/bn-jpn.dec1.elf', 'wb').write(bytes(buf))
if os.path.exists('build/dnas/bn-jpn.dec.elf'):
    b2 = bytearray(open('build/dnas/bn-jpn.dec.elf', 'rb').read())
    for start, ln, *_ in rows:
        if ln:
            b2[A2O(start):A2O(start) + ln] = buf[A2O(start):A2O(start) + ln]
    open('build/dnas/bn-jpn.dec-all.elf', 'wb').write(bytes(b2))
else:
    print('warning: build/dnas/bn-jpn.dec.elf missing (run tools/dnas2_decrypt.py first): '
          'build/dnas/bn-jpn.dec-all.elf not written')
lines = ['0x%08x 0x%08x %5d id=0x%08x chain=%-18s excl=%-3d calls=%s %s\n'
         % (start, start + ln, ln, ident, ','.join(str(x) for x in ch),
            ne, ','.join(hex(c) for c in cs), o)
         for start, ln, ident, ch, ne, bad, cs, o in rows]
open('build/dnas/regions1.txt', 'w').writelines(lines)
open('config/dnas1_regions.txt', 'w').writelines(lines)   # committed copy

# --- config/dnas1_selfmod.txt (same logic as dnas2_decrypt.py)
SYMSIZE = {nm: e - a for a, e, nm in ranges}
_owners = sorted({o for *_, o in rows if o in SYMSIZE})
with open('config/dnas1_selfmod.txt', 'w') as f:
    f.write('# Functions with a self-modifying DNAS1 region.\n'
            '# GENERATED by tools/dnas1_decrypt.py -- do not edit by hand.\n'
            '#\n'
            '# The body of these functions contains an encrypted region that the\n'
            '# _sce_dnas_pwrap_main shim (0x5c5030) decrypts at runtime (mprotect RWX ->\n'
            '# _sce_dnas_pwrap_descramble -> cacheflush -> mprotect RX) and encrypts\n'
            '# again right after (_sce_dnas_pwrap_scramble). The cipher is\n'
            '# bit-for-bit the same as DNAS2 (same 8 primitives, same constants\n'
            '# K1=0x09F8ED17 K2=0xA95128C3, same bank order), so the plaintext is\n'
            '# readable (`python3 tools/dnas1_decrypt.py` writes the decrypted image).\n'
            '# Each site also embeds the descriptor (5 words before the region + 3\n'
            '# after), the encrypted region itself and the `b` jumps around it, which\n'
            '# gcc cannot emit from C: the build puts that scaffold in place of the\n'
            '# markers of src/dnas/dnas_prot.h (tools/dnas_subst.py), and the gate\n'
            '# compares the whole function with the decrypted image, descriptors\n'
            '# masked (tools/dnas_ref.py). See docs/dnas.md.\n'
            '#\n'
            '# %d functions / %d bytes.\n' % (len(_owners), sum(SYMSIZE[o] for o in _owners)))
    for o in _owners:
        f.write('%s\n' % o)
print('dnas1      %d regions decrypted (roundtrip ok), %d functions / %d B in config/dnas1_selfmod.txt'
      % (len(rows), len(_owners), sum(SYMSIZE[o] for o in _owners)))

# ------------------------------------------------ check by disassembly
BAD_RE = re.compile(r'\.word\s+0x')
TGT_RE = re.compile(r'^\s*([0-9a-f]+):\s+[0-9a-f]{8}\s+(\S+)\s+(.*)$')


STR_RE = re.compile(rb'[ -~]{6,}')


def _verify_image(img, blob):
    """Disassemble every region of `img` again and count the words that are
    not valid MIPS. Words covered by a run of printable ASCII at least 6
    characters long are counted separately: part of the DNAS1 regions is a
    string pool (DNAS server URLs, HTTP headers), not code; they are the
    strongest evidence that decryption is right, but do not disassemble."""
    tot = strw = badw = badt = 0
    for start, ln, *_ in rows:
        if not ln:
            continue
        b = blob[A2O(start):A2O(start) + ln]
        smask = set()
        for m in STR_RE.finditer(b):
            for e in range((m.start() // 4) * 4, ((m.end() + 3) // 4) * 4, 4):
                smask.add(start + e)
        out = subprocess.run(
            ['docker', 'run', '--rm', '-v', os.getcwd() + ':/w', '-w', '/w',
             'open-bn-env', 'mipsel-linux-gnu-objdump', '-d',
             '--start-address=0x%x' % start, '--stop-address=0x%x' % (start + ln),
             img], capture_output=True, text=True).stdout
        for line in out.splitlines():
            m = TGT_RE.match(line)
            if not m:
                continue
            if int(m.group(1), 16) in smask:
                strw += 1
                continue
            tot += 1
            if BAD_RE.search(line):
                badw += 1
                continue
            mn, ops = m.group(2), m.group(3)
            if mn.startswith(('b', 'j')) and mn != 'break':
                mt = re.search(r'\b([0-9a-f]{6,8})\s+<', ops)
                if mt and not (TEXT_LO <= int(mt.group(1), 16) < TEXT_HI):
                    badt += 1
    nall = sum(ln for _s, ln, *_ in rows) // 4
    # objdump compresses runs of zeros into `...`: those words produce no row.
    print('  %-30s words %4d = %4d code + %4d ASCII strings + %4d elided zeros'
          ' | undecodable: %3d | targets outside .text: %3d'
          % (os.path.basename(img), nall, tot, strw, nall - tot - strw,
             badw, badt))
    return tot, badw + badt


def verify():
    """Encrypted vs decrypted comparison: the decisive measure."""
    print('verify by disassembly (Sony binutils in the container):')
    _t0, b0 = _verify_image('orig/bn-jpn.elf', DATA)
    t1, b1 = _verify_image('build/dnas/bn-jpn.dec1.elf',
                           open('build/dnas/bn-jpn.dec1.elf', 'rb').read())
    print('  encrypted: %d invalid words  ->  decrypted: %d of %d'
          % (b0, b1, t1))
    return b1 == 0


if '--verify' in sys.argv:
    ok = verify()
    print('verify: %s' % ('ok' if ok else 'FAILED'))
