#!/usr/bin/env python3
"""Post-link step of the DNAS protection: descriptors and re-encryption of the regions.

Does on the linked binary what Sony's protection tool did on the original binary after
the link: fills the descriptor words and encrypts the self-modifying regions, so that the
shim (DNAS2 `__sce_dnas2_symbol11002`/`12002`, DNAS1 `_sce_dnas_pwrap_main`) decrypts them
at runtime. Cipher and descriptors: docs/dnas.md, tools/dnas2_crypt.py,
tools/dnas{1,2}_decrypt.py.

For every region of config/dnas{1,2}_regions.txt (166, by address):
1. Descriptor words: in our .o they are the placeholder of tools/dnas_subst.py (0xd5c0d5c0),
   which no compiler produces; they are copied from the user's original ELF (asset, never
   committed). They are looked for only in the window [start-44, end+48) (11 words before
   the start, 10 after the end) and inside the region (2 words before each hidden string):
   inside the region the value is the original's plaintext, i.e. the original decrypted
   with the code0 of its descriptor. Outside the windows the placeholder must not appear
   in .text (exits 1).
2. Encryption parameters, read from the linked binary (not from the original): code1 at
   end+32 (the descriptor just written), id at start-12, list of the excluded words (the
   relocations, which the dynamic loader rewrites) from the shim's table: DNAS2 through the
   GOT (the GOT slot of the boundary call names the shim, and each shim has its own table),
   DNAS1 `_sce_dnas_pwrap_info_tbl`.
3. Encryption: BANK_A (= DNAS1 scramble, tools/dnas1_crypt.py proves it primitive by
   primitive) with code1, skipping the excluded words; the regions are written to the file.

Report: for every region, whether our plaintext equals the original's and whether the
ciphertext equals the original's bytes. A differing region does not fail the step (it is a
measure: the difference is in the code, and `make link-cmp` sees it); it exits 1 only for
inconsistencies of the model (misplaced placeholder, unrecognized descriptor, broken
roundtrip).

The descriptors are taken AS is from the original (docs/original-bytes.md, item 2):
valid only for the original's code and addresses. If a protected region is changed they
must be regenerated, or the protection removed.

Usage: python3 tools/relink/dnas_protect.py IN.elf OUT.elf
"""
import os
import re
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
ARGS = [os.path.abspath(a) for a in sys.argv[1:]]   # before the chdir: paths relative to the cwd
os.chdir(ROOT)
from dnas2_crypt import transform, BANK_A, BANK_B, gc   # noqa: E402  (gc = tools/got_callees.py)
import dnas_subst                                       # noqa: E402

PLACEHOLDER = dnas_subst.PLACEHOLDER
REGION_FILES = (('dnas2', 'config/dnas2_regions.txt'), ('dnas1', 'config/dnas1_regions.txt'))
REG_RE = re.compile(r'^(0x[0-9a-f]+) (0x[0-9a-f]+)\s+\d+ .*calls=([0-9a-fx,]+) ')
GP_ORIG = 0x100d9910
PRE, POST = 44, 48            # window of the descriptor words around the region (bytes)
# DNAS2: shim -> GOT slot (relative to gp) of the pointer to its exclusion table
# (tools/dnas2_decrypt.py: TABLES); DNAS1: single table via -23080(gp) (tools/dnas1_decrypt.py)
DNAS2_TBL_SLOT = {0x601170: -17496, 0x614cc0: -13392}
DNAS1_SHIM, DNAS1_TBL_SLOT = 0x5c5030, -23080


class Image:
    def __init__(self, path):
        data, secs = gc.load_elf(path)
        self.buf = bytearray(data)
        self.secs = secs
        self.a2o = gc.make_addr_to_off(secs)
        self.text = [s for s in secs if s['nm'] == '.text'][0]

    def rd(self, a):
        return struct.unpack_from('<I', self.buf, self.a2o(a))[0]

    def words(self, a, n):
        return list(struct.unpack_from('<%dI' % n, self.buf, self.a2o(a))) if n else []

    def put(self, a, ws):
        if ws:
            struct.pack_into('<%dI' % len(ws), self.buf, self.a2o(a), *ws)


