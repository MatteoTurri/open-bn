#!/usr/bin/env python3
"""Build step that substitutes the DNAS scaffold (docs/dnas.md).

The model: in Sony's source every boundary of a protected region was an
ordinary C call preceded by an asm marker (in DNAS1 only at the end) that
leaves the descriptor and the label in the fixed registers of the scaffold,
and the call reads its fields from them; after compilation a tool replaced
those calls and markers in the assembler text with the scaffold (flag test,
`la` of the descriptor, call to the shim, `b` over the descriptor words),
which has the same shape on all 166 regions. This module is that tool: it
runs on the `.s` of every unit after gcc and before gas, in tools/check.py
and in tools/match.

What it substitutes (the macros are in src/dnas/dnas_prot.h). The boundary
calls go to the real SHIM (DNAS2 `__sce_dnas2_symbol11002`/`12002`, DNAS1
`_sce_dnas_pwrap_main`), and the scaffold calls the same symbol; start and
end are told apart by the marker:
  [`#DNASBEG $3`] ... `jal SHIM`           -> start scaffold (marker removed;
                                             arguments removed: DNAS2 a0 = &&label
                                             and a1-a3 loaded from v1, DNAS1 a0 = &&label)
  `#DNASPRE` (cse barrier before the end, src/dnas/dnas_prot.h) -> removed
  `#DNASEND $2 $4` ... `jal SHIM`          -> end scaffold: the prefix goes on the
                                             marker, the call on the `jal`
                                             (arguments removed: a1-a3 loaded from v0;
                                             a0 is already the marker's output)
  `#DNASSTR $R "text"`  (string hidden inside a region) -> `b T; nop; 2 words;
      text; .align 3; T: nop; la $R,text` (shape seen on all 31 sites)
  the `_sce_dnas_tbl*` tables -> removed
The arguments of the boundary calls are removed on every path reaching the
call (one in a delay slot becomes `nop`). The descriptor words are
placeholders (PLACEHOLDER): bytes no compiler produced, which the gate masks
on both sides (tools/dnas_ref.py:desc_addrs).

Usage: python3 tools/dnas_subst.py IN.s OUT.s   (exits 1 if a substitution fails;
       a .s without markers comes out identical)
As a module: dnas_subst.has_markers(text), dnas_subst.substitute(lines) -> (lines, anomalies, sites).
"""
import re
import sys

PLACEHOLDER = 0xd5c0d5c0
_W = '0x%08x' % PLACEHOLDER

# shape of all 166 regions (tools/dnas_ref.py, _PRE_SHAPE/_POST_ARGS):
# 11 words before the start, the descriptor at start-20 (6th word); after the
# end `b` + nop + 10 words, the descriptor at the 7th.
def begin_text(shim):
    return [x.replace('SHIM', shim) for x in BEGIN]


def end_text(shim):
    return [x.replace('SHIM', shim) for x in END]


BEGIN = ['.set push', '.set reorder',
         'la $3,91f', 'lw $2,0($3)', 'bnez $2,92f',
         'la $4,93f', 'lw $5,12($3)', 'lw $6,4($3)', 'lw $7,8($3)',
         'jal SHIM',
         '92: b 93f',
         '.word ' + ','.join([_W] * 6),
         '91: .word ' + ','.join([_W] * 5),
         '93:', '.set pop']
END = ['.set push', '.set reorder',
       '94: b 96f',
       '.word ' + ','.join([_W] * 6),
       '95: .word ' + ','.join([_W] * 4),
       '96: la $2,95b', 'la $4,94b', 'lw $5,8($2)', 'lw $6,0($2)', 'lw $7,4($2)',
       'jal SHIM', '.set pop']

# shim -> arguments to remove (start, end): all 166 regions call the same shim
# at the start and at the end (see src/dnas/dnas_prot.h). a0..a3 = '4'..'7'.
# At the end a0 is already in the register (marker output): nothing to remove.
SHIMS = {'__sce_dnas2_symbol11002': ('4567', '567'), '__sce_dnas2_symbol12002': ('4567', '567'),
         '_sce_dnas_pwrap_main': ('4567', '567')}
# Fixed descriptor register in the scaffold: v1 at the start, v0 at the end
DESC_REG = {'begin': '3', 'end': '2'}

