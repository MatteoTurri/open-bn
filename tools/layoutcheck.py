#!/usr/bin/env python3
"""layoutcheck.py -- is the .text of each of our .o files ready for the link? (measurement, not a gate)

The function gate places each function on its own (by name, by anchors, by content), so it
does not see whether one of our .o files, laid by the linker at a base address, would
reproduce the original addresses. For each unit of config/units.txt this takes the functions
of the `.text` section of our .o (build/check/<unit>/out.o) in .o order and the original
address the gate credited to each (build/check.json: by name if credited to this unit, locals
via the `local` field), and measures (json keys):
  unmapped   bodies without an address in the original (the gate's "out of alignment")
  dup        named functions credited to another unit (at link time: duplicate definitions)
  binding    LOCAL in our .o, global (.dynsym) in the original
  breaks     the delta (original address - our offset) changes between consecutive functions
  inversions ... and the original address goes down (function order differs)
  holes      original functions inside the unit's range that we do not put there
  own_linkonce_holes  the same, for our own linkonce functions
  misaligned the base is not a multiple of our .text alignment (16: a MIPS gas .o cannot
             start there -- a TU boundary inside the unit)
  in_linkonce_region  the base falls in the original's .gnu.linkonce.t region (after crtstuff_end)
A unit is ready if it has none of these (except binding). The .gnu.linkonce.t.* sections are
left out: in the original they all sit at the end of .text.

Limits: for a single-function unit the test is vacuous except holes/alignment; `holes` as a
number means little (two distant clusters), it is a signal. In scoped mode a global record of
a scoped unit does not survive (a local gone out of place, renamed, a unit that no longer
compiles); a named function that the full gate credits to a unit outside the scope is `dup`
in the scoped run too; --unit refuses a unit missing from the units file. The assignment of
the static functions in a scoped run sees only the scoped units, so the full gate is the
reference.

    python3 tools/layoutcheck.py                  # all units: summary, build/layoutcheck.json
    python3 tools/layoutcheck.py --unit SRC       # one unit: detail, exit 0 if READY
    UNITS=file python3 tools/layoutcheck.py ...   # after a scoped gate: also reads
                                                  # build/check-<base>.json (like datacheck)
    LAYOUT_OVERLAP=1 python3 tools/layoutcheck.py [--unit SRC]   # + overlaps between units

Overlaps between units (LAYOUT_OVERLAP=1, off by default).
Everything above looks at one unit at a time: two ready units may not fit together. Example:
a unit at base 0x5e4a40 with .text 0x160 ends at 0x5e4ba0; a second unit at base 0x5e4b90 is
ready on its own, but in the link the two overlap by 16 B and the whole .text after them shifts
by +16 (the two are really one translation unit).
The rule is ld's with the default script: for each output section the input sections of that
name are laid in .o order, each at align(end of the previous one, its sh_addralign). The .o
order is the original's (.text base; data-only units inserted after the anchored block that
precedes them, as in tools/relink/link_order.py). Each section is walked in that order; where a
unit has a known address in the original (.text base; a block located by datacheck/ehcheck,
state other than `unanchored`/`conflict`) it is compared with the expected position:
  OVERLAP          original < expected: the previous .o files (since the last known
                   address) are too long to fit: TU boundary, padding, extra literals or objects
  HOLE             original > expected: bytes our .o files lack (DNAS/asset blobs, orphan FDEs,
                   units without a base) or a .o that is too short
and the walk restarts from the original's address. Blocks without a known address are laid
where they fall (`floating`: their weight falls on the next comparison and they are
listed with the pair). The .gnu.linkonce.* sections are left out (they go at the end of the
output section). Measurement, not a gate: it changes neither the exit code nor
build/layoutcheck.json; it writes build/layoutcheck_overlap[-<base>].json.
In a scoped run the units outside the scope take base and size from build/layoutcheck.json and
data addresses from build/datacheck.json/build/ehcheck.json (the scoped ones from
build/{datacheck,ehcheck}-<base>.json), the .o files always from build/check/<unit>/out.o.
"""
import bisect
import collections
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)
sys.path.insert(0, os.path.join(ROOT, 'tools'))
from datacheck import Elf, STT_FUNC                               # noqa: E402
import universe                                                   # noqa: E402

