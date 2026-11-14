#!/usr/bin/env python3
"""Identity of the GOT symbols of the green functions.

The instruction comparison of tools/check.py masks the GOT offsets
(tools/normalize_asm.py: the linker picks the slot), so it does not see which
global symbol a function references through the GOT: a call to `free` instead
of `R_free`, to `g_mc` instead of `mcClientID`, an `int` parameter instead of
`bool` in the callee's mangling all give the same instructions.

For every green copy of a function (an STL dedup symbol has one copy per unit)
the ordered sequence of the names of the global symbols referenced through the
GOT is compared:

- original: every `lw rX,OFF(gp)` whose address gp+off falls in the global
  region of the GOT. The name comes from the position (MIPS o32: entry
  `local_gotno + k` is always dynsym `gotsym + k`, tools/got_callees.py), not
  from the value in the slot, which is 0 in the file for lazy imports.
  For the DNAS functions the instructions come from the decrypted image; the
  scaffold rows (tools/dnas_ref.py:cuts) are removed except the two calls to
  the shim, which count: our .o calls the real shim too
  (src/dnas/dnas_prot.h, tools/dnas_subst.py).
- ours: the GOT relocations of the .o (in this project only R_MIPS_CALL16 and
  R_MIPS_GOT16) to a non-local symbol, in address order. Relocations to local
  symbols (statics, sections, `$L` labels) go to the local GOT and have no
  name in the original: they are not counted on either side.

Second dimension, the data reached through the local GOT (section "data
through the local GOT" below, debt config/gotlocal_debt.txt): section class
and name.

Third dimension, LO16 targets (debt config/gotlo16_debt.txt). The first two do
not see which address a `lw rX,%got(page)(gp)` + LO16 pair reaches
(`addiu rX,rX,%lo(x)`, `lw/sw ..,%lo(x)(rX)`): a local-page call to
`UpdatePosition` instead of `CheckPosRange` of the same TU, `g_drawX` instead of
`g_drawY` (same class, no name in the original) give MATCH n/n. Here the target
of every local GOT + LO16 pair, t9 included, is resolved on both sides and the
identity is compared (lo16_judge):
- original: page read from the original's GOT + LO16 = absolute address.
- ours: R_MIPS_GOT16 to a section (or a local symbol) + AHL addend = section +
  offset in the .o. The pairs are aligned in order (the copy is green: same
  instructions).
- code (.text, .gnu.linkonce.t.*): the function of the .o containing the
  offset has an address in the original (universe name, or the gate's
  placement for a local: check.lo16_context) and address + internal offset
  must be the original one; a candidate by content of the local (identical
  body, same function) is accepted too.
- data: the linker lays out every input section of the .o whole, so for
  (unit, section) original address - our offset is one constant. The
  references of the unambiguous green copies of the unit vote; one with a
  difference other than the majority reaches the wrong object or the wrong
  offset inside the object.
- linkonce copies (template/inline dedup): the image holds the copy the linker
  kept, with references to the sections of its unit; a reference of a
  linkonce copy is valid if it works for its own unit or for the same pair of
  another copy of the function, and linkonce copies do not vote.
Distance = wrong references + difference in the number of pairs; fingerprint =
(index, original address, expected address), stable if the file grows
elsewhere. Same ratchet.
Limits of the third dimension: a data section with one reference only (or two
tied) is not judged (unverifiable, counted); if the majority is wrong (a hole
in the middle of a section) the minority copies are reported, which may not be
the ones to fix (the diagnosis prints the votes); a target in a local "out of
alignment" or without an address is not verified. Outside all three: the
pointers inside .data tables (R_MIPS_32) and the jump table entries; static vs
global linkage of a datum with the same name.

"TU boundary" class (diagnosis `[TU]`): gas never emits CALL16 to a strong
(non-weak) global defined in the same .o, but the local-page form. If the
original calls X with CALL16 and our .o defines X, the name is not wrong: X is
in the wrong TU (in the original it is in another TU), and the fix is to move
the body, not to rename. The converse (we CALL16 to X, the original does not)
says that in the original X is in the same TU (diagnosis `[TU?]`). In both
cases the .o size differs from the original by 4 B per call site, because
normalize_asm collapses the two forms.

Ratchet: the current debt is in config/got_debt.txt (GENERATED), one row per
copy `SYMBOL@UNIT` with the distance (symmetric difference of the name
multisets; 1 if only the order changes) and the fingerprint of the two
sequences. The gate fails for: a green copy with a different identity
outside the debt; a debt copy whose distance grows, or whose fingerprint
changes at equal distance. A decreasing distance is an improvement.
`--write-debt` by default only removes rows (clean or vanished copies) and
updates the improved ones; adding rows needs `--add` and prints them.

CLI:
    python3 tools/gotident.py --write-debt [--add]    # regenerate config/got_debt.txt
    python3 tools/gotident.py SYM [SYM...] [--unit U]  # both sides, from the .o of the last gate
    python3 tools/gotident.py --map UNIT               # positional map ours -> orig of the
                                                       # dirty copies (mechanical renames)
    python3 tools/gotident.py --obj OUT.o [--local NAME] SYM   # for tools/match: exit 1
                                                       # if different and outside the debt
"""
import bisect
import glob
import hashlib
import json
import os
import re
import subprocess
import sys
from collections import Counter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import got_callees as gc  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEBT = 'config/got_debt.txt'
LDEBT = 'config/gotlocal_debt.txt'
TDEBT = 'config/gotlo16_debt.txt'
COPIES = 'build/got_copies.json'
OBJDUMP = 'mipsel-linux-gnu-objdump'
GOT_RTYPES = ('R_MIPS_CALL16', 'R_MIPS_GOT16')

RELOC_RE = re.compile(r'^\s*([0-9a-f]+): (R_MIPS_\w+)\s+(\S+)')
SECTION_RE = re.compile(r'^Disassembly of section (\S+):\s*$')
ADDEND_RE = re.compile(r'[+-]0x[0-9a-f]+$')
FUNC_RE = re.compile(r'^([0-9a-f]+) [ ]?(?:g|w|l)\s+F (\S+)\s+([0-9a-f]+)\s+(\S+)$')  # = check.OBJ_SYM_RE

# {unit: set(global symbols defined in its .o)}: filled by check.do_unit
# (needed by the diagnosis of the "TU boundary" class)
UNIT_DEFS = {}


# --------------------------------------------------------------------------- #
# Our side
# --------------------------------------------------------------------------- #
def symtab_info(symtab_text):
    """`objdump -t` of a .o -> (locals, strong globals defined).

    Row format: `ADDR FLAGS(7) SECTION\\tSIZE NAME`; the first flag is the
    visibility (`l` local, `g` global, `w` weak, ` ` none)."""
    loc, defs = set(), set()
    for line in symtab_text.splitlines():
        if len(line) < 18 or not re.match(r'^[0-9a-f]{8} ', line):
            continue
        rest = line[17:].split()
        if len(rest) < 3:
            continue
        name, sec = rest[-1], rest[0]
        if line[9] == 'l':
            loc.add(name)
        elif line[9] == 'g' and sec != '*UND*':
            # Strong globals only: to a weak defined in the same .o (STL
            # linkonce) gas does emit CALL16 (e.g. find_first_of in
            # thxmlloader.cpp), so it is not the [TU] class
            defs.add(name)
    return loc, defs


def ours_by_func(dis_text, objsyms, locals_set):
    """`objdump -z -dr` of a whole .o -> {function: [global GOT names in order]}.

    objsyms = {name: (addr, size, section)} (check.defined_functions). The
    relocations are attributed to the function by (section, address)."""
    starts = {}
    for name, (addr, size, sec) in objsyms.items():
        if size:
            starts.setdefault(sec, []).append((addr, addr + size, name))
    for sec in starts:
        starts[sec].sort()
    keys = {sec: [a for a, _, _ in v] for sec, v in starts.items()}
    out = {name: [] for name, (_, size, _) in objsyms.items() if size}
    sec = None
    for line in dis_text.splitlines():
        m = SECTION_RE.match(line)
        if m:
            sec = m.group(1)
            continue
        m = RELOC_RE.match(line)
        if not m or sec not in starts:
            continue
        addr, rtype, sym = int(m.group(1), 16), m.group(2), m.group(3)
        if rtype not in GOT_RTYPES:
            continue
        sym = ADDEND_RE.sub('', sym)
        if sym.startswith(('.', '$')) or sym in locals_set:
            continue
        i = bisect.bisect_right(keys[sec], addr) - 1
        if i < 0:
            continue
        a, e, name = starts[sec][i]
        if a <= addr < e:
            out[name].append(sym)
    return out