def load_table(img, tp):
    """Exclusion table (format of tools/dnas{1,2}_decrypt.py:load_table): id -> (shift, list)."""
    n = img.rd(tp)
    base = tp + 4 + n * 8
    out = {}
    for i in range(n):
        idw, t3 = img.rd(tp + 8 * i + 4), img.rd(tp + 8 * i + 8)
        rec = base + t3
        cnt = img.rd(rec + 4)
        out[idw >> 1] = (img.rd(rec), [img.rd(rec + 8 + 4 * k) for k in range(cnt)])
    return out


def s16(v):
    return v - 0x10000 if v & 0x8000 else v


def regions():
    out = []
    for fam, f in REGION_FILES:
        for line in open(f):
            m = REG_RE.match(line)
            if m:
                calls = [int(c, 16) for c in m.group(3).split(',') if c]
                out.append((int(m.group(1), 16), int(m.group(2), 16), fam, calls))
    return sorted(out)


def main():
    if len(ARGS) != 2:
        sys.exit('usage: python3 tools/relink/dnas_protect.py IN.elf OUT.elf')
    ours = Image(ARGS[0])
    orig = Image('orig/bn-jpn.elf')
    if (ours.text['addr'], ours.text['size']) != (orig.text['addr'], orig.text['size']):
        sys.exit('error: .text differs from the original in address or size: this pass only works on the original layout')
    # gp of the linked binary (o32: _gp = .got + 0x7ff0), not the original's constant
    GP = [s for s in ours.secs if s['nm'] == '.got'][0]['addr'] + 0x7ff0
    if GP != GP_ORIG:
        sys.exit('error: gp 0x%x differs from the original 0x%x: GOT slots of the tables are not valid' % (GP, GP_ORIG))
    regs = regions()
    errors = []
    # --- 1. descriptors: placeholder -> original words (decrypted inside the region)
    windows = []
    n_desc = n_inner = 0
    plain_orig = {}
    for start, end, fam, calls in regs:
        ln = end - start
        c0 = orig.rd(start - 16)
        if (c0 ^ orig.rd(start - 8)) & 0xffffffff != ln:
            errors.append('0x%x: descriptor of the original not recognized' % start)
            continue
        po = transform(orig.words(start, ln // 4), c0, BANK_B, 0, ())
        # the excluded words are not encrypted: they are put back later, with the parameters (step 2)
        plain_orig[start] = po
        lo, hi = start - PRE, end + POST
        windows.append((lo, hi))
        # Exact positions (dnas_subst.BEGIN/END = dnas_ref.desc_addrs): 11 words before
        # the start, 10 after the `b`+nop of the end. All must be placeholders: one that
        # is not means a moved scaffold or an already protected binary (double encryption).
        want = list(range(lo, start, 4)) + list(range(end + 8, hi, 4))
        missing = [a for a in want if ours.rd(a) != PLACEHOLDER]
        if missing:
            errors.append('0x%x: %d descriptor words without a placeholder (first 0x%x)'
                          % (start, len(missing), missing[0]))
            plain_orig.pop(start)
            continue
        for a in want:
            ours.put(a, [orig.rd(a)])
        n_desc += len(want)
        for a in (end, end + 4):
            if ours.rd(a) == PLACEHOLDER:
                errors.append('0x%x: placeholder on the final `b`/nop 0x%x' % (start, a))
        # inside the region: only the pairs before a hidden string, after `b` + nop
        # (dnas_subst.string_block); elsewhere a placeholder would be our code
        # replaced with the original's
        pu = ours.words(start, ln // 4)
        for k, v in enumerate(pu):
            if v != PLACEHOLDER:
                continue
            first = k >= 2 and pu[k - 2] >> 16 == 0x1000 and pu[k - 1] == 0 \
                and k + 1 < len(pu) and pu[k + 1] == PLACEHOLDER
            second = k >= 3 and pu[k - 1] == PLACEHOLDER and pu[k - 3] >> 16 == 0x1000 \
                and pu[k - 2] == 0
            if not (first or second):
                errors.append('0x%x: placeholder inside the region outside a string block 0x%x'
                              % (start, start + 4 * k))
            n_inner += 1         # real value later, once the exclusions are known
    # placeholder outside the windows = broken model
    t0, tn = ours.text['addr'], ours.text['size'] // 4
    tw = ours.words(t0, tn)
    windows.sort()
    stray = []
    for k, v in enumerate(tw):
        if v != PLACEHOLDER:
            continue
        a = t0 + 4 * k
        if not any(lo <= a < hi for lo, hi in windows):
            stray.append(a)
    if stray:
        errors.append('%d placeholders outside the DNAS windows (first 0x%x)' % (len(stray), stray[0]))
    # --- 2./3. parameters from the linked binary and encryption
    tables = {}
    n_same_plain = n_same_cipher = 0
    diff_rows = []
    for start, end, fam, calls in regs:
        if start not in plain_orig:
            continue
        ln = end - start
        c1, i1, lc1 = ours.rd(end + 32), ours.rd(end + 36), ours.rd(end + 40)
        c0, i0 = ours.rd(start - 16), ours.rd(start - 12)
        if (c1 ^ lc1) & 0xffffffff != ln or i1 != (i0 | 1):
            errors.append('0x%x: end descriptor inconsistent after filling' % start)
            continue
        ident = i0 >> 1
        # the shim from the GOT slot of the first boundary call (`lw t9,OFF(gp)`)
        lw = ours.rd(calls[0])
        if lw & 0xffff0000 != 0x8f990000:
            errors.append('0x%x: boundary call 0x%x is not `lw t9,OFF(gp)`' % (start, calls[0]))
            continue
        shim = ours.rd(GP + s16(lw & 0xffff))
        if fam == 'dnas2':
            slot = DNAS2_TBL_SLOT.get(shim)
        else:
            slot = DNAS1_TBL_SLOT if shim == DNAS1_SHIM else None
        if slot is None:
            errors.append('0x%x: unexpected shim 0x%x for %s' % (start, shim, fam))
            continue
        tp = ours.rd(GP + slot)
        if tp not in tables:
            tables[tp] = load_table(ours, tp)
        shift, lst = tables[tp].get(ident, (0, []))
        excl = set(e - shift for e in lst)
        if any(not (0 <= e < ln) or e % 4 for e in excl):
            errors.append('0x%x: exclusion outside the region' % start)
            continue
        po = plain_orig[start]
        ow = orig.words(start, ln // 4)
        po = [ow[k] if 4 * k in excl else po[k] for k in range(len(po))]
        pu = ours.words(start, ln // 4)
        pu = [po[k] if v == PLACEHOLDER else v for k, v in enumerate(pu)]
        enc = transform(pu, c1, BANK_A, 0, excl)
        if transform(enc, c0, BANK_B, 0, excl) != pu:
            errors.append('0x%x: broken roundtrip (code0/code1 inconsistent)' % start)
            continue
        ours.put(start, enc)
        same_p = pu == po
        same_c = enc == ow
        n_same_plain += same_p
        n_same_cipher += same_c
        if not same_c:
            nd = sum(a != b for a, b in zip(enc, ow))
            diff_rows.append('  0x%08x %-5s %5d B  differing words %4d (plaintext %s)'
                             % (start, fam, ln, nd, 'same' if same_p else 'different'))
    nreg = len(regs)
    print('dnas_protect: %d regions, %d descriptor words from the original, %d inside the regions'
          % (nreg, n_desc, n_inner))
    print('dnas_protect: plaintext same as the original %d/%d, ciphertext same %d/%d'
          % (n_same_plain, nreg, n_same_cipher, nreg))
    for r in diff_rows:
        print(r)
    if errors:
        for e in errors:
            print('error: ' + e, file=sys.stderr)
        sys.exit(1)
    tmp = ARGS[1] + '.tmp'
    with open(tmp, 'wb') as f:
        f.write(ours.buf)
    os.replace(tmp, ARGS[1])


if __name__ == '__main__':
    main()