JAL_RE = re.compile(r'\s*jal\s+(%s)\s*$' % '|'.join(SHIMS))
ARG_RE = re.compile(r'\s*(?:move\s+\$([4-7]),\$0|li\s+\$([4-7]),(?:0x)?[0-9a-f]+(?:\s+#.*)?|la\s+\$(4),\$L\d+)\s*$')
LABEL_RE = re.compile(r'^\$?\w+:')
STR_RE = re.compile(r'\s*#DNASSTR (\$\d+) (".*")\s*$')
TBL_RE = re.compile(r'^(_sce_dnas_tbl\d+\.\d+):\s*$')
MARKERS = re.compile(r'#DNASSTR|#DNASEND|#DNASBEG|#DNASPRE|_sce_dnas_tbl')


def has_markers(text):
    return MARKERS.search(text) is not None


def string_block(n, reg, lit):
    return ['\t.set push', '\t.set reorder', f'\tb 9{n}f',
            '\t.word ' + ','.join([_W] * 2), f'$DSTR{n}:', f'\t.asciiz {lit}',
            '\t.align 3', '\t.set noreorder', f'9{n}:\tnop', '\t.set reorder',
            f'\tla {reg},$DSTR{n}', '\t.set pop']


def substitute_strings(lines):
    out, n = [], 0
    for l in lines:
        m = STR_RE.match(l)
        if m:
            n += 1
            out += string_block(n, m.group(1), m.group(2))
        else:
            out.append(l)
    return out


def drop_tables(lines):
    """Remove the `_sce_dnas_tbl*` tables (label + `.word $Lnn`) and the
    directives before them (`.type`, `.align`): the binary has no trace of them."""
    out = []
    i = 0
    while i < len(lines):
        m = TBL_RE.match(lines[i])
        if m:
            while out and (re.match(r'\s*\.(type|align)\b', out[-1]) and (
                    m.group(1) in out[-1] or re.match(r'\s*\.align\b', out[-1]))):
                out.pop()
            i += 1
            while i < len(lines) and re.match(r'\s*\.word\s+\$L\d+\s*$', lines[i]):
                i += 1
            while i < len(lines) and re.match(r'\s*\.size\s+%s\b' % re.escape(m.group(1)), lines[i]):
                i += 1
            continue
        out.append(lines[i])
        i += 1
    return out


def in_noreorder(out, k):
    for i in range(k, -1, -1):
        m = re.match(r'\s*\.set\s+(noreorder|reorder)\s*$', out[i])
        if m:
            return m.group(1) == 'noreorder'
    return False


def remove_guard(out):
    """Region start: remove the flag guard gcc compiled, `lw $R,0($3)` and the
    branch on $R that skips the call, because the start scaffold has its own
    (`lw $2,0($3); bnez $2,92f`, fixed text). The branch's delay slot: if reorg
    filled it by stealing from the target (branch-likely, or target redirected
    past the region label) it is a copy, and the original stays at the target:
    removed; an instruction taken from above stays where it is (executed on
    both paths, before the scaffold).
    -> None if there is no guard."""
    lo = len(out) - 1
    while lo >= 0 and not re.match(r'\s*(jal|\.ent)\b', out[lo]):
        lo -= 1
    flag = None
    for i in range(len(out) - 1, lo, -1):
        m = re.match(r'\s*lw\s+(\$\d+),0\(\$3\)\s*$', out[i])
        if m:
            flag = m.group(1)
            del out[i]
            at = i
            break
    if flag is None:
        return None
    rgn = None
    for t in out[lo + 1:]:
        ml = re.match(r'\s*la\s+\$4,(\$L\d+)\s*$', t)
        if ml:
            rgn = ml.group(1)
    for i in range(at, len(out)):
        mb = re.match(r'\s*b(ne|eq)z?(l?)\s+%s\b.*,(\$L\d+)\s*$' % re.escape(flag), out[i])
        if mb:
            del out[i]
            if mb.group(2) or (rgn and mb.group(3) != rgn):
                j = i
                while j < len(out) and (not out[j].strip() or out[j].strip().startswith('.')):
                    j += 1
                del out[j]
            return True
    return False


