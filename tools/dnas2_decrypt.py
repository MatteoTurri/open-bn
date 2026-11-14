#!/usr/bin/env python3
"""DNAS2: survey + full static decryption, with exclusion lists.

Writes build/dnas/bn-jpn.dec.elf and build/dnas/regions.txt (plus the
committed copy config/dnas2_regions.txt), and regenerates
config/dnas2_selfmod.txt.

Usage: python3 tools/dnas2_decrypt.py
"""
import struct, sys, collections, bisect, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from dnas2_crypt import (DATA, SECTIONS, A2O, read_words, transform,
                         recipe, BANK_A, BANK_B)

GP = 0x100d9910
TEXT = [s for s in SECTIONS if s['nm'] == '.text'][0]
NW = TEXT['size'] // 4
W = struct.unpack_from('<%dI' % NW, DATA, TEXT['offset'])
def rd32(a):
    o = A2O(a); return None if o is None else struct.unpack_from('<I', DATA, o)[0]
def s16(v): return v - 0x10000 if v & 0x8000 else v

# ------------------------------------------------ exclusion tables
def load_table(tp):
    n = rd32(tp); base = tp + 4 + n*8; out = {}
    for i in range(n):
        idw = rd32(tp+8*i+4); t3 = rd32(tp+8*i+8)
        rec = base + t3
        shift = rd32(rec); cnt = rd32(rec+4)
        lst = [rd32(rec+8+4*k) for k in range(cnt)]
        out[idw >> 1] = (shift, lst)
    return out
TABLES = {0x601170: load_table(rd32(GP-17496)),   # 11002 -> 0x6160b0
          0x614cc0: load_table(rd32(GP-13392))}   # 12002 -> 0x61c7b0

