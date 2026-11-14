#!/usr/bin/env python3
"""The nameless functions of .text: the `static` functions (and the crt) missing from .dynsym.

The shipped binary has no `.symtab`: `config/symbol_addrs.txt` comes from `.dynsym`
and covers 3,682,256 of the 4,979,944 bytes of `.text`. The rest -- 1,297,688 bytes in
4,969 gaps -- is code (the TUs' `static` functions, plus the crt at the head).

How the boundaries are found without names (checked on the 8,403 known functions):
  - every function compiled by gcc 2.95 -mabicalls starts with `.cpload $25`,
    i.e. the triple `lui gp,HI / addiu gp,gp,LO / addu gp,gp,t9`: 8403/8403.
  - every start is 8-aligned: 8403/8403 (4776 at 0 mod 16, 3627 at 8).
  - the .dynsym size does not include the zero padding after the function
    (size mod 8 = 4 in 4482 cases); the final delay slot may be a nop.
So a function start in a gap is an 8-aligned .cpload triple; the end is the
next start (static or .dynsym) minus the zero padding words, keeping one zero
word if it is the delay slot of a jump (func_end). Applied to the 8,403 known
functions as if they were anonymous, the rule gives back 8403 starts and 8403
exact sizes (`--validate`): the method is not a heuristic.
What does not start with a prologue is an anomaly, printed and left out of the
count: 5 regions / 39,152 B, none produced by the compiler -- the crt `_start`
at the head of .text (96 B, hand-written asm of crt1.o), three DNAS data tables
in .text (`_sce_dnas_pwrap_info_tbl` 25,296 B, `__sce_dnas2_symbol11001`
4,304 B, `__sce_dnas2_symbol12001` 4,928 B, OBJECT in .dynsym) and the linker's
lazy-binding stubs (`lw t9,-32752(gp) / move t7,ra / jalr t9 / li t8,N`,
4,512 B at 0x911690: `tanf` & co. are these).
DNAS ciphertext: detection runs on the decrypted image (tools/dnas_ref.py), where
the regions are ordinary code.

The synthetic names are `static_<ADDR as 8 upper-case hex digits>`. They are an
identity by address: the real name was lost with .symtab, and in our .o the same
function carries the name we gave it. `locate()` links the two: the .dynsym
functions present in the .o are anchors (name -> known address), and a local
function of the .o sits at `anchor + (its offset - anchor's offset)` -- the
layout of a TU in the link is that of its .o (the .text sections of the .o files
are concatenated, gcc 2.95 emits functions in source order).

Usage:
    python3 tools/statics.py --validate   redo the measurement on the known functions
    python3 tools/statics.py --write      regenerate config/static_funcs.txt
    python3 tools/statics.py              summary (gaps, functions, bytes, anomalies)
    python3 tools/statics.py --verify-list   the committed list is the detector's (make check)
    python3 tools/statics.py --locate build/check/<unit>/out.o [--for static_XXXXXXXX]
                                          (container) original addresses of a .o's locals
    python3 tools/statics.py --populate-skips   after a gate: put the red statics in the skip column

As a module (check.py, split.py, universe.py, match.sh via --locate):
    statics.load()                -> {name: (addr, size)} from config/static_funcs.txt
    statics.detect(text, dynsym)  -> ([(addr, size)], anomalies)  over the gaps
    statics.locate(objsyms, syms) -> {local_name: {addr, size, sec, anchor, dir, alt}}
    statics.name(addr)            -> 'static_XXXXXXXX'
"""
import json
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LIST = 'config/static_funcs.txt'
ELF = 'orig/bn-jpn.elf'
TEXT_START = 0x47C1D0
TEXT_SIZE = 0x4BFCE8
TEXT_OFF = 0x7C1D0            # file offset of .text (readelf -S)
NAME_RE = re.compile(r'^static_([0-9A-F]{8})$')
LINE_RE = re.compile(r'^(static_[0-9A-F]{8}) = (0x[0-9A-Fa-f]+); // type:func size:(0x[0-9A-Fa-f]+)')

