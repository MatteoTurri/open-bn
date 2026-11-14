#!/usr/bin/env python3
"""Gate of .eh_frame and .gcc_except_table: the exception tables, byte by byte.

These are 21,664 + 2,724 B of the original that the compiler emits from our C++
sources (units with exceptions) and that tools/datacheck.py does not cover
(DATA_UNIVERSE = .rodata/.data/.ctors/.dtors). A green function with a
try/catch in the wrong place, a unit compiled with exceptions that had none in
the original (or vice versa), a frame described differently: all invisible to
the other gates.

Format (gcc 2.95.2, old exception scheme, no -fnew-exceptions):
  .eh_frame: for every .o a CIE with augmentation "eh" (CIE = len, id 0, ver, "eh\\0",
     pointer to the .o's `__EXCEPTION_TABLE__` in .gcc_except_table, code/data align,
     RA column, instructions) and one FDE per function (len, distance from the CIE,
     pc_begin, pc_range, instructions). The linker concatenates the .eh_frame
     sections in link order and crtend.o closes with a 0 word.
  .gcc_except_table: for every .o `__EXCEPTION_TABLE__`, triples (start, end, handler)
     of labels in .text, closed by -1, -1 (except.c:output_exception_table).

Placement (identity by address, as in datacheck): every relocated word of our
.o pointing into .text is translated into the original address: our function
containing it is matched, so it has a known address in the original, and the
offset inside the function is the same on both sides. Anchors of .eh_frame =
the pc_begin of the FDEs (each translated pc_begin is looked up among the
original FDEs' pc_begin); anchor of the exception table = the "eh" pointer of
the original CIE, at the placed position. A relocated word that cannot be
translated is MASKED (counted separately).

Block states (JSON `state`): `ok`; `fail` (bytes differ, or `boundary`: in the
original the block does not open on a CIE (`start`), or after it the CIE continues
with other FDEs (`end`) = the TU differs; disagreeing anchors (`conflict`) =
function order differs from the original); `unanchored`; `out-of-section`;
`malformed` (with `error`). All but `ok` make it exit 1 unless the block
is in skip (`data:.eh_frame` / `data:.gcc_except_table` in the skip column of
config/units.txt). The "eh" word of the CIE is read from the original (it is the
anchor of the table): it stays masked and counts as equal only if the table it
points to is equal. Original FDEs whose pc_begin none of our FDEs translates
(units compiled without exceptions, functions without an FDE) are measured per
unit (`orphan_fdes`), not judged.

    python3 tools/ehcheck.py [--verbose] [--unit SRC]     (UNITS=file for the scoped gate)
Writes build/ehcheck.json (scoped: build/ehcheck-<base>.json; --unit: build/ehcheck-unit.json).
"""
import json
import os
import re
import struct
import sys
from bisect import bisect_right

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from datacheck import Elf, R_MIPS_32, STT_FUNC, STT_SECTION, read_units  # noqa: E402
import universe                                                           # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SECS = ('.eh_frame', '.gcc_except_table')
VERBOSE = '--verbose' in sys.argv
# statistics that need no action: only with VERBOSE=1 (environment) or -v/--verbose
STATS = VERBOSE or '-v' in sys.argv or os.environ.get('VERBOSE', '') not in ('', '0')
# block states as printed: only `fail` is shown differently
STATE_EN = {'fail': 'DIFF'}
# field names of the error lines as printed
KEY_EN = {'eq': 'equal', 'bad': 'differ', 'first_bad': 'first_diff'}


def parse_frames(b):
    """[(off, len_tot, is_cie, cie_off, pc_off)]: pc_off = offset of the pc_begin (FDE)."""
    out, o = [], 0
    while o + 4 <= len(b):
        ln = struct.unpack_from('<I', b, o)[0]
        if ln == 0:
            out.append((o, 4, None, None, None))      # terminator (crtend.o)
            o += 4
            continue
        if o + 4 + ln > len(b) or ln < 4:
            raise ValueError(f'frame at 0x{o:x}: length {ln} outside the buffer')
        cid = struct.unpack_from('<I', b, o + 4)[0]
        if cid == 0:
            out.append((o, ln + 4, True, o, None))
        else:
            out.append((o, ln + 4, False, o + 4 - cid, o + 8))
        o += ln + 4
    return out