def substitute(lines):
    """-> (new lines, anomalies, sites). An anomaly (argument or marker not
    found, markers not alternating) means the model does not apply: the
    caller treats it as a build error."""
    lines = drop_tables(substitute_strings(lines))
    out, anomalies, sites = [], [], 0
    seq, fn, marked = [], None, False
    for l in lines:
        e = re.match(r'\s*\.(ent|end)\s+(\S+)', l)
        if e:
            if e.group(1) == 'end' and seq and (len(seq) % 2 or any(
                    k != ('begin', 'end')[i % 2] for i, k in enumerate(seq))):
                anomalies.append(f'{fn}: region boundaries not alternating {seq}')
            if marked:
                anomalies.append(f'{fn}: #DNASEND without a call to the shim')
            seq, fn, marked = [], e.group(2), False
        if '#DNASEND' in l:
            marked = True
        if '#DNASBEG' in l or '#DNASPRE' in l:
            continue
        j = JAL_RE.match(l)
        if not j:
            out.append(l)
            continue
        sites += 1
        shim = j.group(1)
        side = 'end' if marked else 'begin'
        regs = SHIMS[shim][side == 'end']
        load = re.compile(r'\s*lw\s+\$([5-7]),-?\d+\(\$%s\)\s*$' % DESC_REG[side])
        marked = False
        seq.append(side)
        if side == 'begin' and not remove_guard(out):
            anomalies.append(f'{fn}: site {sites} (begin): flag guard not found')
        # The arguments are removed on every path reaching the call: before the
        # first label, the first write of each register; past a label (reorg
        # filled a branch's slot from the target thread and duplicated the row
        # on the fall-through path), the copy on the fall-through path and the
        # one in the slot, which becomes nop.
        need, k = set(regs), len(out) - 1
        crossed, late, labels = 0, set(), set()
        allargs = set(need)
        while (need or late) and k >= 0 and not re.match(r'\s*(jal|\.ent)\b', out[k]):
            if LABEL_RE.match(out[k]):
                crossed += 1
                labels.add(out[k].split(':')[0].strip())
                k -= 1
                continue
            mo = re.match(r'\s*ori\s+\$([4-7]),\$([4-7]),0x[0-9a-f]+\s*$', out[k])
            if mo and mo.group(1) == mo.group(2) and mo.group(1) in need:
                del out[k]
                k -= 1
                continue
            m = ARG_RE.match(out[k]) or load.match(out[k])
            r = m and next((g for g in m.groups() if g), None)
            if r and (r in need or r in late):
                slot = in_noreorder(out, k)
                if not crossed or r in need or slot:
                    need.discard(r)
                    if crossed:
                        late.add(r)
                    if slot:
                        out[k] = '\tnop'
                    else:
                        del out[k]
            k -= 1
        if labels:
            i = len(out) - 1
            while i >= 0 and not re.match(r'\s*\.ent\b', out[i]):
                t = out[i].split()
                if t and (t[0].startswith('b') or t[0] == 'j') and t[-1].split(',')[-1] in labels:
                    for d in range(i + 1, min(i + 4, len(out))):
                        m = ARG_RE.match(out[d]) or load.match(out[d])
                        r = m and next((g for g in m.groups() if g), None)
                        if r and r in allargs and in_noreorder(out, d):
                            out[d] = '\tnop'
                            need.discard(r)
                            break
                        if out[d].strip() and not out[d].strip().startswith('.'):
                            break
                i -= 1
        if need:
            anomalies.append(f'{fn}: site {sites} ({side}): arguments not found {sorted(need)}')
        if side == 'begin':
            out += ['\t' + x for x in begin_text(shim)]
            continue
        q = len(out) - 1
        while q >= 0 and '#DNASEND' not in out[q] and not re.match(r'\s*\.ent\b', out[q]):
            q -= 1
        if q < 0 or '#DNASEND' not in out[q]:
            anomalies.append(f'{fn}: site {sites} (end): #DNASEND marker not found')
            continue
        out[q:q + 1] = ['\t' + x for x in end_text(shim) if not x.startswith('jal')]
        out += ['\t.set push', '\t.set reorder', f'\tjal {shim}', '\t.set pop']
    rest = [l for l in out if MARKERS.search(l)]
    if rest:
        anomalies.append(f'markers left after the substitution: {rest[:3]}')
    return out, anomalies, sites


def main():
    if len(sys.argv) != 3:
        sys.exit('usage: dnas_subst.py IN.s OUT.s')
    text = open(sys.argv[1]).read()
    if not has_markers(text):
        if sys.argv[1] != sys.argv[2]:
            open(sys.argv[2], 'w').write(text)
        return 0
    lines, anomalies, sites = substitute(text.splitlines())
    open(sys.argv[2], 'w').write('\n'.join(lines) + '\n')
    for a in anomalies:
        print(f'error: dnas_subst: {a}', file=sys.stderr)
    return 1 if anomalies else 0


if __name__ == '__main__':
    sys.exit(main())
