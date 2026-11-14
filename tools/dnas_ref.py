#!/usr/bin/env python3
"""Reference of the self-modifying DNAS functions (163: 89 named + 74 static).

The functions with an encrypted region are decompiled like all the others
(docs/dnas.md). The reference is the whole function of the decrypted
image (tools/dnas2_decrypt.py + tools/dnas1_decrypt.py ->
build/dnas/bn-jpn.dec-all.elf), scaffold included, with no filtering. Our side
gets the scaffold from the build's substitution step (tools/dnas_subst.py,
macros in src/dnas/dnas_prot.h). Only the words no compiler produces are
masked (`desc_addrs`: 11 words before each region, 10 after the end, 2 before
each hidden string): in our .o they are placeholders, in the binary they are
assets, and `reference_rows` writes them with the placeholder text so the two
sides compare equal. The cipher is not part of the comparison: the gate
compares functions, not a linked binary, and the reference is the decrypted
image.

Anatomy of a site (e.g. _sce_dnas1_keep_password @0x5c081c):
    la   rX, desc0            desc0 = start-20 (5 words: flag, code, id|dir, len^code, 0)
    lw   v0, 0(rX) ; bnez v0, <b> ; nop   if already decrypted, skip the call
    la   a0, start ; lw a1/a2/a3 from desc0 ; jal shim ; lw gp
    b    start ; nop ; [6 words] [5 words desc0]
    <encrypted region, len bytes; the relocations inside are not encrypted>
    b    <post> ; nop ; [6 words] [3 words desc1, at end+32] [1 zero word]
    la   v0, desc1 ; la a0, end ; lw a1/a2/a3 ; jal shim ; lw gp
The whole scaffold is 27 + 23 rows, with one shape (up to offsets) on all 166
regions: `--verify` checks the shape row by row (_PRE_SHAPE / _POST_ARGS), not
just the counts. `cuts` returns the scaffold rows: pre = [la rX,desc0 .. lw t9)
+ [b start .. start) = 10 + 13 rows; post = [end .. lw t9 of the second call) =
2 + 10 words + 7 argument rows = 19 rows; the 4 rows of each call remain. The
only real rows inside the post are `move s1,zero` and `move s2,zero` (@0x5e6068,
@0x5e6bc8), kept: see below. Normalization (tools/normalize_asm.py) computes
branch targets by index among the rows present, so removing rows is enough: no
renumbering is needed. Branches pointing inside a cut become EXT.

Use as a module (check.py, split.py, gotident.py):
    dnas_ref.protected()              -> set of symbols (config/dnas*_selfmod.txt + statics)
    dnas_ref.ensure_image()           -> path of the decrypted image (created if missing)
    dnas_ref.desc_addrs(rows)         -> addresses of the descriptor words (rows = parse_rows
                                         of the whole image)
    dnas_ref.reference_rows(rows_slice, desc) -> reference (descriptors masked)
    dnas_ref.cuts(rows)               -> addresses of the scaffold (shape check with
                                         --verify; tools/gotident.py skips its GOT accesses)
From the shell (in the container):  python3 tools/dnas_ref.py --verify   (pattern anomalies)
"""
import os
import re
import struct
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import selfmod

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
IMAGE = 'build/dnas/bn-jpn.dec-all.elf'
REGION_FILES = ('config/dnas2_regions.txt', 'config/dnas1_regions.txt')
GP = 0x100D9910
REG_RE = re.compile(r'^(0x[0-9a-f]+) (0x[0-9a-f]+)\s+\d+ .*calls=([0-9a-fx,]+) (\S+)')
PRE_MAX = 64                                  # max rows to walk back to find `la rX,desc0`
_SCAF_REGS = ('v0', 'a0', 'a1', 'a2', 'a3')   # registers written by the shim setup

# Shape of the scaffold (mnemonic, operand regex; {r} = desc0 register).
# PRE: from `la rX,desc0` to the first call, excluded.
_PRE_SHAPE = [('lw', r'{r},-?\d+\(gp\)'), ('addiu', r'{r},{r},-?\d+'), ('lw', r'v0,0\({r}\)'),
              ('bnez', r'v0,[0-9a-f]+'), ('nop', r''), ('lw', r'a0,-?\d+\(gp\)'),
              ('addiu', r'a0,a0,-?\d+'), ('lw', r'a1,12\({r}\)'), ('lw', r'a2,4\({r}\)'),
              ('lw', r'a3,8\({r}\)')]