ADDU_GP_T9 = 0x0399E021


def name(addr):
    return f'static_{addr:08X}'


def is_static(sym):
    return bool(NAME_RE.match(sym))


def addr_of(sym):
    m = NAME_RE.match(sym)
    return int(m.group(1), 16) if m else None


# --------------------------------------------------------------------------- #
# generated list
# --------------------------------------------------------------------------- #
def load(path=LIST):
    """{name: (addr, size)} from the generated list. Empty if the file is
    missing: the tools still work on the .dynsym universe alone."""
    out = {}
    p = os.path.join(ROOT, path)
    if not os.path.exists(p):
        return out
    for line in open(p):
        m = LINE_RE.match(line)
        if m:
            out[m.group(1)] = (int(m.group(2), 16), int(m.group(3), 16))
    return out


# --------------------------------------------------------------------------- #
# detection on the bytes
# --------------------------------------------------------------------------- #
def text_image(path=None):
    """The .text bytes of the image (decrypted if available)."""
    if path is None:
        import dnas_ref
        path = dnas_ref.ensure_image()
    data = open(path, 'rb').read()
    return data[TEXT_OFF:TEXT_OFF + TEXT_SIZE]


def _word(text, va):
    return struct.unpack_from('<I', text, va - TEXT_START)[0]


def is_prologue(text, va):
    if va + 12 > TEXT_START + TEXT_SIZE:
        return False
    w0, w1, w2 = (_word(text, va), _word(text, va + 4), _word(text, va + 8))
    return (w0 >> 16) == 0x3C1C and (w1 >> 16) == 0x279C and w2 == ADDU_GP_T9


def gaps(syms):
    """[(start, end)] of .text not covered by sized functions in `syms`."""
    iv = sorted({(a, a + s) for (a, s) in syms.values() if s})
    out, prev = [], TEXT_START
    for a, b in iv:
        if a > prev:
            out.append((prev, a))
        prev = max(prev, b)
    end = TEXT_START + TEXT_SIZE
    if prev < end:
        out.append((prev, end))
    return out


STUB = (0x8F998010, 0x03E07825, 0x0320F809)   # lw t9,-32752(gp) / move t7,ra / jalr t9 / li t8,N


def is_stub(text, va):
    """Linker lazy-binding stub (.MIPS.stubs, 16 B each): not compiler output."""
    if va + 16 > TEXT_START + TEXT_SIZE:
        return False
    return (tuple(_word(text, va + 4 * i) for i in range(3)) == STUB
            and (_word(text, va + 12) >> 16) == 0x3418)


def has_delay_slot(w):
    """Is instruction w a jump/branch with a delay slot?"""
    op = w >> 26
    if op in (2, 3, 4, 5, 6, 7, 0x14, 0x15, 0x16, 0x17):
        return True
    if op == 1:                       # REGIMM: bltz/bgez/bltzl/bgezl/bltzal/bgezal...
        return ((w >> 16) & 0x1F) in (0, 1, 2, 3, 16, 17, 18, 19)
    if op == 0 and (w & 0x3F) in (8, 9):   # jr / jalr
        return True
    if op == 0x11 and ((w >> 21) & 0x1F) == 8:   # bc1f/bc1t(l)
        return True
    return False


def func_end(text, a, nxt):
    """End of the function starting at `a` whose successor starts at `nxt`:
    the trailing zero padding words are dropped, keeping the delay slot (a zero
    word right after a jump). Gives back the 8403 exact .dynsym sizes,
    including the 4482 with size = 4 mod 8."""
    e = nxt
    while e - 4 >= a + 12 and _word(text, e - 4) == 0:
        e -= 4
    if e < nxt and has_delay_slot(_word(text, e - 4)):
        e += 4
    return e