# ------------------------------------------------ call site -> region
GOT = [s for s in SECTIONS if s['nm'] == '.got'][0]
GV = struct.unpack_from('<%dI' % (GOT['size']//4), DATA, GOT['offset'])
shim_off = {}
for i, v in enumerate(GV):
    if v in (0x601170, 0x614cc0): shim_off[(GOT['addr'] + 4*i) - GP] = v

ranges = []
for line in open('config/symbol_addrs.txt'):
    if 'type:func' not in line: continue
    nm = line.split('=')[0].strip()
    a = int(line.split('=')[1].split(';')[0].strip(), 16)
    sz = int(line.split('size:')[1].split()[0], 16)
    if sz: ranges.append((a, a+sz, nm))
ranges.sort(); starts = [r[0] for r in ranges]
def owner(a):
    i = bisect.bisect_right(starts, a) - 1
    return ranges[i][2] if i >= 0 and ranges[i][0] <= a < ranges[i][1] else '<gap>'

def desc0(start):
    g,c0,i0,lc0,pad = (rd32(start-20),rd32(start-16),rd32(start-12),rd32(start-8),rd32(start-4))
    if None in (g,c0,i0,lc0,pad) or g or pad or (i0 & 1): return None
    ln = (c0 ^ lc0) & 0xffffffff
    if ln == 0 or ln % 4 or ln > 0x10000: return None
    c1,i1,lc1 = rd32(start+ln+32), rd32(start+ln+36), rd32(start+ln+40)
    if None in (c1,i1,lc1) or i1 != (i0|1) or ((c1^lc1)&0xffffffff) != ln: return None
    return ln, c0, i0 >> 1, c1

regions = {}
for off, shim in shim_off.items():
    pat = 0x8f990000 | (off & 0xffff)
    for i, w in enumerate(W):
        if w != pat: continue
        C = TEXT['addr'] + 4*i; a0 = None
        for k in range(1, 16):
            j = i - k
            if j < 1: break
            if (W[j] & 0xffff0000) == 0x24840000 and (W[j-1] & 0xffff0000) == 0x8f840000:
                a0 = (rd32(GP + s16(W[j-1] & 0xffff)) + s16(W[j] & 0xffff)) & 0xffffffff
                break
        d = desc0(a0) if a0 else None
        if d: start = a0
        else:
            c1,i1,lc1 = rd32(a0+32), rd32(a0+36), rd32(a0+40)
            start = a0 - ((c1 ^ lc1) & 0xffffffff); d = desc0(start)
        regions.setdefault(start, (d, shim, []))[2].append(C)

# ------------------------------------------------ decrypt + checks
buf = bytearray(DATA)
rows = []; n_excl = 0; n_excl_words = 0; oob = 0
for start, ((ln, code0, ident, code1), shim, cs) in sorted(regions.items()):
    shift, lst = TABLES[shim].get(ident, (0, []))
    excl = set(e - shift for e in lst)
    bad = [e for e in excl if not (0 <= e < ln) or e % 4]
    oob += len(bad)
    if excl: n_excl += 1; n_excl_words += len(excl)
    n = ln // 4
    w = read_words(start, n)
    dec = transform(w, code0, BANK_B, 0, excl)
    assert transform(dec, code1, BANK_A, 0, excl) == w, 'roundtrip failed at 0x%x' % start
    struct.pack_into('<%dI' % n, buf, A2O(start), *dec)
    rows.append((start, ln, ident, recipe(code0)[1], len(excl), bad, cs, owner(start)))

VERBOSE = os.environ.get('VERBOSE', '') not in ('', '0') or '-v' in sys.argv[1:]
if VERBOSE:
    print('regions: %d  bytes: %d' % (len(rows), sum(r[1] for r in rows)))
    print('regions with an exclusion list: %d  excluded words: %d' % (n_excl, n_excl_words))
if oob:
    print('warning: %d exclusion offsets outside the region or misaligned' % oob)
os.makedirs('build/dnas', exist_ok=True)
open('build/dnas/bn-jpn.dec.elf','wb').write(bytes(buf))
lines = ['0x%08x 0x%08x %5d id=%-11d chain=%-22s excl=%-3d calls=%s %s\n'
         % (start, start+ln, ln, ident, ch, ne, ','.join(hex(c) for c in cs), o)
         for start, ln, ident, ch, ne, bad, cs, o in rows]
open('build/dnas/regions.txt', 'w').writelines(lines)
open('config/dnas2_regions.txt', 'w').writelines(lines)   # committed copy

# --- config/dnas2_selfmod.txt: the .dynsym functions that contain an encrypted
# region. Each site embeds 21 words of literal data + 2 `b` inside the body,
# which gcc does not emit from C: the build substitutes them (tools/dnas_subst.py).
# Regenerated from here: do not edit it by hand.
SYMSIZE = {nm: e - a for a, e, nm in ranges}
_owners = sorted({o for *_, o in rows if o in SYMSIZE})
with open('config/dnas2_selfmod.txt', 'w') as f:
    f.write('# Functions with a self-modifying DNAS2 region.\n'
            '# GENERATED by tools/dnas2_decrypt.py -- do not edit by hand.\n'
            '#\n'
            '# The body of these functions contains an encrypted region that the\n'
            '# __sce_dnas2_symbol11002 shim decrypts at runtime (mprotect RWX -> transform ->\n'
            '# cacheflush -> mprotect RX). The cipher is reconstructed and the plaintext is\n'
            '# readable (`python3 tools/dnas2_decrypt.py` writes the decrypted image).\n'
            '# Each site also embeds 21 words of literal data plus 2 `b` placed inside\n'
            '# the body (100 bytes per site) that gcc cannot emit from C: the build puts\n'
            '# that scaffold in place of the markers of src/dnas/dnas_prot.h\n'
            '# (tools/dnas_subst.py), and the gate compares the whole function with the\n'
            '# decrypted image, descriptors masked (tools/dnas_ref.py). See\n'
            '# docs/dnas.md.\n'
            '#\n'
            '# %d functions / %d bytes.\n' % (len(_owners), sum(SYMSIZE[o] for o in _owners)))
    for o in _owners:
        f.write('%s\n' % o)
print('dnas2      %d regions decrypted (roundtrip ok), %d functions / %d B in config/dnas2_selfmod.txt'
      % (len(rows), len(_owners), sum(SYMSIZE[o] for o in _owners)))