# POST: the 7 argument rows of the second call, after `b` + `nop` + 10 words.
_POST_ARGS = [('lw', r'v0,-?\d+\(gp\)'), ('addiu', r'v0,v0,-?\d+'), ('lw', r'a0,-?\d+\(gp\)'),
              ('addiu', r'a0,a0,-?\d+'), ('lw', r'a1,8\(v0\)'), ('lw', r'a2,0\(v0\)'),
              ('lw', r'a3,4\(v0\)')]


def _shape_ok(got, shape, r=''):
    return len(got) == len(shape) and all(
        mn == m and re.fullmatch(pat.format(r=r), ops.strip())
        for (mn, ops), (m, pat) in zip(got, shape))


def protected(syms=None):
    """Names of the protected (self-modifying) functions: the generated lists
    config/dnas{1,2}_selfmod.txt (.dynsym) plus, if `syms` {name: (addr, size)}
    is given, the unnamed functions (tools/statics.py, `static_<ADDR>`) whose
    body intersects an encrypted region: 60 DNAS2 and 17 DNAS1 regions sit in
    gaps/statics, and a list written by name cannot hold them -- their identity
    is the address."""
    out = set(selfmod.load())
    if syms:
        import statics
        regs = [(a, b) for a, b, _, _ in regions()]
        for n, (a, s) in syms.items():
            if s and statics.is_static(n) and any(a < e and st < a + s for st, e in regs):
                out.add(n)
    return out


def regions():
    """[(start, end, [call1, call2], owner)] from config/dnas{1,2}_regions.txt (committed)."""
    out = []
    for f in REGION_FILES:
        for line in open(os.path.join(ROOT, f)):
            m = REG_RE.match(line)
            if m:
                calls = [int(c, 16) for c in m.group(3).split(',') if c]
                out.append((int(m.group(1), 16), int(m.group(2), 16), calls, m.group(4)))
    return sorted(out)


def ensure_image():
    p = os.path.join(ROOT, IMAGE)
    if not os.path.exists(p):
        for tool in ('tools/dnas2_decrypt.py', 'tools/dnas1_decrypt.py'):
            r = subprocess.run([sys.executable, tool], cwd=ROOT, capture_output=True, text=True)
            if r.returncode != 0:
                sys.exit(f'error: {tool} failed:\n{r.stderr[-2000:]}')
        if not os.path.exists(p):
            sys.exit(f'error: {IMAGE} not produced')
    return p


_got = None


def _got_read(off):
    """Value of the GOT slot at gp+off (the GOT is not encrypted: read from the original ELF)."""
    global _got
    if _got is None:
        import importlib.util
        spec = importlib.util.spec_from_file_location('gc', os.path.join(ROOT, 'tools/got_callees.py'))
        gc = importlib.util.module_from_spec(spec); spec.loader.exec_module(gc)
        data, secs = gc.load_elf(os.path.join(ROOT, gc.ELF))
        _got = (data, gc.make_addr_to_off(secs))
    data, a2o = _got
    o = a2o(GP + off)
    return None if o is None else struct.unpack_from('<I', data, o)[0]


def _s16(v):
    return v - 0x10000 if v & 0x8000 else v


LA_LW = re.compile(r'^(\w+),(-?\d+)\(gp\)$')
LA_ADDIU = re.compile(r'^(\w+),(\w+),(-?\d+)$')


def _la_target(rows, i):
    """If rows[i-1],rows[i] are `lw rX,OFF(gp)` + `addiu rX,rX,IMM`, the address loaded."""
    if i < 1:
        return None
    a_addr, a_mn, a_ops = rows[i]
    l_addr, l_mn, l_ops = rows[i - 1]
    if a_mn != 'addiu' or l_mn != 'lw':
        return None
    ma, ml = LA_ADDIU.match(a_ops), LA_LW.match(l_ops)
    if not ma or not ml or ma.group(1) != ma.group(2) or ma.group(1) != ml.group(1):
        return None
    base = _got_read(int(ml.group(2)))
    return None if base is None else (base + int(ma.group(3))) & 0xffffffff