def split_region(text, start, end):
    """Functions inside [start, end): ([(addr, size)], [anomalies]).

    The starts are the .cpload prologues at 8-aligned addresses; the end is the
    next prologue minus the zero padding (func_end). What does not start with a
    prologue (crt, data tables, linker stubs) is an anomaly: reported and not
    counted as code to decompile."""
    funcs, anomalies = [], []
    va = start
    starts = []
    stubs = None
    while va + 12 <= end:
        if va % 8 == 0 and is_prologue(text, va):
            starts.append(va)
            va += 12
        elif va % 16 == 0 and is_stub(text, va):
            if stubs is None:
                stubs = va
            va += 16
        else:
            va += 4
    if stubs is not None:
        # from the stubs on there is no more compiled code: the region ends there
        n = stubs
        while n + 16 <= end and is_stub(text, n):
            n += 16
        tail = [x for x in range(n, end, 4) if _word(text, x)]
        anomalies.append((stubs, end, f'{n - stubs} B of linker lazy-binding stubs'
                          + (f' + {end - n} B after' if tail else '')))
        starts = [x for x in starts if x < stubs]
        end = stubs
    if not starts:
        body = [x for x in range(start, end, 4) if _word(text, x)]
        if body:
            anomalies.append((start, end, f'{end - start} B without a prologue'))
        return funcs, anomalies
    if starts[0] != start:
        lead = [x for x in range(start, starts[0], 4) if _word(text, x)]
        if lead:
            anomalies.append((start, starts[0],
                              f'{starts[0] - start} B without a prologue before the first'))
    for i, a in enumerate(starts):
        nxt = starts[i + 1] if i + 1 < len(starts) else end
        fend = func_end(text, a, nxt)
        if nxt - fend > 12:
            anomalies.append((fend, nxt, f'{nxt - fend} B of zero padding (>3 words)'))
        funcs.append((a, fend - a))
    return funcs, anomalies


def detect(text, syms):
    """All the functions in the gaps of .text: ([(addr, size)], [anomalies])."""
    funcs, anomalies = [], []
    for s, e in gaps(syms):
        f, a = split_region(text, s, e)
        funcs += f
        anomalies += a
    return funcs, anomalies