# --------------------------------------------------------------------------- #
# Data through the local GOT (second dimension)
# --------------------------------------------------------------------------- #
# A `lw rX,OFF(gp)` to the local region of the GOT gives the page; the real object is
# page + the LO16 of the first instruction using rX as a base (addiu/lw/sw/...). The
# instruction comparison masks both: here the object reached on the two sides is
# compared, in the order they appear -- the section class (.text = a function
# pointer, .rodata, .data, .bss) and, if the original's object has a name in
# config/symbol_addrs.txt, the name. It catches e.g. swapped objects (the
# thPosixThread lists) or a buffer invented in place of a literal.
_MEMOPS = {0x09, 0x20, 0x21, 0x23, 0x24, 0x25, 0x28, 0x29, 0x2b, 0x31, 0x39, 0x37, 0x3f,
           0x1e, 0x1f, 0x22, 0x26}


def _sext(x):
    return x - 0x10000 if x >= 0x8000 else x


def _local_pairs(words, with_t9=False):
    """[(k, hi, lo)]: for every `lw rX,imm(gp)` (rX not t9/gp/zero; t9 included with
    with_t9, the local-page calls `lw t9` + `addiu t9,t9,%lo`) the LO16 of the first
    instruction using rX as a base in the next 16 (lo=None if rX is rewritten
    before, or if there is no use)."""
    out = []
    excl = (28, 0) if with_t9 else (25, 28, 0)
    for k, w in enumerate(words):
        if (w >> 26) != 0x23 or ((w >> 21) & 31) != 28:
            continue
        rx = (w >> 16) & 31
        if rx in excl:
            continue
        lo = None
        for j in range(k + 1, min(k + 16, len(words))):
            w2 = words[j]
            op = w2 >> 26
            if op in _MEMOPS and ((w2 >> 21) & 31) == rx:
                lo = _sext(w2 & 0xffff)
                break
            if op == 0 and ((w2 >> 11) & 31) == rx:
                break
            if op != 0 and op not in (0x04, 0x05, 0x2b, 0x28, 0x29, 0x3f, 0x1f) \
                    and ((w2 >> 16) & 31) == rx:
                break
        out.append((k, _sext(w & 0xffff), lo))
    return out


def _secclass(name):
    n = name or '?'
    for pre, cls in (('.text', '.text'), ('.rodata', '.rodata'), ('.sdata', '.data'),
                     ('.data', '.data'), ('.sbss', '.bss'), ('.bss', '.bss'),
                     ('.gnu.linkonce.t', '.text'), ('.gnu.linkonce.r', '.rodata'),
                     ('.gnu.linkonce.d', '.data'), ('.gnu.linkonce.b', '.bss')):
        if n.startswith(pre):
            return cls
    return n


def local_data_by_func(dis_text, objsyms, symtab_text):
    """`objdump -z -dr` + `objdump -t` of a .o -> {function: [(class, name|None)]} of the
    objects reached through the local GOT, in order. A name is that of the .o symbol
    containing the offset (static or global), None for an area without a symbol (literals)."""
    return local_refs_by_func(dis_text, objsyms, symtab_text)[0]