def _inline_data_cuts(rows, index, start, end):
    """Data blocks embedded in the code of a protected region: a forward `b`
    jumping over data words (a string with its padding), followed by a
    `lw GOT_PAGE`/`addiu` pair pointing inside the skipped block. gcc never
    emits data in the instruction stream from C: a literal goes to `.rodata`
    and is addressed with the same pair. It is the protection that moves the
    string inside the encrypted region (the DNAS server hostnames), so it is
    scaffold like the descriptor. The build puts the block back
    (tools/dnas_subst.py, SCE_DNAS_STR marker) and only the 2 words before the
    string are masked (desc_addrs). Found by --verify on 31 sites in 4
    functions.
    Returns the list of (addr_b, addr_target, addr_la_target)."""
    out = []
    k, ke = index[start], index[end]
    while k < ke:
        addr, mn, ops = rows[k]
        if mn == 'b':
            m = re.match(r'([0-9a-f]+)', ops.strip())
            tgt = int(m.group(1), 16) if m else None
            if tgt and addr < tgt <= end and tgt in index:
                ti = index[tgt]
                for q in range(ti, min(ti + 4, len(rows))):
                    la = _la_target(rows, q)
                    if la is not None and addr + 8 <= la < tgt:
                        out.append((k, ti, la))
                        k = ti
                        break
        k += 1
    return out


def cuts(rows, verbose=False):
    """Addresses of the scaffold, for all regions. rows = parse_rows of the whole image."""
    index = {r[0]: i for i, r in enumerate(rows)}
    cut, anomalies = set(), []
    for start, end, calls, owner in regions():
        if start not in index or end not in index:
            anomalies.append((owner, start, 'region not in the disassembly'))
            continue
        i0 = index[start]
        # PRE: walk back to `la rX,desc0` (desc0 = start-20)
        pre_i = None
        for i in range(i0 - 1, max(0, i0 - PRE_MAX) - 1, -1):
            if _la_target(rows, i) == start - 20:
                pre_i = i - 1
                break
        if pre_i is None:
            anomalies.append((owner, start, 'la desc0 not found'))
        elif not calls or calls[0] not in index or not (pre_i < index[calls[0]] < i0):
            anomalies.append((owner, start, 'first call outside the pre block'))
        else:
            c0 = index[calls[0]]
            # expected: lw t9 / jalr / nop / lw gp / b start / nop / 11 words
            shape = [rows[c0][1], rows[c0 + 1][1], rows[c0 + 2][1], rows[c0 + 3][1], rows[c0 + 4][1]]
            if shape != ['lw', 'jalr', 'nop', 'lw', 'b'] or not rows[c0 + 4][2].strip().startswith('%x' % start):
                anomalies.append((owner, start, f'unexpected shape of the first call: {shape}'))
            # the shape of the guard and of the arguments, row by row: a real
            # instruction scheduled in here would be cut without the row count
            # alone telling
            r = rows[pre_i][2].split(',')[0].strip()
            if not _shape_ok([(x[1], x[2]) for x in rows[pre_i:c0]], _PRE_SHAPE, r):
                anomalies.append((owner, start, 'unexpected shape of the pre guard/arguments'))
            if rows[c0 + 5][1] != 'nop' or i0 - (c0 + 6) != 11:
                anomalies.append((owner, start, f'pre: b start + nop + {i0 - (c0 + 6)} words (11 required)'))
            if verbose:
                n = i0 - pre_i
                if n != 27:
                    anomalies.append((owner, start, f'pre of {n} lines, standard 27 (expected)'))
            cut.update(rows[k][0] for k in range(pre_i, c0))          # guard + arguments
            cut.update(rows[k][0] for k in range(c0 + 4, i0))         # b + nop + 11 words
        # POST: from end to `lw gp` after the second jal
        if len(calls) < 2 or calls[1] not in index:
            anomalies.append((owner, start, 'second call missing'))
            continue
        j = index[calls[1]]
        if rows[j][1] != 'lw' or not rows[j][2].startswith('t9,'):
            anomalies.append((owner, start, f'call2 is not lw t9: {rows[j][1]} {rows[j][2]}'))
        k = j + 1
        while k < len(rows) and rows[k][1] in ('jalr', 'nop'):
            k += 1
        if k < len(rows) and rows[k][1] == 'lw' and rows[k][2].startswith('gp,'):
            k += 1
        e0 = index[end]
        if rows[e0][1] != 'b':
            anomalies.append((owner, start, f'no b at end: {rows[e0][1]}'))
        if verbose and (k - e0) != 23:
            anomalies.append((owner, start, f'post of {k - e0} lines, standard 23, real lines kept below (expected)'))
        if rows[e0 + 1][1] != 'nop':
            anomalies.append((owner, start, f'no nop at end+4: {rows[e0 + 1][1]}'))
        # b + nop + 10 words (6 + desc1 + zero): always scaffold
        cut.update(rows[m][0] for m in range(e0, min(e0 + 12, j)))
        # arguments of the second call: the scaffold writes only v0 and a0-a3. A
        # row writing another register is real program code the scheduler put
        # there (`move s1,zero` = `i = 0` of the loop after the region) and is
        # kept (the two 24-row posts @0x5e6068 and @0x5e6bc8 are exactly this).
        args = []
        for m in range(e0 + 12, j):
            mn, ops = rows[m][1], rows[m][2]
            dest = ops.split(',')[0].strip() if ops else ''
            if mn in ('lw', 'addiu', 'lui', 'ori', 'move', 'addu') and dest in _SCAF_REGS:
                cut.add(rows[m][0])
                args.append((mn, ops))
            elif verbose:
                anomalies.append((owner, start, f'real line kept in the post: 0x{rows[m][0]:x} {mn} {ops} (expected)'))
        # the rows removed must be exactly `la v0,desc1` / `la a0,end` / `lw a1-a3`
        if not _shape_ok(args, _POST_ARGS):
            anomalies.append((owner, start, f'unexpected shape of the post arguments: {args}'))
        # Data inside the region (strings hidden by the protection)
        for kb, ti, la in _inline_data_cuts(rows, index, start, end):
            last = ti if rows[ti][1] == 'nop' else ti - 1
            cut.update(rows[m][0] for m in range(kb, last + 1))
            if verbose:
                anomalies.append((owner, start, f'inline data removed: b@0x{rows[kb][0]:x} '
                                  f'-> 0x{rows[ti][0]:x}, la -> 0x{la:x} ({last + 1 - kb} lines) (expected)'))
    return cut, anomalies