class Orig:
    def __init__(self):
        self.elf = Elf(os.path.join(ROOT, 'orig/bn-jpn.elf'))
        self.sec = {n: self.elf.by_name[n] for n in SECS}
        self.raw = {n: self.elf.bytes_of(self.sec[n]) for n in SECS}
        self.frames = parse_frames(self.raw['.eh_frame'])
        # pc_begin -> [FDE offset]
        self.fde_by_pc = {}
        for off, ln, cie, coff, pco in self.frames:
            if cie is False:
                pc = struct.unpack_from('<I', self.raw['.eh_frame'], pco)[0]
                self.fde_by_pc.setdefault(pc, []).append(off)
        self.funcs = universe.symbols()
        self.kind = {off: cie for off, ln, cie, coff, pco in self.frames}   # True CIE, False FDE, None term.


def our_func_map(o, src, check_json, orig):
    """section -> [(start, end, orig_addr)] of the functions of our .o with a known address."""
    local_addr = {}
    for sym, r in check_json.items():
        if r.get('unit') == src and r.get('ok') and r.get('local'):
            local_addr[r['local']] = orig.funcs[sym][0]
    out = {}
    for name, value, size, typ, bind, shndx in o.symbols():
        if typ != STT_FUNC or not size or shndx >= len(o.secs):
            continue
        a = local_addr.get(name)
        if a is None and bind != 0 and name in orig.funcs:     # global/weak: address by name
            a = orig.funcs[name][0]
        if a is None:
            continue
        out.setdefault(shndx, []).append((value, value + size, a))
    for v in out.values():
        v.sort()
    return out


def translate(o, sec_name, fmap, ex_base):
    """offset -> original value (or None = masked) for the relocated words of the section."""
    syms = o.symbols()
    raw = o.bytes_of(o.by_name[sec_name])
    out = {}
    for off, symidx, typ in o.relocs(sec_name):
        if typ != R_MIPS_32:
            out[off] = None
            continue
        name, value, size, styp, bind, shndx = syms[symidx]
        add = struct.unpack_from('<I', raw, off)[0]
        x = value + add if styp != STT_SECTION else add
        tgt = o.secs[shndx]['nm'] if shndx < len(o.secs) else None
        if tgt == '.gcc_except_table':
            out[off] = ('ex', x)
            continue
        funcs = fmap.get(shndx, [])
        i = bisect_right(funcs, (x, 1 << 40, 0)) - 1
        val = None
        # a label may fall on the end of the function (LEHE of the last region)
        for j in (i, i - 1):
            if 0 <= j < len(funcs) and funcs[j][0] <= x <= funcs[j][1]:
                val = funcs[j][2] + x - funcs[j][0]
                break
        out[off] = val
    return out


def compare(ours, theirs, rel, resolve):
    """(eq, bad, masked, first_bad): word by word, relocated words resolved."""
    eq = bad = masked = 0
    first_bad = None
    n = len(ours)
    k = 0
    while k < n:
        if k in rel:
            v = resolve(rel[k])
            if v is None:
                masked += 4
            elif struct.unpack_from('<I', theirs, k)[0] == v & 0xffffffff:
                eq += 4
            else:
                bad += 4
                first_bad = k if first_bad is None else first_bad
            k += 4
            continue
        if ours[k] == theirs[k]:
            eq += 1
        else:
            bad += 1
            first_bad = k if first_bad is None else first_bad
        k += 1
    return eq, bad, masked, first_bad