UNITS = os.environ.get('UNITS', 'config/units.txt')
GLOBAL, OTHER = {}, set()


def load_check():
    cj = json.load(open('build/check.json'))
    if UNITS != 'config/units.txt':
        base = re.sub(r'[^A-Za-z0-9_.-]', '_', os.path.basename(UNITS).rsplit('.', 1)[0])
        sj = f'build/check-{base}.json'
        if os.path.exists(sj):
            # the credits of the scoped units are decided only by the scoped run: a global
            # record of one of their functions (local gone out of place, renamed, moved, unit
            # that no longer compiles) must not survive
            scoped = set(scoped_units())
            cj = {s: r for s, r in cj.items() if r.get('unit') not in scoped}
            cj.update(json.load(open(sj)))
        else:
            print(f'warning: {sj} missing: credits from the global check.json', file=sys.stderr)
    return cj


def scoped_units():
    return [l.split('|')[0].strip() for l in open(UNITS) if l.strip() and not l.startswith('#')]


def all_units():
    return [l.split('|')[0].strip() for l in open('config/units.txt')
            if l.strip() and not l.startswith('#') and '|' in l]


def is_ready(r):
    return (r['mapped'] and not r['unmapped'] and not r['dup'] and r['breaks'] == 0
            and not r['holes'] and not r['own_linkonce_holes'] and not r['misaligned']
            and not r['in_linkonce_region'])


def analyze(units, cj, U):
    loc = collections.defaultdict(dict)
    for s, r in cj.items():
        if r.get('ok') and r.get('local') and s in U:
            loc[r['unit']][r['local']] = (U[s][0], s)
    oaddrs = sorted((a, n, sz) for n, (a, sz) in U.items() if sz)
    starts = [a for a, n, sz in oaddrs]
    crtend = max(U[s][0] for s, r in json.load(open('build/check.json')).items()
                 if r.get('unit') == 'ref/gcc-2.95.2/crtstuff_end.c' and s in U)
    rep, detail = [], {}
    for src in units:
        p = 'build/check/%s/out.o' % re.sub(r'[^A-Za-z0-9_]', '_', src)
        if not os.path.exists(p):
            continue
        o = Elf(p)
        syms = o.symbols()
        own_lk = {U[n][0] for n, v, sz, t, b, sh in syms
                  if t == STT_FUNC and sz and 0 < sh < 0xff00
                  and o.secs[sh]['nm'].startswith('.gnu.linkonce.t')
                  and cj.get(n, {}).get('unit') == src and n in U}
        ti = [i for i, s in enumerate(o.secs) if s['nm'] == '.text']
        if not ti:
            continue
        ti = ti[0]
        sec = o.secs[ti]
        fs = sorted((v, sz, n, b) for n, v, sz, t, b, sh in syms
                    if t == STT_FUNC and sh == ti and sz)
        if not fs:
            continue
        seq, dup, binding = [], [], []
        for v, sz, n, b in fs:
            a = None
            if n in loc[src]:
                a = loc[src][n][0]
            elif n in U and cj.get(n, {}).get('ok'):
                g = GLOBAL.get(n, {}).get('unit')
                if cj[n]['unit'] == src and g not in OTHER:
                    a = U[n][0]
                    if b == 0:
                        binding.append(n)
                else:
                    dup.append((n, cj[n]['unit'] if cj[n]['unit'] != src else g))
            seq.append((v, sz, n, a))
        dupn = {n for n, _ in dup}
        mapped = [(v, a) for v, sz, n, a in seq if a is not None]
        unm = [(n, sz) for v, sz, n, a in seq if a is None and n not in dupn]
        breaks = sum(1 for (v1, a1), (v2, a2) in zip(mapped, mapped[1:]) if (a2 - v2) != (a1 - v1))
        inv = sum(1 for (v1, a1), (v2, a2) in zip(mapped, mapped[1:]) if a2 < a1)
        holes, holes_lk, base = [], [], None
        if mapped:
            base = mapped[0][1] - mapped[0][0]
            lo = min(a for v, a in mapped)
            hi = max(a for v, a in mapped)
            mine = {a for v, a in mapped}
            i = bisect.bisect_left(starts, lo)
            while i < len(oaddrs) and oaddrs[i][0] <= hi:
                if oaddrs[i][0] not in mine:
                    (holes_lk if oaddrs[i][0] in own_lk else holes).append(oaddrs[i][1])
                i += 1
        rep.append({'src': src, 'nfun': len(seq), 'mapped': len(mapped), 'unmapped': unm,
                    'dup': dup, 'binding': binding, 'breaks': breaks, 'inversions': inv,
                    'holes': holes, 'own_linkonce_holes': holes_lk, 'base': base,
                    'align': sec['align'],
                    'misaligned': base is not None and base % sec['align'] != 0,
                    'in_linkonce_region': base is not None and base > crtend,
                    'text': sec['size']})
        detail[src] = seq
    return rep, detail