def desc_addrs(rows):
    """Addresses of the words no compiler produces, for all regions:
    [start-44, start) (6 words + start descriptor), [end+8, end+48) (6 words +
    end descriptor + zero) and the 2 words before each hidden string
    (`_inline_data_cuts`). rows = parse_rows of the whole image. In our .o they
    are the placeholders of tools/dnas_subst.py (same shape on all 166 regions
    and 31 strings)."""
    index = {r[0]: i for i, r in enumerate(rows)}
    out = set()
    for start, end, _, _ in regions():
        out.update(range(start - 44, start, 4))
        out.update(range(end + 8, end + 48, 4))
        if start in index and end in index:
            for _, _, la in _inline_data_cuts(rows, index, start, end):
                out.update((la - 8, la - 4))
    return out


def reference_rows(rows_slice, desc):
    """Reference: the whole function, with the descriptor words written with
    the placeholder text (tools/dnas_subst.py:PLACEHOLDER)."""
    import dnas_subst
    w = '0x%08x' % dnas_subst.PLACEHOLDER
    return [[r[0], '.word', w] if r[0] in desc else r for r in rows_slice]


def main():
    import normalize_asm
    verify = '--verify' in sys.argv
    img = ensure_image()
    dis = subprocess.run(['mipsel-linux-gnu-objdump', '-z', '-d', img],
                         capture_output=True, text=True, cwd=ROOT).stdout
    rows = normalize_asm.parse_rows(dis.splitlines())
    cut, anomalies = cuts(rows, verbose=verify)
    regs = regions()
    hard = [a for a in anomalies if '(expected)' not in a[2]]
    loud = os.environ.get('VERBOSE', '') not in ('', '0') or '-v' in sys.argv[1:]
    for owner, start, what in anomalies:
        if loud or '(expected)' not in what:
            print(f'{"error" if "(expected)" not in what else "note"}: {owner} @0x{start:x}: {what}')
    print(f'dnas_ref   scaffold pattern {"FAILED on " + str(len(hard)) + " site(s)" if hard else "ok"}'
          f' in {len(regs)} regions ({len(protected())} protected functions)')
    return 1 if hard else 0


if __name__ == '__main__':
    sys.exit(main())