def do_unit(unit, orig, check_json):
    src, flags, asflags, skip = unit
    base = re.sub(r'[^A-Za-z0-9_]', '_', src)
    obj = os.path.join(ROOT, f'build/check/{base}/out.o')
    if not os.path.exists(obj):
        return None
    o = Elf(obj)
    have = {s: o.by_name[s] for s in SECS if s in o.by_name and o.by_name[s]['size']}
    if not have:
        return {}
    fmap = our_func_map(o, src, check_json, orig)
    res = {}
    skips = set(skip or ())
    eh = have.get('.eh_frame')
    ex_addr = None
    if eh:
        ours = o.bytes_of(eh)
        rel = translate(o, '.eh_frame', fmap, None)
        try:
            frames = parse_frames(ours)
        except ValueError as e:
            res['.eh_frame'] = {'size': len(ours), 'skip': 'data:.eh_frame' in skips,
                                'state': 'malformed', 'error': str(e)}
            frames = None
    if eh and frames is not None:
        # anchors: translated pc_begin -> FDE offset in the original
        deltas = {}
        n_fde = 0
        for off, ln, cie, coff, pco in frames:
            if cie is False:
                n_fde += 1
                v = rel.get(pco)
                if isinstance(v, int):
                    for ooff in orig.fde_by_pc.get(v, []):
                        deltas.setdefault(ooff - off, []).append(v)
        r = {'size': len(ours), 'fde': n_fde, 'skip': 'data:.eh_frame' in skips,
             'pcs': [rel.get(pco) for off, ln, cie, coff, pco in frames
                     if cie is False and isinstance(rel.get(pco), int)]}
        if not frames or all(cie is None for off, ln, cie, coff, pco in frames):
            # only the terminator (crtend.o, ref/gcc-2.95.2/crtstuff_end.c): it sits at
            # the end of the original section, after the last frame
            term = [off for off, ln, cie, coff, pco in orig.frames if cie is None]
            if term and term[-1] + len(ours) == len(orig.raw['.eh_frame']):
                deltas = {term[-1]: [None]}
        if not deltas:
            r['state'] = 'unanchored'
        else:
            best = max(deltas, key=lambda d: len(deltas[d]))
            r['anchors'] = len(deltas[best])
            if len(deltas) > 1:
                r['conflict'] = {str(d): len(v) for d, v in deltas.items()}
            start = best
            theirs = orig.raw['.eh_frame'][start:start + len(ours)] if start >= 0 else b''
            if len(theirs) < len(ours):
                r['state'] = 'out-of-section'
            else:
                # .o boundaries: in the original the block must open on a CIE and close
                # before a CIE or the terminator, otherwise the TU differs
                if orig.kind.get(start) is not (True if n_fde else None):
                    r['boundary'] = 'start'
                elif orig.kind.get(start + len(ours), True) is False:
                    r['boundary'] = 'end'
                # "eh" pointer of the CIE: the original says where our table is,
                # but only if the original CIE has the "eh" augmentation too
                for off, ln, cie, coff, pco in frames:
                    if (cie and isinstance(rel.get(off + 12), tuple)
                            and orig.kind.get(start + off) is True
                            and theirs[off + 9:off + 12] == b'eh\0'):
                        exw = struct.unpack_from('<I', theirs, off + 12)[0]
                        ex_addr = exw - rel[off + 12][1]
                        break

                # the "eh" word is read from the original itself (ex_addr): comparing
                # it would be tautological, so it stays MASKED and becomes equal only
                # if the table it points to is equal byte by byte (below)
                eq, bad, masked, fb = compare(ours, theirs, rel,
                                              lambda v: None if isinstance(v, tuple) else v)
                r.update({'addr': orig.sec['.eh_frame']['addr'] + start, 'eq': eq, 'bad': bad,
                          'masked': masked, 'first_bad': fb,
                          'state': 'ok' if bad == 0 and 'boundary' not in r else 'fail'})
        res['.eh_frame'] = r
    ex = have.get('.gcc_except_table')
    if ex:
        ours = o.bytes_of(ex)
        rel = translate(o, '.gcc_except_table', fmap, None)
        r = {'size': len(ours), 'skip': 'data:.gcc_except_table' in skips}
        sa = orig.sec['.gcc_except_table']['addr']
        if ex_addr is None:
            r['state'] = 'unanchored'
        elif not (sa <= ex_addr and ex_addr + len(ours) <= sa + orig.sec['.gcc_except_table']['size']):
            r['state'] = 'out-of-section'
            r['addr'] = ex_addr
        else:
            theirs = orig.raw['.gcc_except_table'][ex_addr - sa:ex_addr - sa + len(ours)]
            eq, bad, masked, fb = compare(ours, theirs, rel,
                                          lambda v: None if isinstance(v, tuple) else v)
            r.update({'addr': ex_addr, 'eq': eq, 'bad': bad, 'masked': masked,
                      'first_bad': fb, 'state': 'ok' if bad == 0 else 'fail'})
            e = res.get('.eh_frame')
            if r['state'] == 'ok' and e and e.get('masked', 0) >= 4:
                e['eq'] += 4
                e['masked'] -= 4
        res['.gcc_except_table'] = r
    return res


def union(iv):
    u, prev = 0, None
    for a, b in sorted(iv):
        if prev is not None and a < prev:
            a = prev
        if b > a:
            u += b - a
            prev = b
    return u