def print_unit(r, seq, U):
    print(f'== {r["src"]}: {"READY" if is_ready(r) else "NOT ready"} -- {r["nfun"]} functions, '
          f'.text {r["text"]} B, base {hex(r["base"]) if r["base"] is not None else "?"}')
    for k in ('unmapped', 'dup', 'binding', 'holes', 'own_linkonce_holes'):
        if r[k]:
            print(f'   {k}: {r[k][:12]}{" ..." if len(r[k]) > 12 else ""}')
    for k in ('breaks', 'inversions', 'misaligned', 'in_linkonce_region'):
        if r[k]:
            print(f'   {k}: {r[k]}')
    mapped = sorted(a for v, sz, n, a in seq if a is not None)
    rank = {a: i for i, a in enumerate(mapped)}
    print('   our order (offset in the .o) -> position and address in the original:')
    for v, sz, n, a in seq:
        pos = f'#{rank[a]:3d} {a:#x}' if a is not None else '   ?        '
        print(f'     {v:#7x} {sz:6d}  {pos}  {n}')


OVL_SECS = ['.text', '.rodata', '.data', '.eh_frame', '.gcc_except_table', '.ctors', '.dtors', '.bss']


def scope_base():
    return re.sub(r'[^A-Za-z0-9_.-]', '_', os.path.basename(UNITS).rsplit('.', 1)[0])


def load_units_json(name):
    """build/<name>.json['units'], with the scoped units taken from the scoped run."""
    p = f'build/{name}.json'
    d = json.load(open(p))['units'] if os.path.exists(p) else {}
    if UNITS != 'config/units.txt':
        sp = f'build/{name}-{scope_base()}.json'
        if os.path.exists(sp):
            for u in scoped_units():
                d.pop(u, None)
            d.update(json.load(open(sp))['units'])
        else:
            print(f'warning: {sp} missing: data addresses from {p}', file=sys.stderr)
    return d