def local_refs_by_func(dis_text, objsyms, symtab_text):
    """One pass on the dump for the second and the third dimension -> (data, targets):
    data = {function: [(class, name|None)]} (local_data_by_func, without the t9 pairs);
    targets = {function: [(section, offset)]} of all the local GOT16 + LO16 pairs,
    those on t9 (local-page calls) included, in order (third dimension, LO16
    targets: see below)."""
    # data/function symbols of the .o by section
    bysec = {}
    for line in symtab_text.splitlines():
        if len(line) < 18 or not re.match(r'^[0-9a-f]{8} ', line):
            continue
        rest = line[17:].split()
        if len(rest) < 3 or rest[-1].startswith(('.', '$')):
            continue
        sec, size, name = rest[0], rest[-2], rest[-1]
        try:
            bysec.setdefault(sec, []).append((int(line[:8], 16), int(size, 16), name))
        except ValueError:
            continue
    symsec = {n: (s, v) for s, L in bysec.items() for v, sz, n in L}
    # only local symbols go to the local GOT: a GOT16 to a global or an
    # undefined symbol is a global entry, already judged by the first dimension
    loc_syms = symtab_info(symtab_text)[0]

    def ourname(sec, off):
        # size-0 symbols (`gcc2_compiled.`, `__gnu_compiled_c` at .text+0 in every .o)
        # are not objects: without this filter the pointer to the file's first
        # function would take their name
        best = None
        for v, sz, n in bysec.get(sec, ()):
            if not sz:
                continue
            if v == off:
                return n
            if v < off < v + sz:
                best = n
        return best

    words, rels, sec = {}, {}, None
    for line in dis_text.splitlines():
        m = SECTION_RE.match(line)
        if m:
            sec = m.group(1)
            continue
        if sec is None:
            continue
        m = re.match(r'^\s*([0-9a-f]+):\t([0-9a-f]{8}) ', line)
        if m:
            words.setdefault(sec, {})[int(m.group(1), 16)] = int(m.group(2), 16)
            continue
        m = RELOC_RE.match(line)
        if m:
            rels.setdefault(sec, {})[int(m.group(1), 16)] = (m.group(2), ADDEND_RE.sub('', m.group(3)))
    out, tgt = {}, {}
    for fn, (addr, size, fsec) in objsyms.items():
        if not size or fsec not in words:
            continue
        W = [words[fsec].get(addr + 4 * i, 0) for i in range(size // 4)]
        R = rels.get(fsec, {})
        seq, tseq = [], []
        for k, hi, lo in _local_pairs(W, with_t9=True):
            rel = R.get(addr + 4 * k)
            if not rel or rel[0] != 'R_MIPS_GOT16' or lo is None:
                continue
            target = rel[1]
            t9 = ((W[k] >> 16) & 31) == 25
            if target.startswith('.'):
                off = (hi << 16) + lo
                if not t9:
                    seq.append((_secclass(target), ourname(target, off)))
                tseq.append((target, off))
            elif target in loc_syms and target in symsec:
                if not t9:
                    seq.append((_secclass(symsec[target][0]), target))
                tseq.append((symsec[target][0], symsec[target][1] + (hi << 16) + lo))
        out[fn] = seq
        tgt[fn] = tseq
    return out, tgt


def compare_local(orig, ours):
    """-> (equal, distance, fingerprint, diagnosis) of the [(class, name)] sequences.
    The section class is always judged, the name only when the original's object has
    one (for an original function the name counts only if it is not a static_<ADDR>)."""
    bad, nerr = [], 0
    for i, (o, u) in enumerate(zip(orig, ours)):
        (oc, on), (uc, un) = o, u
        # section and name are counted separately: fixing only the section (e.g. the
        # explicit `= 0` that sends the object to .data) must lower the distance, not
        # leave it equal with a different fingerprint
        sec_bad = oc != uc
        name_bad = bool(on) and un != on and not on.startswith('static_')
        if sec_bad:
            bad.append(('FUNCTION POINTER ' if oc == '.text' else '') +
                       f'SECTION {oc} ({on or "unnamed"}) -> {uc} ({un or "unnamed"})'
                       + (' + NAME' if name_bad else ''))
        elif name_bad:
            bad.append(f'NAME {on} -> {un or "unnamed"}')
        nerr += sec_bad + name_bad
    dist = nerr + abs(len(orig) - len(ours))
    if not dist:
        return True, 0, '', ''
    # the fingerprint holds only what is judged: the section class of both sides and
    # our name only where the original's object has one (renaming one of our statics
    # that is anonymous in the original is not a regression)
    fp = hashlib.sha1(repr([(oc, on, uc, un if on else None)
                            for (oc, on), (uc, un) in zip(orig, ours)]
                           + [len(orig), len(ours)]).encode()).hexdigest()[:8]
    diag = '; '.join(bad[:3]) + (f' (+{len(bad) - 3})' if len(bad) > 3 else '')
    if len(orig) != len(ours):
        diag = (diag + ' | ' if diag else '') + f'local references {len(orig)} orig / {len(ours)} ours'
    return False, dist, fp, diag


# --------------------------------------------------------------------------- #
# LO16 targets (third dimension)
# --------------------------------------------------------------------------- #
# {unit: {local or global function of the .o: (offset, size, section)}} (check.do_unit)
UNIT_FUNCS = {}
# {unit: {function of the .o: original address}}: the universe names and the locals
# that assign_statics placed (states ok/fail/skip; never "misaligned", not an identity)
UNIT_ADDR = {}
# {unit: {local function: set(addresses)}}: the candidates by content of the local
# (identical body): a call to any of them is the same function
UNIT_HITS = {}
# {unit: {function of the .o: LO16 targets}} of all the functions of the .o (linkonce copies)
UNIT_TGT = {}


_FIDX = {}


def _func_at(funcs, sec, off):
    """(name, start) of the function of the .o containing sec+off, or (None, None)."""
    k = (id(funcs), sec)
    if k not in _FIDX:
        L = sorted((a, a + sz, n) for n, (a, sz, s) in funcs.items() if s == sec and sz)
        _FIDX[k] = (funcs, L, [x[0] for x in L])
    _, L, starts = _FIDX[k]
    i = bisect.bisect_right(starts, off) - 1
    while i >= 0:
        a, e, n = L[i]
        if a <= off < e:
            return n, a
        if a < off - 0x100000:
            break
        i -= 1
    return None, None


def _claimed(AD, fn):
    """Original addresses placed at the other functions of the .o (cached per dict)."""
    k = id(AD)
    if _CLAIM.get(k, (None,))[0] is not AD:
        _CLAIM[k] = (AD, set(AD.values()))
    own = AD.get(fn)
    return _CLAIM[k][1] - ({own} if own is not None else set())


_CLAIM = {}


def _is_lk(sec):
    return sec.startswith('.gnu.linkonce.')


def lo16_judge(entries, orig, funcs=None, addrs=None, hits=None, tgts=None, lk_alone=False,
               full_run=False):
    """Judge the third dimension. entries = [(key, sym, unit, lname, targets, amb,
    (addr, size))] with targets = [(.o section, offset)] (local_refs_by_func) -> {key:
    rec} with rec = {key, ok, dist, fp, diag, unverifiable} (count of references that cannot be verified).

    For every copy the pairs are aligned in order with those of the original
    (Orig.local_addrs: same extractor on the same instructions, because the copy is
    green). Then:
    - target in a code section (.text, .gnu.linkonce.t.*): the function of the .o
      containing the offset, with its original address (universe name, or the gate's
      placement for a local) + the offset inside the function, must be the original
      address; a candidate by content of the local (identical body) is accepted too.
      A function without an address (local "out of alignment") = unverifiable.
    - target in a data section: the linker lays out every input section of a .o
      whole, so for (unit, section) the difference original address - our offset is
      one. The references of the unit's green copies that are not ambiguous and not
      in a linkonce section (see below) vote; a reference with a difference other
      than the majority one is a wrong target (object, or offset inside the object).
      With two differences tied, nothing is decided: unverifiable.
    - linkonce copies (function in .gnu.linkonce.t.*, template/inline dedup): the
      image holds one copy, the first in link order, and its references go to the
      sections of its unit (without this rule discarded copies pointing to their own
      unit's .rodata, e.g. `_S_oom_malloc`, `__tf13thPosixThread`, would be false
      positives). The gate does not know which copy was kept: a reference of a
      linkonce copy is good if it is good for its own unit or for the copy of the
      same function (same name, same pair index) in another unit defining it
      (tgts = {unit: {function: targets}}, copies in skip included). Linkonce copies
      do not vote. If the run (scoped gate, tools/match with lk_alone) has no other
      copy to look at, the reference stays unverifiable: the full run decides."""
    funcs = UNIT_FUNCS if funcs is None else funcs
    addrs = UNIT_ADDR if addrs is None else addrs
    hits = UNIT_HITS if hits is None else hits
    tgts = UNIT_TGT if tgts is None else tgts
    al, votes = {}, {}
    lk_units = {}
    for u, F in funcs.items():
        for n, (_, sz, s) in F.items():
            if sz and _is_lk(s):
                lk_units.setdefault(n, []).append(u)
    for key, sym, unit, lname, tg, amb, (a, sz) in entries:
        A = orig.local_addrs(sym, a, sz)
        al[key] = A
        if amb or _is_lk(funcs.get(unit, {}).get(lname, (0, 0, ''))[2]):
            continue
        for (sec, off), x in zip(tg, A):
            if _secclass(sec) != '.text':
                votes.setdefault((unit, sec), Counter())[(x - off) & 0xffffffff] += 1

    def check(unit, sec, off, x):
        """-> ('ok'|'unverifiable'|'bad', expected, description)"""
        if _secclass(sec) == '.text':
            F, AD, H = funcs.get(unit, {}), addrs.get(unit, {}), hits.get(unit, {})
            fn, st = _func_at(F, sec, off)
            e = AD.get(fn) if fn else None
            if e is None:
                return 'unverifiable', None, 'code without an address' if fn else 'code outside any function'
            exp = e + off - st
            # a candidate by content is valid only if no other function of this .o is
            # placed there: the content is compared with relocations MASKED, so two
            # "identical" bodies may be two different functions (e.g. BaseName/Extension
            # of natives_play.cpp)
            if exp == x or ((x - (off - st)) in H.get(fn, ())
                            and (x - (off - st)) not in _claimed(AD, fn)):
                return 'ok', exp, ''
            return 'bad', exp, (f'CODE {fn}' + (f'+{off - st}' if off != st else '')
                                + f' (orig {orig.where(exp)}) where the original has {orig.where(x)}')
        c = votes.get((unit, sec))
        if not c:
            return 'unverifiable', None, 'data section without votes'
        if sum(c.values()) == 1:
            # the only reference of the section voted for itself: no check
            return 'unverifiable', None, 'section with a single reference'
        top = c.most_common(2)
        d0, n0 = top[0]
        if len(top) > 1 and top[1][1] == n0:
            return 'unverifiable', None, 'tied placement votes'
        if (x - off) & 0xffffffff == d0:
            return 'ok', x, ''
        exp = (off + d0) & 0xffffffff
        return 'bad', exp, (f'DATA {sec}+{off:#x} -> orig {orig.where(exp)} where the original has '
                            f'{orig.where(x)} ({x - exp:+d} B; placement of {sec} {n0}/{sum(c.values())} votes)')

    out = {}
    for key, sym, unit, lname, tg, amb, _ in entries:
        A = al[key]
        bad, fpi, unverifiable, why = [], [], 0, {}
        lk = _is_lk(funcs.get(unit, {}).get(lname, (0, 0, ''))[2])
        for i, ((sec, off), x) in enumerate(zip(tg, A)):
            st, exp, desc = check(unit, sec, off, x)
            if st == 'bad' and lk and lk_alone:
                st, desc = 'unverifiable', 'linkonce, other copies not seen'
            if st == 'bad' and lk:
                # another copy of the same linkonce function may be the kept one;
                # if this (scoped) run has no other copy, nothing is decided
                seen = 0
                for u in lk_units.get(lname, ()):
                    t = tgts.get(u, {}).get(lname)
                    if u == unit or not t or len(t) <= i:
                        continue
                    seen += 1
                    s2 = check(u, t[i][0], t[i][1], x)[0]
                    if s2 == 'ok':
                        st = 'ok'
                        break
                    if s2 == 'unverifiable':
                        st, desc = 'unverifiable', 'linkonce, copy in another unit not verifiable'
                if not seen and not full_run:
                    st, desc = 'unverifiable', 'linkonce, other copies not seen'
                # in the full run no other unit defines the section: this is the only
                # copy, hence the one in the image, and the 'bad' stands
            if st == 'unverifiable':
                unverifiable += 1
                why[desc] = why.get(desc, 0) + 1
            elif st == 'bad':
                bad.append(f'#{i} {desc}')
                fpi.append(('t' if _secclass(sec) == '.text' else 'd', i, x, exp))
        dist = len(bad) + abs(len(A) - len(tg))
        rec = {'key': key, 'ok': not dist, 'dist': dist, 'fp': '', 'diag': '', 'unverifiable': unverifiable}
        if why:
            rec['unverifiable_why'] = why
        if dist:
            rec['fp'] = hashlib.sha1(repr(fpi + [len(A), len(tg)]).encode()).hexdigest()[:8]
            diag = '; '.join(bad[:3]) + (f' (+{len(bad) - 3})' if len(bad) > 3 else '')
            if len(A) != len(tg):
                diag = (diag + ' | ' if diag else '') + \
                    f'local GOT+LO16 pairs {len(A)} orig / {len(tg)} ours'
            rec['diag'] = diag
        out[key] = rec
    return out


# --------------------------------------------------------------------------- #
# Original side
# --------------------------------------------------------------------------- #
class Orig:
    """Reader of the original ELF (and of the decrypted image, for the DNAS functions)."""

    def __init__(self, protected=()):
        path = os.path.join(ROOT, gc.ELF)
        self.data, secs = gc.load_elf(path)
        self.a2o = gc.make_addr_to_off(secs)
        self.resolve = gc.make_gotsym_resolver(self.data, secs)
        if self.resolve is None:
            raise SystemExit('error: gotident: GOT/dynsym layout not recognized')
        self.protected = set(protected)
        self._img = None
        self._skip = set()
        self._desc = set()
        self.bind = _dynsym_bindings(self.data, secs)

    def _image(self):
        if self._img is None:
            import dnas_ref
            img = dnas_ref.ensure_image()
            data, secs = gc.load_elf(img)
            self._img = (data, gc.make_addr_to_off(secs))
            cut, self._desc = _dnas_cut(img)
            self._skip = cut
        return self._img

    def local_callees(self, sym, addr, size):
        """Names (from the universe, statics included) of the functions the original
        calls at local page (`lw t9,OFF(gp)` + `addiu t9,t9,D` to a known address): the
        callees of the same TU. For the DNAS functions they are read from the decrypted image."""
        if not hasattr(self, '_byaddr'):
            import universe
            self._byaddr = {a: s for s, (a, sz) in universe.symbols(
                os.path.join(ROOT, 'config/symbol_addrs.txt')).items() if sz}
        data, a2o = self._image() if sym in self.protected else (self.data, self.a2o)
        off = a2o(addr)
        out = []
        if off is None:
            return out
        for k in range(size // 4 - 1):
            w = int.from_bytes(data[off + 4 * k:off + 4 * k + 4], 'little')
            if (w >> 26) != 0x23 or ((w >> 21) & 31) != 28 or ((w >> 16) & 31) != 25:
                continue
            imm = w & 0xffff
            imm -= 0x10000 if imm >= 0x8000 else 0
            if self.resolve(gc.GP + imm):
                continue
            w2 = int.from_bytes(data[off + 4 * k + 4:off + 4 * k + 8], 'little')
            if (w2 >> 26) == 0x09 and ((w2 >> 21) & 31) == 25 and ((w2 >> 16) & 31) == 25:
                d = w2 & 0xffff
                d -= 0x10000 if d >= 0x8000 else 0
                go = self.a2o(gc.GP + imm)
                if go is None:
                    continue
                tgt = int.from_bytes(self.data[go:go + 4], 'little') + d
                if tgt in self._byaddr:
                    out.append(self._byaddr[tgt])
        return out

    def _data_index(self):
        if not hasattr(self, '_dsyms'):
            secs = gc.load_elf(os.path.join(ROOT, gc.ELF))[1]
            self._osecs = sorted((s['addr'], s['addr'] + s['size'], s['nm']) for s in secs
                                 if s.get('addr'))
            _, info = gc.load_symbols(os.path.join(ROOT, gc.SYMBOL_ADDRS))
            self._dsyms = sorted((a, sz, n) for n, (a, sz) in info.items() if sz)
            self._dstarts = [x[0] for x in self._dsyms]

    def _osec(self, a):
        for lo, hi, n in self._osecs:
            if lo <= a < hi:
                return n
        return '?'

    def _oname(self, a):
        i = bisect.bisect_right(self._dstarts, a) - 1
        while i >= 0:
            s, sz, n = self._dsyms[i]
            if s <= a < s + sz:
                return n
            if a - s > 0x100000:
                break
            i -= 1
        return None

    def local_data(self, sym, addr, size):
        """[(class, name|None)] of the objects the original reaches through the local GOT,
        in order. For the DNAS functions: decrypted image, scaffold included (our .o has
        it, with its `la` to .text); only the descriptor words are skipped, which are
        data and may happen to look like a `lw rX,OFF(gp)`."""
        self._data_index()
        if sym in self.protected:
            data, a2o = self._image()
            skip = self._desc
        else:
            data, a2o, skip = self.data, self.a2o, ()
        off = a2o(addr)
        if off is None:
            return []
        W = [int.from_bytes(data[off + 4 * i:off + 4 * i + 4], 'little') for i in range(size // 4)]
        out = []
        for k, hi, lo in _local_pairs(W):
            if lo is None or addr + 4 * k in skip or self.resolve(gc.GP + hi):
                continue
            go = self.a2o(gc.GP + hi)
            if go is None:
                continue
            fa = int.from_bytes(self.data[go:go + 4], 'little') + lo
            out.append((_secclass(self._osec(fa)), self._oname(fa)))
        return out

    def local_addrs(self, sym, addr, size):
        """[absolute address] of the targets of the original's local GOT + LO16 pairs, t9
        included (third dimension), in order: page read from the original's GOT + LO16.
        For the DNAS functions like local_data (decrypted image, descriptors skipped)."""
        if sym in self.protected:
            data, a2o = self._image()
            skip = self._desc
        else:
            data, a2o, skip = self.data, self.a2o, ()
        off = a2o(addr)
        if off is None:
            return []
        W = [int.from_bytes(data[off + 4 * i:off + 4 * i + 4], 'little') for i in range(size // 4)]
        out = []
        for k, hi, lo in _local_pairs(W, with_t9=True):
            if lo is None or addr + 4 * k in skip or self.resolve(gc.GP + hi):
                continue
            go = self.a2o(gc.GP + hi)
            if go is None:
                continue
            out.append((int.from_bytes(self.data[go:go + 4], 'little') + lo) & 0xffffffff)
        return out

    def where(self, a):
        """'name+off' (config/symbol_addrs.txt, static_<ADDR> included) or 'section:0xADDR'."""
        self._data_index()
        if not hasattr(self, '_fidx'):
            import universe
            self._fidx = sorted((v[0], v[1], s) for s, v in universe.symbols(
                os.path.join(ROOT, 'config/symbol_addrs.txt')).items() if v[1])
            self._fstarts = [x[0] for x in self._fidx]
        i = bisect.bisect_right(self._fstarts, a) - 1
        if i >= 0 and self._fidx[i][0] <= a < self._fidx[i][0] + self._fidx[i][1]:
            d = a - self._fidx[i][0]
            return self._fidx[i][2] + (f'+{d}' if d else '')
        n = self._oname(a)
        if n:
            i = bisect.bisect_right(self._dstarts, a) - 1
            while i >= 0 and self._dsyms[i][2] != n:
                i -= 1
            d = a - self._dsyms[i][0] if i >= 0 else 0
            return n + (f'+{d}' if d else '')
        return f'{self._osec(a)}:{a:#x}'

    def names(self, sym, addr, size):
        if sym in self.protected:
            data, a2o = self._image()
            skip = self._skip
        else:
            data, a2o, skip = self.data, self.a2o, ()
        off = a2o(addr)
        if off is None or not size:
            return []
        out = []
        for k in range(size // 4):
            w = int.from_bytes(data[off + 4 * k:off + 4 * k + 4], 'little')
            # lw rt, imm(gp)
            if (w >> 26) & 0x3f != 0x23 or (w >> 21) & 0x1f != 28:
                continue
            if addr + 4 * k in skip:
                continue
            imm = w & 0xffff
            if imm >= 0x8000:
                imm -= 0x10000
            hit = self.resolve(gc.GP + imm)
            if hit:
                out.append(hit[0])
        return out


def _dynsym_bindings(data, secs):
    """{dynsym name: 'GLOBAL' | 'WEAK' | 'LOCAL' | 'UND'} of the original (for the
    diagnosis subclass: a weak callee is a linkonce copy of a template/inline, one
    absent from .dynsym is a static)."""
    S = {s['nm']: s for s in secs}
    ds, dstr = S.get('.dynsym'), S.get('.dynstr')
    out = {}
    if not (ds and dstr):
        return out
    for i in range(ds['size'] // 16):
        o = ds['offset'] + i * 16
        st_name = int.from_bytes(data[o:o + 4], 'little')
        info = data[o + 12]
        e = data.index(b'\0', dstr['offset'] + st_name)
        name = data[dstr['offset'] + st_name:e].decode()
        shndx = int.from_bytes(data[o + 14:o + 16], 'little')
        if name:
            # UND = import not defined in the binary (libc, ld.so): it cannot be in any TU
            out[name] = 'UND' if shndx == 0 else {0: 'LOCAL', 1: 'GLOBAL', 2: 'WEAK'}.get(info >> 4, '?')
    return out


def _dnas_cut(img):
    """(addresses of the DNAS scaffold, addresses of the descriptor words)
    (tools/dnas_ref.py:cuts and desc_addrs), cached by hash of the image and of the
    module: computing them costs an objdump of the image."""
    import dnas_ref
    import normalize_asm
    h = hashlib.sha1(open(img, 'rb').read()).hexdigest()[:10] + \
        hashlib.sha1(open(dnas_ref.__file__, 'rb').read()).hexdigest()[:6]
    path = os.path.join(ROOT, f'build/cache/dnas_cut2-{h}.json')
    if os.path.exists(path):
        d = json.load(open(path))
        return set(d['cut']), set(d['desc'])
    dis = subprocess.run([OBJDUMP, '-z', '-d', img], capture_output=True, text=True).stdout
    rows = normalize_asm.parse_rows(dis.splitlines())
    cut, _ = dnas_ref.cuts(rows)
    desc = dnas_ref.desc_addrs(rows)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    json.dump({'cut': sorted(cut), 'desc': sorted(desc)}, open(path, 'w'))
    return set(cut), set(desc)


# --------------------------------------------------------------------------- #
# comparison and ratchet
# --------------------------------------------------------------------------- #
def compare(orig, ours, protected, unit_defs=(), bind=None, local=None):
    """-> (equal, distance, fingerprint, diagnosis).

    bind = {name: binding} of the original's .dynsym (Orig.bind): if given, the
    diagnosis carries the subclass with its fix (fingerprint and distance do not
    depend on it):
      `[TU]`        the callee is a strong global of another TU in the original: move the body;
      `[TU-weak]`   the callee is weak in the original (linkonce copy of a template or of a
                    non-expanded inline): make it a real template/inline, do not move it;
      `[TU?]`       we CALL16 to X and the original calls X at local page (checked with
                    `local`, the original's local callees): X is in the same TU (merge or
                    move);
      `[TU?-static]` X is not in the original's .dynsym and no "original only" names are
                    left: it is a static of this TU (our name is invented), write it static here;
      otherwise (X imported/UND, or a global the original does not call, or next to
      "original only" names) the real callee is another one: the diagnosis shows the
      original's local callees and the fix is to rename."""
    if orig == ours:
        return True, 0, '', ''
    co, cu = Counter(orig), Counter(ours)
    dist = sum(((co - cu) + (cu - co)).values()) or 1
    fp = hashlib.sha1(('\0'.join(orig) + '\1' + '\0'.join(ours)).encode()).hexdigest()[:8]
    so, su = set(orig), set(ours)
    tu = sorted(x for x in so - su if x in unit_defs)
    parts = []
    if tu:
        weak = [x for x in tu if bind and bind.get(x) == 'WEAK']
        strong = [x for x in tu if x not in weak]
        if strong:
            parts.append('[TU] defined in this unit, in another TU in the original (CALL16): '
                         + ', '.join(strong[:3]) + ' -- move the body, do not rename')
        if weak:
            parts.append('[TU-weak] WEAK copies in the original (template/inline linkonce): '
                         + ', '.join(weak[:3]) + ' -- make them real templates/inlines')
    if len(orig) == len(ours) and so != su and not tu:
        pairs = [(a, b) for a, b in zip(orig, ours) if a != b]
        parts.append('different names: ' + ', '.join(f'{a} -> {b}' for a, b in pairs[:3])
                     + (f' (+{len(pairs) - 3})' if len(pairs) > 3 else ''))
    else:
        rest = sorted(so - su - set(tu))
        if rest:
            parts.append('only orig: ' + ', '.join(rest[:3]))
        if su - so:
            extra = sorted(su - so)
            if bind is not None:
                loc = set(local or ())
                verified = [x for x in extra if x in loc and bind.get(x) not in ('UND', 'WEAK')]
                stat = [x for x in extra if x not in bind and x not in verified and not rest]
                other = [x for x in extra if x not in verified and x not in stat]
                if verified:
                    parts.append('only ours: ' + ', '.join(verified[:3])
                                 + ' [TU? the original calls it at a local page: same TU, '
                                 'merge or move]')
                if stat:
                    parts.append('only ours: ' + ', '.join(stat[:3])
                                 + ' [TU?-static: not in the .dynsym of the original, a static '
                                 'of this TU: write it static here]')
                if other:
                    lc = sorted(set(local or ()))
                    parts.append('only ours: ' + ', '.join(other[:3])
                                 + ' [DIFFERENT callee: rename; local calls of the original: '
                                 + (', '.join(lc[:4]) or 'none') + ']')
            else:
                parts.append('only ours: ' + ', '.join(extra[:3])
                             + ' [TU? if the name is right, it is in the same TU in the original]')
        if not parts:
            parts.append(f'same names, different order or multiplicity ({len(orig)}/{len(ours)})')
    return False, dist, fp, ' | '.join(parts)


def load_debt(path=DEBT):
    """{SYMBOL@UNIT: (distance, fingerprint)}."""
    out = {}
    p = os.path.join(ROOT, path)
    if not os.path.exists(p):
        return out
    for line in open(p):
        body, _, comment = line.partition('#')
        key = body.strip()
        if not key:
            continue
        m = re.search(r'dist=(\d+) fp=([0-9a-f]*)', comment)
        out[key] = (int(m.group(1)), m.group(2)) if m else (10 ** 9, '')
    return out


def verdict(key, dist, fp, debt, present_keys, used=None):
    """Verdict of a dirty copy against the debt -> (outcome, debt_row).

    outcome: 'debt' (same row, unchanged), 'improved' (distance decreased),
    'worse' (distance increased, or fingerprint changed at equal distance),
    'moved' (the row belongs to another unit of the same symbol that is no longer
    in this run: a body moved between units -- the very fix of the [TU] class --
    carries its row with the same rule as in place: equal fingerprint, or strictly
    lower distance; the fingerprint does not depend on the unit), 'new' (outside
    the debt). present_keys = the keys of the copies present in this run; used = rows
    already inherited in this run (a row is inherited once only). In a scoped gate
    present_keys holds only the units of the run, so inheritance is more tolerant
    than in the full run (a new copy could inherit the row of a unit still present in
    the full build): the full run's judgement is the one that counts."""
    if key in debt:
        d0, fp0 = debt[key]
        if dist > d0 or (dist == d0 and fp0 and fp != fp0):
            return 'worse', key
        return ('improved' if dist < d0 else 'debt'), key
    sym = key.split('@', 1)[0]
    for k, (d0, fp0) in sorted(debt.items()):
        if k.split('@', 1)[0] == sym and k != key and k not in present_keys \
                and (used is None or k not in used):
            if fp == fp0 or dist < d0:
                if used is not None:
                    used.add(k)
                return 'moved', k
    return 'new', None


def _ratchet(recs, debt, present, scoped):
    """Ratchet of one dimension. recs = [(rec, ident)] with rec = {key, ok, dist, fp,
    diag}. Returns the dict of verdicts (see judge)."""
    R = {k: [] for k in ('new', 'worse', 'improved', 'moved', 'clean', 'warnings')}
    n_ok = n_debt = n_amb = 0
    used = set()
    for rec, ident in recs:
        key, same, dist, fp, diag = rec['key'], rec['ok'], rec['dist'], rec['fp'], rec['diag']
        if same:
            n_ok += 1
            if key in debt:
                R['clean'].append(key)
            continue
        ident = ident or ''
        if 'candidates' in ident:
            n_amb += 1
            rec['amb'] = True        # measured, not judged: it does not enter the debt
            continue
        outcome, row = verdict(key, dist, fp, debt, present, used)
        if outcome == 'moved':
            rec['from'] = row
        if scoped and ident.startswith('content') and outcome in ('new', 'worse'):
            R['warnings'].append((key, diag))
            continue
        if outcome == 'new':
            R['new'].append((key, diag))
            continue
        n_debt += 1
        if outcome == 'worse':
            R['worse'].append((key, diag, debt[row][0], dist))
        elif outcome == 'improved':
            R['improved'].append((key, debt[row][0], dist))
        elif outcome == 'moved':
            R['moved'].append((key, row))
    inherited = {r['from'] for r, _ in recs if 'from' in r}
    R['orphans'] = [] if scoped else sorted(set(debt) - present - inherited)
    R.update(ok=n_ok, tot=len(recs), debt=n_debt, ambiguous=n_amb,
             per_copy=[r for r, _ in recs])
    return R


def judge(copies, syms, protected, debt, results=None, scoped=False, ldebt=None, l16debt=None):
    """Judge the green copies [(sym, unit, got_ours, ident[, local_ours])] on the
    dimensions: the global symbols through the GOT (debt `debt`, config/got_debt.txt),
    the data reached through the local GOT (debt `ldebt`, config/gotlocal_debt.txt;
    only if the copy carries local_ours) and the LO16 targets (debt `l16debt`).

    Returns the dict of the GOT dimension, with `loc` = that of the data dimension
    inside. In each: `new` (outside the debt) and `worse` (distance increased,
    or fingerprint changed at equal distance) fail the gate; `improved`, `moved`
    (row inherited from another unit, see verdict), `clean` (debt copies now clean),
    `orphans` (debt rows not seen: full runs only), `warnings`, `per_copy` (for
    --write-debt), counts. Statics attributed by content to several candidates are
    measured and not judged; in scoped runs statics attributed by content to a single
    candidate are judged but give a warning, not a failure (the address may differ
    from the full run, and the debt row with it). Annotates `results[sym]` with
    `got_ok`/`got_diag` and `gotloc_ok`/`gotloc_diag`."""
    orig = Orig(protected)
    present = {f'{c[0]}@{c[1]}' for c in copies if c[0] in syms}
    g_recs, l_recs, t_entries, t_ident = [], [], [], {}
    for c in copies:
        sym, unit, ours, ident = c[:4]
        lours = c[4] if len(c) > 4 else None
        if len(c) > 5 and c[5] is not None and sym in syms:
            k = f'{sym}@{unit}'
            t_entries.append((k, sym, unit, c[6] if len(c) > 6 else sym, c[5],
                              'candidates' in (ident or ''), syms[sym]))
            t_ident[k] = (sym, unit, ident)
        if sym not in syms:
            continue
        key = f'{sym}@{unit}'
        addr, size = syms[sym]
        o = orig.names(sym, addr, size)
        same, dist, fp, diag = compare(o, ours, sym in protected, UNIT_DEFS.get(unit, ()), orig.bind,
                                       orig.local_callees(sym, addr, size))
        g_recs.append(({'key': key, 'ok': same, 'dist': dist, 'fp': fp, 'diag': diag}, ident))
        r = results.get(sym) if results is not None else None
        if r is not None:
            r['got_ok'] = bool(r.get('got_ok', True) and same)
            if not same and 'got_diag' not in r:
                r['got_diag'] = f'{diag} [{unit}]'
        if lours is not None:
            lo = orig.local_data(sym, addr, size)
            ls, ld, lf, ldg = compare_local(lo, lours)
            l_recs.append(({'key': key, 'ok': ls, 'dist': ld, 'fp': lf, 'diag': ldg}, ident))
            if r is not None:
                r['gotloc_ok'] = bool(r.get('gotloc_ok', True) and ls)
                if not ls and 'gotloc_diag' not in r:
                    r['gotloc_diag'] = f'{ldg} [{unit}]'
    R = _ratchet(g_recs, debt, present, scoped)
    R['loc'] = _ratchet(l_recs, ldebt if ldebt is not None else {}, present, scoped)
    t_recs = []
    for k, rec in lo16_judge(t_entries, orig, full_run=not scoped).items():
        sym, unit, ident = t_ident[k]
        t_recs.append((rec, ident))
        r = results.get(sym) if results is not None else None
        if r is not None:
            r['gotlo_ok'] = bool(r.get('gotlo_ok', True) and rec['ok'])
            if not rec['ok'] and 'gotlo_diag' not in r:
                r['gotlo_diag'] = f'{rec["diag"]} [{unit}]'
    R['lo16'] = _ratchet(t_recs, l16debt if l16debt is not None else {}, present, scoped)
    R['lo16']['unverifiable'] = sum(r['unverifiable'] for r, _ in t_recs)
    why = Counter()
    for r, _ in t_recs:
        why.update(r.get('unverifiable_why', {}))
    R['lo16']['unverifiable_why'] = dict(why)
    R['lo16']['refs'] = sum(len(e[4]) for e in t_entries)
    return R


_HEADERS = {
    DEBT: ('# GOT symbol identity debt -- GENERATED by `make got-debt`\n'
           '#   (python3 tools/gotident.py --write-debt, after a full `make check`).\n'
           '# One row = one green copy (SYMBOL@UNIT) with identical instructions that references\n'
           '# through the GOT at least one global symbol different from the original, with the\n'
           '# distance and fingerprint of the two sequences. The gate fails for a dirty green copy\n'
           '# not listed here, or for a row whose distance grows / whose fingerprint changes.\n'
           '# By default `make got-debt` only removes rows; adding rows needs --add.\n'
           '# `[TU]` = TU boundary: move the body, do not rename. See tools/gotident.py.\n'),
    LDEBT: ('# Identity debt of the data reached through the local GOT -- GENERATED by `make got-debt`\n'
            '#   (python3 tools/gotident.py --write-debt, after a full `make check`).\n'
            '# One row = one green copy (SYMBOL@UNIT) with identical instructions that, through the\n'
            '# local GOT, reaches an object in another section (e.g. a .bss buffer where the original\n'
            '# has a .rodata literal, an empty .bss table where the original has initialized .data,\n'
            '# a data object where the original has a function pointer) or with another name (when\n'
            '# the object of the original has one in config/symbol_addrs.txt). Same ratchet as the\n'
            '# GOT debt. See tools/gotident.py.\n'),
    TDEBT: ('# Identity debt of the LO16 targets -- GENERATED by `make got-debt`\n'
            '#   (python3 tools/gotident.py --write-debt, after a full `make check`).\n'
            '# One row = one green copy (SYMBOL@UNIT) with identical instructions in which a local\n'
            '# GOT + LO16 pair (a local-page call `lw t9`+`addiu t9`, a `la`, a `lw/sw %lo(x)`)\n'
            '# reaches an address different from the original: the wrong function of the same TU,\n'
            '# or a data object outside the placement of its section. Same ratchet as the GOT debt.\n'
            '# See tools/gotident.py.\n'),
}


def write_debt(per_copy, add=False, path=DEBT):
    """Rewrite a debt: keep the rows still dirty (updating distance and fingerprint
    if the distance decreased), move the rows inherited from another unit to the new
    key (verdict: 'moved'), remove the clean and vanished ones. A worsened row
    keeps the old value; with add=True it is absorbed but reported separately
    (`absorbed`), so `--add` does not hide a regression. Dirty copies outside the
    debt are added only with add=True.
    -> dict(rows, added, removed, worse, absorbed, moved)."""
    old = load_debt(path)
    old_diag = {}
    p = os.path.join(ROOT, path)
    if os.path.exists(p):
        for line in open(p):
            body, _, comment = line.partition('#')
            if body.strip():
                # separator ' | ' (older files use the middle dot)
                sep = '| ' if '| ' in comment else '\u00b7 '
                old_diag[body.strip()] = comment.split(sep, 1)[-1].strip()
    present = {c['key'] for c in per_copy}
    used = set()
    keep, out = {}, {k: [] for k in ('added', 'worse', 'absorbed', 'moved')}
    for c in per_copy:
        k = c['key']
        if c['ok'] or c.get('amb'):
            continue
        outcome, row = verdict(k, c['dist'], c['fp'], old, present, used)
        if outcome == 'worse':
            if add:
                keep[k] = (c['dist'], c['fp'], c['diag'])
                out['absorbed'].append(k)
            else:
                keep[k] = (old[k][0], old[k][1], old_diag.get(k, c['diag']))
                out['worse'].append(k)
        elif outcome in ('debt', 'improved', 'moved'):
            keep[k] = (c['dist'], c['fp'], c['diag'])
            if outcome == 'moved':
                out['moved'].append(f'{row} -> {k}')
        elif add:
            keep[k] = (c['dist'], c['fp'], c['diag'])
            out['added'].append(k)
    out['removed'] = sorted(set(old) - set(keep))
    out['rows'] = len(keep)
    rows = sorted(keep.items(), key=lambda kv: (kv[0].split('@', 1)[-1], kv[0]))
    with open(p, 'w') as f:
        f.write(_HEADERS.get(path, _HEADERS[DEBT]))
        cur = None
        for k, (d, fp, diag) in rows:
            unit = k.split('@', 1)[-1]
            if unit != cur:
                f.write(f'\n# == {unit}\n')
                cur = unit
            f.write(f'{k}  # dist={d} fp={fp} | {diag}\n')
    return out


# --------------------------------------------------------------------------- #
# CLI
# --------------------------------------------------------------------------- #
def _ours_from_obj(obj, lname):
    st = subprocess.run([OBJDUMP, '-t', obj], capture_output=True, text=True).stdout
    loc, defs = symtab_info(st)
    objsyms = {}
    for line in st.splitlines():
        m = FUNC_RE.match(line)
        if m:
            objsyms[m.group(4)] = (int(m.group(1), 16), int(m.group(3), 16), m.group(2))
    dis = subprocess.run([OBJDUMP, '-z', '-dr', obj], capture_output=True, text=True).stdout
    data, tgt = local_refs_by_func(dis, objsyms, st)
    _ours_from_obj.local = data.get(lname)
    _ours_from_obj.targets = tgt
    _ours_from_obj.objsyms = objsyms
    return ours_by_func(dis, objsyms, loc).get(lname), defs


def _lo16_single(O, syms, unit, sym, lname):
    """Third dimension on one .o outside the gate (tools/match, CLI): the identity of
    the .o's functions comes from the last gate (universe names + build/check_statics.json
    for the locals; those whose address by content is ambiguous stay unverifiable),
    and the placement of the data sections is voted on the copies the last gate gave
    green in this unit (build/check.json), plus this one. -> lo16_judge rec, or None."""
    tgt = getattr(_ours_from_obj, 'targets', None) or {}
    F = getattr(_ours_from_obj, 'objsyms', None) or {}
    if lname not in tgt:
        return None
    ad = {n: syms[n][0] for n in F if n in syms}
    amb_loc = set()
    st_p = os.path.join(ROOT, 'build/check_statics.json')
    st_rows = json.load(open(st_p)).get(unit, []) if unit and os.path.exists(st_p) else []
    for r in st_rows:
        if r.get('state') == 'misaligned':
            continue
        if 'candidates' in (r.get('ident') or ''):
            amb_loc.add(r['local'])
            continue
        ad[r['local']] = r['addr']
    for n in amb_loc:
        ad.pop(n, None)
    if sym in syms:
        ad[lname] = syms[sym][0]
    voters = []
    chk_p = os.path.join(ROOT, 'build/check.json')
    if unit and os.path.exists(chk_p):
        for s, r in json.load(open(chk_p)).items():
            if r.get('unit') != unit or not r.get('ok') or s == sym or s not in syms:
                continue
            if 'candidates' in (r.get('ident') or ''):
                continue
            ln = r.get('local') or s
            if ln in tgt:
                voters.append((f'{s}@{unit}', s, unit, ln, tgt[ln], False, syms[s]))
    me = (f'{sym}@{unit}', sym, unit, lname, tgt[lname], False, syms[sym])
    # a linkonce copy is not compared here with the copies of the other units (it
    # would need the placement of their sections): a reference of it that does not
    # work for its own unit stays unverifiable, the gate judges it
    res = lo16_judge([me] + voters, O, {unit: F}, {unit: ad}, {unit: {}}, {unit: tgt},
                     lk_alone=True)
    return res[me[0]]


def _env():
    import universe
    import dnas_ref
    syms = {s: v for s, v in universe.symbols('config/symbol_addrs.txt').items() if v[1]}
    return syms, dnas_ref.protected(syms) & set(syms)


def _show_local(orig_seq, ours_seq):
    n = max(len(orig_seq), len(ours_seq))
    for i in range(n):
        o = orig_seq[i] if i < len(orig_seq) else ('-', None)
        u = ours_seq[i] if i < len(ours_seq) else ('-', None)
        mark = '  ' if o[0] == u[0] and (not o[1] or o[1] == u[1] or o[1].startswith('static_')) else '!!'
        print(f'    {mark} #{i:<3} orig {o[0]:8} {o[1] or "(unnamed)":40} ours {u[0]:8} {u[1] or "(unnamed)"}')


def _show(sym, orig_names, ours, prot):
    print('    orig:', ' '.join(orig_names) or '(none)')
    print('    ours:', ' '.join(ours) or '(none)')


def _copies_of(sym):
    """[(unit, local)] of the green copies of sym in the last gate: from the full run
    (build/got_copies.json, all dedup copies) and, if more recent, from a scoped json
    (build/check-*.json). Never an old scoped json over the full run."""
    out = {}
    full = os.path.join(ROOT, COPIES)
    t_full = os.path.getmtime(full) if os.path.exists(full) else 0
    chk = os.path.join(ROOT, 'build/check.json')
    loc = {}
    if os.path.exists(chk):
        r = json.load(open(chk)).get(sym) or {}
        if r.get('local'):
            loc[r.get('unit')] = r['local']
    if os.path.exists(full):
        cp = json.load(open(full))
        for c in (cp.get('got', []) if isinstance(cp, dict) else cp):
            s, _, u = c['key'].partition('@')
            if s == sym:
                out[u] = loc.get(u, sym)
    for jp in sorted(glob.glob(os.path.join(ROOT, 'build/check-*.json')),
                     key=os.path.getmtime):
        if os.path.getmtime(jp) <= t_full:
            continue
        try:
            r = json.load(open(jp)).get(sym)
        except Exception:
            continue
        if isinstance(r, dict) and 'unit' in r:
            out[r['unit']] = r.get('local') or sym
    return sorted(out.items())


def main():
    argv = sys.argv[1:]

    def opt(name):
        if name not in argv:
            return None
        i = argv.index(name) + 1
        if i >= len(argv) or argv[i].startswith('-'):
            sys.exit(f'error: {name} needs a value')
        return argv[i]

    if '--write-debt' in argv:
        p = os.path.join(ROOT, COPIES)
        if not os.path.exists(p):
            sys.exit(f'error: {COPIES} missing: run a full make check first')
        cp = json.load(open(p))
        if isinstance(cp, list):          # format from before the data dimension
            cp = {'got': cp, 'loc': []}
        for path, recs in ((DEBT, cp.get('got', [])), (LDEBT, cp.get('loc', [])),
                           (TDEBT, cp.get('lo16', []))):
            if not recs and path in (LDEBT, TDEBT):
                continue
            r = write_debt(recs, add='--add' in argv, path=path)
            print(f'{path}: {r["rows"]} rows, {len(r["removed"])} removed, {len(r["added"])} added, '
                  f'{len(r["moved"])} moved')
            for k in r['added'][:20]:
                print(f'   [+] {k}')
            if len(r['added']) > 20:
                print(f'   ... and {len(r["added"]) - 20} more')
            for k in r['moved'][:20]:
                print(f'   [>] {k}')
            if r['worse']:
                print(f'warning: {len(r["worse"])} rows got worse, kept with the old value '
                      f'(the gate rejects them): {", ".join(r["worse"][:10])}')
            if r['absorbed']:
                print(f'warning: --add absorbed {len(r["absorbed"])} rows that got worse '
                      f'(accepted regressions): {", ".join(r["absorbed"][:20])}')
        return
    args, i = [], 0
    while i < len(argv):          # positional: what is not an option or its value
        if argv[i] in ('--obj', '--local', '--unit', '--map'):
            i += 2
            continue
        if not argv[i].startswith('-'):
            args.append(argv[i])
        i += 1
    if '--map' in argv:
        # Positional map of the dirty copies of a unit: when the two sequences have
        # the same length and each of our names always maps to the same original
        # name, the rename is mechanical (ours -> orig).
        unit = opt('--map')
        syms, prot = _env()
        O = Orig(prot)
        p = os.path.join(ROOT, COPIES)
        if not os.path.exists(p):
            sys.exit(f'error: {COPIES} missing: run a full make check first')
        chk = json.load(open(os.path.join(ROOT, 'build/check.json')))
        obj = os.path.join(ROOT, 'build/check', re.sub(r'[^A-Za-z0-9_]', '_', unit), 'out.o')
        if not os.path.exists(obj):
            sys.exit(f'error: {obj} missing: run the gate on that unit')
        if subprocess.run(['which', OBJDUMP], capture_output=True).returncode != 0:
            sys.exit(f'error: {OBJDUMP} not found: run in the container (docker run ... open-bn-env)')
        dirty = 0
        cp = json.load(open(p))
        for c in (cp.get('got', []) if isinstance(cp, dict) else cp):
            s, _, u = c['key'].partition('@')
            if u != unit or c['ok'] or s not in syms:
                continue
            lname = (chk.get(s) or {}).get('local') or s
            o = O.names(s, *syms[s])
            ours, defs = _ours_from_obj(obj, lname)
            if ours is None:
                print(f'{s} ({lname}): not found in the .o')
                continue
            dirty += 1
            m, ok = {}, len(o) == len(ours)
            if ok:
                for a, b in zip(ours, o):
                    if a != b:
                        if m.get(a, b) != b:
                            ok = False
                        m[a] = b
                # injective: two of our names cannot map to the same original name
                if len(set(m.values())) != len(m):
                    ok = False
            if ok and m:
                print(f'{s} ({lname}): MAP ' + ', '.join(
                    f'{a} -> {b}' + (' (defined in this unit: rename and move the body, '
                                     'otherwise it becomes a local-page call)' if b in defs else '')
                    for a, b in sorted(m.items())))
            else:
                print(f'{s} ({lname}): no positional map -- {c["diag"]}')
                _show(s, o, ours, prot)
        if not dirty:
            print(f'{unit}: no dirty copy in the full run ({COPIES})')
        return
    if '--obj' in argv:
        obj, lname, unit = opt('--obj'), opt('--local'), opt('--unit')
        if not args:
            sys.exit('usage: gotident.py --obj OUT.o [--local NAME] [--unit U] SYM')
        sym = args[0]
        syms, prot = _env()
        if sym not in syms:
            return
        ours, defs = _ours_from_obj(obj, lname or sym)
        if ours is None:
            return
        lours = getattr(_ours_from_obj, 'local', None) or []
        O = Orig(prot)
        o = O.names(sym, *syms[sym])
        same, dist, fp, diag = compare(o, ours, sym in prot, defs, O.bind,
                                       O.local_callees(sym, *syms[sym]))
        lo = O.local_data(sym, *syms[sym])
        ls, ld, lf, ldg = compare_local(lo, lours)
        t = _lo16_single(O, syms, unit, sym, lname or sym) or {'ok': True}
        ok_all = True
        # tools/match: silent if the identity is as in the original; debt rows
        # (accepted) and unverifiable references only with VERBOSE=1
        loud = os.environ.get('VERBOSE', '') not in ('', '0')
        for label, sm, d, f, dg, path, show in (
                ('GOT symbol', same, dist, fp, diag, DEBT, lambda: _show(sym, o, ours, prot)),
                ('local GOT data', ls, ld, lf, ldg, LDEBT, lambda: _show_local(lo, lours)),
                ('LO16 target', t['ok'], t.get('dist'), t.get('fp'), t.get('diag'), TDEBT,
                 lambda: None)):
            if sm:
                unverifiable_count = t.get('unverifiable') if label == 'LO16 target' else 0
                if loud and unverifiable_count:
                    print(f'note: {label} identity as in the original, {unverifiable_count} references '
                          f'not verifiable: {t.get("unverifiable_why")}')
                continue
            debt = load_debt(path)
            key = f'{sym}@{unit}' if unit else None
            if key:
                # tools/match does not know which other units exist right now: for
                # inheritance it is tolerant (any row of the same symbol in another unit
                # with the same fingerprint or strictly lower distance); the gate decides
                outcome, row = verdict(key, d, f, debt, set())
            else:
                rows = [k for k in debt if k.split('@', 1)[0] == sym]
                outcome, row = ('debt', rows[0]) if rows else ('new', None)
            ok = outcome in ('debt', 'improved', 'moved')
            tag = {'debt': f'in the debt ({row})', 'improved': f'in the debt ({row}), improved',
                   'moved': f'row inherited from {row}', 'worse': f'worse than {row} '
                   f'(dist {debt.get(row, (0,))[0]} -> {d})', 'new': 'not in the debt'}[outcome]
            if ok and not loud:
                continue
            print(f'{"note" if ok else "error"}: {label} identity differs (dist={d}), {tag}: {dg}')
            show()
            ok_all = ok_all and ok
        if not ok_all:
            print('    the gate rejects this: use the real name/object (names in .dynsym and '
                  'config/symbol_addrs.txt), or move the body if it is [TU]; for LO16 targets: '
                  'call/reach the function or object of the TU the original reaches at that address')
            sys.exit(1)
        return
    if not args:
        print('usage: gotident.py SYM [SYM...] [--unit U] | --map UNIT | --write-debt [--add] | '
              '--obj OUT.o [--local NAME] [--unit U] SYM')
        return
    syms, prot = _env()
    o = Orig(prot)
    for s in args:
        if s not in syms:
            print(f'{s}: not in the universe')
            continue
        orig_names = o.names(s, *syms[s])
        copies = [(opt('--unit'), None)] if opt('--unit') else _copies_of(s)
        if not copies:
            print(f'== {s}')
            print('    orig:', ' '.join(orig_names) or '(none)')
            print('    ours: (no green copy in the last gate: pass --unit)')
            continue
        for unit, lname in copies:
            print(f'== {s}  [{unit}]')
            obj = os.path.join(ROOT, 'build/check', re.sub(r'[^A-Za-z0-9_]', '_', unit), 'out.o')
            if not os.path.exists(obj):
                print('    orig:', ' '.join(orig_names) or '(none)')
                print(f'    ours: ({obj} missing: run the gate on that unit)')
                continue
            ours, defs = _ours_from_obj(obj, lname or s)
            if ours is None:
                print('    orig:', ' '.join(orig_names) or '(none)')
                print(f'    ours: (function {lname or s} not found in the .o)')
                continue
            same, dist, fp, diag = compare(orig_names, ours, s in prot, defs, o.bind,
                                           o.local_callees(s, *syms[s]))
            _show(s, orig_names, ours, prot)
            print('    -> GOT symbols:', 'identity ok' if same else f'differs (dist={dist}): {diag}')
            lo = o.local_data(s, *syms[s])
            lu = getattr(_ours_from_obj, 'local', None) or []
            ls, ld, lf, ldg = compare_local(lo, lu)
            if not ls:
                _show_local(lo, lu)
            print('    -> local GOT data:', 'identity ok' if ls else f'differs (dist={ld}): {ldg}')
            t = _lo16_single(o, syms, unit, s, lname or s)
            if t is not None:
                print('    -> LO16 targets:', 'identity ok' if t['ok'] else
                      f'differs (dist={t["dist"]}): {t["diag"]}', f'({t["unverifiable"]} not verifiable)')


if __name__ == '__main__':
    main()