def main():
    os.chdir(ROOT)          # universe.symbols() reads config/ with relative paths
    orig = Orig()
    check_json = json.load(open(os.path.join(ROOT, 'build/check.json')))
    # scoped gate (UNITS=file): as in datacheck, the freshly matched functions are in
    # the scoped check build/check-<base>.json, and the full gate's JSON is not touched
    units_file = os.environ.get('UNITS', 'config/units.txt')
    scoped = units_file != 'config/units.txt'
    if scoped:
        sbase = re.sub(r'[^A-Za-z0-9_.-]', '_', os.path.basename(units_file).rsplit('.', 1)[0])
        sj = os.path.join(ROOT, f'build/check-{sbase}.json')
        if os.path.exists(sj):
            check_json.update(json.load(open(sj)))
    units = read_units()
    only = sys.argv[sys.argv.index('--unit') + 1] if '--unit' in sys.argv else None
    if only:
        units = [u for u in units if u[0] == only]
    out = {'orig': {n: orig.sec[n]['size'] for n in SECS}, 'units': {}}
    for u in units:
        res = do_unit(u, orig, check_json)
        if res:
            out['units'][u[0]] = res
    failed = []
    states = {}
    good = {n: [] for n in SECS}
    for u, res in sorted(out['units'].items()):
        for sec, r in res.items():
            states[(sec, r['state'])] = states.get((sec, r['state']), 0) + 1
            if r['state'] == 'ok':
                good[sec].append((r['addr'], r['addr'] + r['size'] - r.get('masked', 0)))
            bad = r['state'] in ('fail', 'out-of-section', 'unanchored', 'malformed') \
                and r['size'] > 4 and not r['skip']
            if bad:
                failed.append((u, sec))
            if bad and len(failed) > 20 and not STATS:
                continue
            if bad or (STATS and r['state'] != 'ok') or VERBOSE:
                st = STATE_EN.get(r['state'], r['state']) + (' (skip)' if r['skip'] else '')
                print(f'{st:15s} {sec:18s} {u}  ' + ', '.join(
                    f'{KEY_EN.get(k, k)}={v}'
                    for k, v in r.items() if k not in ('state', 'skip', 'pcs')))
    # orphan = original FDE whose pc_begin no FDE of ours (of any unit) translates:
    # coverage by position would hide the FDEs under a misplaced DIFF block
    anchored = {pc for res in out['units'].values()
                for pc in res.get('.eh_frame', {}).get('pcs', [])}
    orphan = [off for off, ln, cie, coff, pco in orig.frames
              if cie is False
              and struct.unpack_from('<I', orig.raw['.eh_frame'], pco)[0] not in anchored]
    # original FDEs no block of ours covers. They are credited to the unit the
    # function gate credited the pc_begin's function to (for the deduplicated STL
    # copies that credit is by content: indicative). Aliases at the same address:
    # the name the function gate knows wins
    by_addr = {}
    for s_, (a_, n_) in orig.funcs.items():
        if s_ in check_json or a_ not in by_addr:
            by_addr[a_] = s_
    orph_units = {}
    for off in orphan:
        pco = off + 8
        pc = struct.unpack_from('<I', orig.raw['.eh_frame'], pco)[0]
        sym = by_addr.get(pc)
        u = check_json.get(sym, {}).get('unit', '?') if sym else '?'
        orph_units.setdefault(u, []).append(sym or hex(pc))
    out['orphan_fdes'] = {u: v for u, v in sorted(orph_units.items())}
    if not only and not scoped:
        if STATS or orphan:
            print(f'{"warning: " if orphan else ""}{len(orphan)} FDEs of the original have a pc_begin '
                  f'that none of our FDEs translates')
        for u, v in sorted(orph_units.items(), key=lambda kv: -len(kv[1]))[:20]:
            print(f'     {len(v):3d}  {u}')
    dest = ('build/ehcheck-unit.json' if only else
            f'build/ehcheck-{sbase}.json' if scoped else 'build/ehcheck.json')
    json.dump(out, open(os.path.join(ROOT, dest), 'w'), indent=1)
    if len(failed) > 20 and not STATS:
        print(f'... and {len(failed) - 20} more, see {dest}')
    nok = {sec: sum(n for (s, st), n in states.items() if s == sec and st == 'ok') for sec in SECS}
    eq_b = sum(union(good[sec]) for sec in SECS)
    tot_b = sum(out['orig'][sec] for sec in SECS)
    print(f'eh         ' + ', '.join(f'{sec} {nok[sec]} blocks' for sec in SECS)
          + (f' match ({eq_b:,} B equal)' if only or scoped else f' match ({eq_b:,}/{tot_b:,} B equal)')
          + (f', {len(failed)} differ' if failed else '')
          + (f' (scoped: {dest})' if only or scoped else ''))
    if STATS:
        print('blocks by state: ' + ', '.join(f'{s} {STATE_EN.get(st, st)} {n}'
                                              for (s, st), n in sorted(states.items())))
        print(f'   -> {dest}')
    sys.exit(1 if failed else 0)


if __name__ == '__main__':
    main()