def overlap(rep, only=None):
    """Overlaps and holes between neighbouring units (module docstring, LAYOUT_OVERLAP=1)."""
    units = all_units()
    known = set(units)
    # .text base and size: rep (this run) over build/layoutcheck.json
    lc = {}
    if os.path.exists('build/layoutcheck.json'):  # the units this run did not analyze
        lc = {r['src']: r for r in json.load(open('build/layoutcheck.json')) if r['src'] in known}
    # Order key: the base of this run or, if this run does not find it (scoped run with
    # stale credits, a unit that does not compile), the full run's; the .o stays in the
    # walk (floating) instead of vanishing and leaving a hole as large as its .text
    okey = {u: r.get('base') for u, r in lc.items()}
    lc.update({r['src']: r for r in rep})
    for r in rep:
        if r.get('base') is not None or okey.get(r['src']) is None:
            okey[r['src']] = r.get('base')
    dc, eh = load_units_json('datacheck'), load_units_json('ehcheck')

    def objp(u):
        return 'build/check/%s/out.o' % re.sub(r'[^A-Za-z0-9_]', '_', u)
    secs = {}
    for u in units:
        if os.path.exists(objp(u)):
            o = Elf(objp(u))
            secs[u] = {s['nm']: (s['size'], max(1, s['align'])) for s in o.secs if s['flags'] & 2}
    orig = Elf('orig/bn-jpn.elf')
    orng = {s['nm']: (s['addr'], s['addr'] + s['size']) for s in orig.secs}
    # non-compiled bytes (config/data_regions.txt, except crt, which are not units): in the
    # link they are blobs; in the walk they take their place, so a hole that is only the region
    # disappears and an overlap next to a region does not stay hidden in the hole
    import dataregions
    regs = [(a, b, n) for a, b, c, n in dataregions.regions() if c != 'crt']
    # .text: the three DNAS data tables (OBJECT in .dynsym, size 0: their end is the base of the
    # next unit) -- their start is checked, which the end of the previous unit must reach exactly
    for l in open('config/symbol_addrs.txt'):
        m = re.match(r'(_sce_dnas_pwrap_info_tbl|__sce_dnas2_symbol1[12]001) = (0x[0-9A-Fa-f]+);', l)
        if m:
            regs.append((int(m.group(2), 16), None, m.group(1)))
    stale = []

    # known address in the original per (unit, section)
    exp = {}
    for u in units:
        if lc.get(u, {}).get('base') is not None:
            exp[(u, '.text')] = lc[u]['base']
        for src in (dc.get(u, {}), eh.get(u, {})):
            for s, b in src.items():
                if s not in OVL_SECS or b.get('addr') is None or b.get('state') in (
                        'unanchored', 'conflict', 'out-of-section', 'malformed'):
                    continue
                if s == '.bss' and b.get('state') != 'bss':
                    continue
                if not orng[s][0] <= b['addr'] < orng[s][1]:
                    continue  # as link_order.py for .bss: some of our .bss are .data in the original
                if u in secs and b.get('size') is not None and secs[u].get(s, (None,))[0] != b['size']:
                    stale.append(f'{u} {s}')  # json and .o from different runs
                exp[(u, s)] = b['addr']

    # .o order: .text base; data-only units after the preceding anchored block
    order = [u for _, u in sorted((okey[u], u) for u in units if okey.get(u) is not None)]
    dataonly = [u for u in units if u in secs and u not in lc]
    placed = {}
    for u in dataonly:
        for s in ('.rodata', '.data'):
            if (u, s) in exp:
                a = exp[(u, s)]
                prev = max([(exp[(v, s)], v) for v in order if (v, s) in exp and exp[(v, s)] < a],
                           default=(None, None))[1]
                placed[u] = (a, prev)
                break
    for u, (a, prev) in sorted(placed.items(), key=lambda x: x[1][0]):
        i = order.index(prev) + 1 if prev else 0
        while i < len(order) and order[i] in placed and placed[order[i]][0] < a:
            i += 1
        order.insert(i, u)

    def event(ev, a, e, prev, u, floating):
        ev.append({'type': 'OVERLAP' if e < a else 'HOLE', 'expected': a,
                   'orig': e, 'delta': e - a, 'prev': prev, 'unit': u, 'floating': floating})

    out = {}
    for s in OVL_SECS:
        cur, prev, floating, ev, lastk = None, None, [], [], None
        sregs = [r for r in regs if orng[s][0] <= r[0] < orng[s][1]]
        for u in order:
            if s not in secs.get(u, {}):
                continue
            size, al = secs[u][s]
            if not size:
                continue
            e = exp.get((u, s))
            if e is not None and cur is not None:
                # non-compiled regions between the expected end and this block: laid at their
                # address (a blob starts 16-aligned, like every gas 2.9EE .o)
                for ra, rb, rn in sregs:
                    if lastk < ra and (rb or ra) <= e:
                        rb = rb or e
                        # unaligned start = blob at the tail of the previous .o: the multiple of 16 counts
                        if (ra + 15) // 16 * 16 != (cur + 15) // 16 * 16:
                            event(ev, (cur + 15) // 16 * 16, ra, prev, f'[{rn}]', floating)
                        cur, prev, floating = rb, f'[{rn}]', []
            a = None if cur is None else (cur + al - 1) // al * al
            if e is not None:
                # block starting unaligned: in the original the .o started earlier, with the
                # bytes of a non-compiled region at its head (sound.c, dnas_pwrap.c,
                # dnas2m12_identity.c): the comparison is on the exact end of the region, and the
                # padding tail of our .o is already inside the same multiple of 16
                if e % al and cur is not None:
                    a = cur
                if a is not None and e != a:
                    event(ev, a, e, prev, u, floating)
                a, floating, lastk = e, [], e
                cur = e - e % al + size
            else:
                if a is not None:
                    floating = floating + [u]
                    cur = a + size
            prev = u
        out[s] = ev
    lost = [u for u in units if secs.get(u, {}).get('.text', (0,))[0] and u not in order]
    if lost:
        print(f'warning: {len(lost)} units with .text but no base, outside the path (each of their sections '
              f'becomes a hole or covers an overlap): {", ".join(lost[:6])}{" ..." if len(lost) > 6 else ""}')
    if stale:
        print(f'warning: {len(stale)} blocks with a different size between json and .o (different runs): '
              f'{", ".join(stale[:6])}{" ..." if len(stale) > 6 else ""}')
    if only:
        out = {s: [x for x in ev if only in [x['prev'], x['unit']] + x['floating']]
               for s, ev in out.items()}
    print('== overlaps between units (LAYOUT_OVERLAP=1, measurement) ==')
    for s, ev in out.items():
        sv = [x for x in ev if x['type'] == 'OVERLAP']
        bu = [x for x in ev if x['type'] == 'HOLE']
        print(f'  {s:18s} overlaps {len(sv):3d} ({-sum(x["delta"] for x in sv):7d} B)  '
              f'holes {len(bu):3d} ({sum(x["delta"] for x in bu):8d} B)')
        for x in sv + bu:
            fl = f'  floating: {", ".join(x["floating"])}' if x['floating'] else ''
            print(f'    {x["type"]:15s} 0x{x["orig"]:08x} {x["delta"]:+8d} B  {x["prev"]} / {x["unit"]}{fl}')
    if not only:
        name = ('build/layoutcheck_overlap.json' if UNITS == 'config/units.txt'
                else f'build/layoutcheck_overlap-{scope_base()}.json')
        json.dump({'order': order, 'events': out}, open(name, 'w'), indent=1)
        print(f'   -> {name}')


def safe_overlap(rep, only=None):
    try:  # measurement: an error here (a .o rewritten while being read) does not change the exit code
        overlap(rep, only)
    except Exception as x:
        print(f'warning: LAYOUT_OVERLAP: measurement failed ({type(x).__name__}: {x})', file=sys.stderr)


def main():
    U = universe.symbols()
    cj = load_check()
    # scoped run: a named function that the full gate credits to a unit outside the scope is
    # a duplicate definition even if the scoped run, which does not see the other one, gives it
    # to this unit (if it is a real move, the other unit must be put in the scope)
    if UNITS != 'config/units.txt':
        GLOBAL.update(json.load(open('build/check.json')))
        OTHER.update(set(all_units()) - set(scoped_units()))
    only = sys.argv[sys.argv.index('--unit') + 1] if '--unit' in sys.argv else None
    if only and only not in (all_units() if UNITS == 'config/units.txt' else scoped_units()):
        sys.exit(f'error: {only}: not among the units of {UNITS}')
    units = [only] if only else (all_units() if UNITS == 'config/units.txt' else
                                 [l.split('|')[0].strip() for l in open(UNITS)
                                  if l.strip() and not l.startswith('#')])
    rep, detail = analyze(units, cj, U)
    if only:
        if not rep:
            sys.exit(f'error: {only}: no .text (or .o missing)')
        print_unit(rep[0], detail[only], U)
        if os.environ.get('LAYOUT_OVERLAP') == '1':
            safe_overlap(rep, only)
        sys.exit(0 if is_ready(rep[0]) else 1)
    if UNITS == 'config/units.txt':
        json.dump(rep, open('build/layoutcheck.json', 'w'), indent=1)
    ok = [r for r in rep if is_ready(r)]
    print(f'units with .text: {len(rep)}  ready: {len(ok)} ({sum(r["text"] for r in ok):,} B '
          f'of {sum(r["text"] for r in rep):,})')
    for k in ('unmapped', 'dup', 'binding', 'breaks', 'holes', 'own_linkonce_holes',
              'inversions', 'misaligned', 'in_linkonce_region'):
        print(f'  with {k}: {sum(1 for r in rep if r[k])} units')
    if UNITS != 'config/units.txt':
        for r in rep:
            print(f'  {"READY" if is_ready(r) else "no   "} {r["src"]}')
    if os.environ.get('LAYOUT_OVERLAP') == '1':
        safe_overlap(rep)


if __name__ == '__main__':
    main()