def validate(text, syms):
    """Redo the detection on the known functions as if they had no name: each
    run of contiguous .dynsym functions is treated as a gap and the starts/sizes
    that come back identical are counted. This is what validates the method."""
    known = sorted((a, s) for (a, s) in syms.values() if s)
    runs, cur = [], None
    for a, s in known:
        if cur and a <= cur[1] + 4 and cur[1] % 4 == 0:
            cur[1] = max(cur[1], a + s)
            cur[2].append((a, s))
        else:
            cur = [a, a + s, [(a, s)]]
            runs.append(cur)
    hit_start = hit_size = 0
    total = len(known)
    missed = []
    for start, end, members in runs:
        # the "real" gap reaches the next 8-aligned start
        got, _ = split_region(text, start, (end + 7) // 8 * 8)
        g = dict(got)
        for a, s in members:
            if a in g:
                hit_start += 1
                if g[a] == s:
                    hit_size += 1
                elif len(missed) < 10:
                    missed.append((a, s, g[a]))
            elif len(missed) < 10:
                missed.append((a, s, None))
    return total, hit_start, hit_size, missed


# --------------------------------------------------------------------------- #
# linking the locals of a .o to the original addresses
# --------------------------------------------------------------------------- #
def locate(objsyms, syms, section='.text'):
    """{local_name: {addr, size, sec, anchor, dir, alt, alt_anchor}} for the .o
    functions that are not in `syms` (static, or non-exported globals).

    objsyms: {name: (offset, size, section)} as check.defined_functions().
    Anchors: functions of the .o present in syms (with a size), same section.
    Each local uses the nearest preceding anchor (dir '+'); if there is none,
    the nearest following one (dir '-'). With both anchors available the
    alternative address `alt` is returned too: when the two differ, the TU has
    a function of the wrong size between the two anchors (check.py uses it to
    realign what follows)."""
    anchors = sorted((off, syms[n][0], n) for n, (off, sz, sec) in objsyms.items()
                     if n in syms and syms[n][1] and sec == section and sz)
    out = {}
    if not anchors:
        return out
    for n, (off, sz, sec) in objsyms.items():
        if sec != section or not sz or n in syms:
            continue
        prev = [a for a in anchors if a[0] <= off]
        nxt = [a for a in anchors if a[0] > off]
        fwd = prev[-1][1] + (off - prev[-1][0]) if prev else None
        bwd = nxt[0][1] - (nxt[0][0] - off) if nxt else None
        if fwd is not None:
            out[n] = {'addr': fwd, 'size': sz, 'sec': sec, 'anchor': prev[-1][2],
                      'dir': '+', 'alt': bwd, 'alt_anchor': nxt[0][2] if nxt else None}
        else:
            out[n] = {'addr': bwd, 'size': sz, 'sec': sec, 'anchor': nxt[0][2],
                      'dir': '-', 'alt': None, 'alt_anchor': None}
    return out


# --------------------------------------------------------------------------- #
def write_list(funcs, anomalies, path=LIST, validation=None):
    lines = ['# GENERATED by `python3 tools/statics.py --write`: do not edit by hand.',
             '# .text functions without a name in .dynsym (static + crt), detected by the',
             '# .cpload prologue, closed by the next prologue minus the padding (tools/statics.py).',
             f'# {len(funcs)} functions, {sum(s for _, s in funcs)} bytes; '
             f'{len(anomalies)} anomalous regions excluded (see `python3 tools/statics.py`).']
    if validation:
        total, hs, hz, _ = validation
        lines.append(f'# Validation on the {total} .dynsym functions treated as anonymous: '
                     f'starts found {hs}/{total}, exact sizes {hz}/{total}.')
    lines.append('# Same format as config/symbol_addrs.txt; the name is the address.')
    for a, s in funcs:
        lines.append(f'{name(a)} = 0x{a:08X}; // type:func size:0x{s:X}')
    with open(os.path.join(ROOT, path), 'w') as f:
        f.write('\n'.join(lines) + '\n')


def main():
    import universe
    args = sys.argv[1:]
    syms = universe.dynsym()
    if '--locate' in args:
        # (container) the locals of a .o with the original address derived
        # from the anchors; with --for SYM print only the local name that
        # corresponds to that static (for tools/match.sh)
        obj = args[args.index('--locate') + 1]
        VERBOSE = os.environ.get('VERBOSE', '') not in ('', '0')
        import check
        objsyms = check.defined_functions(obj)
        allsyms = universe.symbols()
        loc = locate(objsyms, allsyms)
        known = load()
        want = args[args.index('--for') + 1] if '--for' in args else None
        # --local NAME: force the local to compare (for tools/match), when the
        # anchors are not enough to decide
        forced = args[args.index('--local') + 1] if '--local' in args else None
        if want and forced:
            # `--local @REGEX` (LOCAL=@... in tools/match): for symbols whose
            # name changes at every compilation -- gcc 2.95 gives the
            # `_GLOBAL_$I$`/`_GLOBAL_$D$` of a TU a random per-invocation suffix
            # (`..._3Repsrc_natives_sysupdate.cppai2uva`), so a LOCAL= copied
            # from an earlier compile no longer exists in the next .o.
            if forced.startswith('@'):
                rx = re.compile(forced[1:])
                cand = sorted(n for n in objsyms if rx.search(n))
                if len(cand) != 1:
                    sys.exit(f'error: LOCAL=@{forced[1:]!r} in {obj}: '
                             + (f'{len(cand)} locals match ({", ".join(cand[:6])}'
                                + (', ...' if len(cand) > 6 else '') + ')'
                                if cand else 'no local matches')
                             + '\n    defined locals: ' + ', '.join(sorted(objsyms)[:20]))
                forced = cand[0]
                if VERBOSE:
                    print(f'note: LOCAL=@regex resolved to {forced!r}', file=sys.stderr)
            if forced not in objsyms:
                sys.exit(f'error: local {forced!r} not defined in {obj}.\n'
                         f'    defined locals: ' + ', '.join(sorted(objsyms)[:20])
                         + ('' if len(objsyms) <= 20 else f'  (+{len(objsyms) - 20} more)'))
            print(forced)
            return
        want_addr, want_size = known.get(want, (None, 0)) if want else (None, 0)
        # First the gate's assignment (build/check_statics.json, written by
        # tools/check.py): it is global and uses anchors, content and, for skips,
        # similarity (check._pair_skip_leftovers). Anchors alone go wrong where they
        # are sparse or the file has a body of a different size upstream (e.g.
        # static_00834938 would resolve to http_parse_date_fmt1 instead of
        # http_parse_date_fmt2). It applies only if that local still exists in the
        # freshly compiled .o (a body renamed after the last gate falls back to the
        # anchors); LOCAL= wins over everything.
        if want_addr is not None:
            try:
                gate = json.load(open(os.path.join(ROOT, 'build/check_statics.json')))
            except (OSError, ValueError):
                gate = {}
            hit = [(u, e) for u, rows in gate.items() for e in rows
                   if e.get('addr') == want_addr and e.get('local') in objsyms]
            if len(hit) == 1:
                u, e = hit[0]
                print(e['local'])
                if VERBOSE:
                    print(f"note: local from the gate assignment (build/check_statics.json, {u}: "
                          f"{e.get('state')}, {e.get('ident')})", file=sys.stderr)
                if objsyms[e['local']][1] != want_size:
                    print(f'warning: size: ours {objsyms[e["local"]][1]} B, original {want_size} B',
                          file=sys.stderr)
                return
        near = []   # fallback: locals overlapping the original body
        for n, d in sorted(loc.items(), key=lambda kv: kv[1]['addr']):
            cands = [d['addr']] + ([d['alt']] if d['alt'] not in (None, d['addr']) else [])
            names = [name(c) for c in cands]
            if want:
                if want in names:
                    print(n)
                    if known.get(want, (0, 0))[1] != d['size']:
                        print(f'warning: size: ours {d["size"]} B, original {known[want][1]} B',
                              file=sys.stderr)
                    return
                if want_addr is not None:
                    for c in cands:
                        if c < want_addr + want_size and c + d['size'] > want_addr:
                            near.append((abs(c - want_addr) + abs(d['size'] - want_size),
                                         c, n, d))
                continue
            sn = names[0]
            tag = 'OK' if sn in known and known[sn][1] == d['size'] else \
                  ('SIZE' if sn in known else 'OUT')
            alt = '' if d['alt'] in (None, d['addr']) else f'  alt=0x{d["alt"]:08X}'
            print(f'0x{d["addr"]:08X} {d["size"]:6d} {tag:5s} {n}  '
                  f'[{d["dir"]}{d["anchor"][:40]}]{alt}')
        if want and near:
            # no local falls exactly at want, but one overlaps it: the "out of
            # alignment" case (a function upstream, or this one, has a size
            # different from the original). That one is compared, and the shift
            # is reported: the gate matches by content anyway.
            _, c, n, d = min(near)
            print(n)
            print(f'warning: out of alignment: the anchors place {n!r} at 0x{c:08X}, '
                  f'{c - want_addr:+d} B from {want} (anchor {d["anchor"][:40]}); '
                  f'size ours {d["size"]} B, original {want_size} B. '
                  f'On a full MATCH the gate recognizes it by content; if it stays shifted, '
                  f'a function earlier in the file has a different size.',
                  file=sys.stderr)
            return
        if want:
            # The list is printed here, not just suggested: without it the message
            # reads as "gcc dropped my function", while the usual cause is a
            # simple mangling error (e.g. `__FPviPv` instead of `__FPviT0`).
            listing = '\n'.join(
                f'      0x{d["addr"]:08X} {d["size"]:6d} B  {n}'
                for n, d in sorted(loc.items(), key=lambda kv: kv[1]['addr'])[:25])
            if not listing:
                # TU of static functions only: `locate` has no anchors to count
                # offsets from and returns empty. Listing the .text locals of the
                # .o is the only useful thing to say, and it is the normal case for
                # a natives module (which exports nothing).
                tl = sorted((n, d) for n, d in objsyms.items()
                            if d[2] == '.text' and d[1])
                listing = '\n'.join(f'      {d[1]:6d} B  {n}' for n, d in tl[:25])
                if listing:
                    listing = ('      (the unit has no .dynsym anchors: no local can be '
                               'placed by address, matching goes by content)\n' + listing)
            sys.exit(f'error: no local function of {obj} falls at {want}: '
                     f'anchors or function order differ from the original.\n'
                     f'    locals defined by the .o (address from the anchors):\n'
                     + (listing or '      (none)')
                     + (f'\n      (+{len(loc) - 25} more)' if len(loc) > 25 else '')
                     + f'\n    force the right one with  LOCAL=<name> tools/match {want} <source>'
                     f'  (LOCAL=@REGEX if the name has a random suffix)')
        return
    if '--populate-skips' in args:
        # after a gate: put in the skip column (config/units.txt, via unitedit)
        # every red static -- body emitted, address known, no match -- so the
        # gate turns green again. A bulk operation; normally the skip column
        # is edited by hand with unitedit.
        import subprocess
        ck = json.load(open(os.path.join(ROOT, 'build/check.json')))
        by_unit = {}
        for sym, r in ck.items():
            if is_static(sym) and not r['ok'] and not r.get('skip'):
                by_unit.setdefault(r['unit'], []).append(sym)
        n = 0
        for unit, names in sorted(by_unit.items()):
            r = subprocess.run([sys.executable, 'tools/unitedit.py', unit,
                                '--skip-add', ','.join(sorted(names))],
                               cwd=ROOT, capture_output=True, text=True)
            if r.returncode:
                print(f'[X] {unit}: {r.stderr.strip() or r.stdout.strip()}')
            else:
                n += len(names)
        print(f'{n} red statics put in skip in {len(by_unit)} units')
        return
    if '--verify-list' in args:
        # gate: the committed list must be the one the detector produces now
        # (same idea as check_toolchain.py: a generated list is derived data,
        # and stale derived data is a false denominator)
        text = text_image()
        funcs, _ = detect(text, syms)
        want = {name(a): (a, s) for a, s in funcs}
        have = load()
        if want != have:
            sys.exit(f'statics    error: {LIST} does not match the detector ({len(want)} vs '
                     f'{len(have)} functions): regenerate with `python3 tools/statics.py --write`')
        print(f'statics    {len(have)} unnamed functions, '
              f'{sum(s for _, s in have.values())} B ({LIST} up to date)')
        return
    text = text_image()
    if '--validate' in args:
        total, hs, hz, missed = validate(text, syms)
        print(f'known functions {total}: starts found {hs}, exact sizes {hz}')
        for a, s, g in missed:
            print(f'  0x{a:08X} size {s:#x} -> {"not found" if g is None else hex(g)}')
        return
    funcs, anomalies = detect(text, syms)
    total_b = sum(s for _, s in funcs)
    anom_b = sum(e - s for s, e, _ in anomalies)
    g = gaps(syms)
    print(f'.text gaps: {len(g)}, {sum(e - s for s, e in g)} bytes')
    print(f'nameless functions: {len(funcs)}, {total_b} bytes')
    print(f'anomalies: {len(anomalies)} regions, {anom_b} bytes')
    for s, e, what in anomalies[:40]:
        print(f'  0x{s:08X}-0x{e:08X}  {what}')
    if len(anomalies) > 40:
        print(f'  ... +{len(anomalies) - 40}')
    if '--write' in args:
        val = validate(text, syms)
        write_list(funcs, anomalies, validation=val)
        print(f'wrote {LIST}')


if __name__ == '__main__':
    main()
