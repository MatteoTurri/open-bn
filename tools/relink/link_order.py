#!/usr/bin/env python3
"""Link order of the .o files: from the original's addresses to link_order.txt.

Usage: python3 tools/relink/link_order.py   (repository root; called by `make link`)

Runs after a full `make check` and `python3 tools/layoutcheck.py`: reads
build/layoutcheck.json (.text base of each unit in the original), build/datacheck.json and
build/ehcheck.json (address of each unit's data / exception blocks), and the gate's .o
files (build/check/<unit>/out.o). The order is that of the .text addresses; data-only
units are inserted after the unit with the preceding data block.

First it checks the inputs (check_inputs: gate .o files newer than the json files ->
exit 1, unless RELINK_ALLOW_STALE=1) and writes build/relink/inputs.sha1.

Writes:
  build/relink/link_order.txt   one .o per line (crt + units + blobs), `#` comments
  build/relink/order_report.txt consistency of the order with the other sections,
                                exceptions, placement simulation (ld style)
  build/relink/order.json       the same in script-readable form (blobs, holes)

The simulated placement is that of ld with the default script: for each output section,
the input sections of that name in file order, each aligned to its sh_addralign,
starting at the original's address. Linkonce sections (.gnu.linkonce.*) go at the end of
the output section, one copy per name (the first in link order). A hole between the
expected end of a unit and the start of the next unit in the original is a piece our .o
files lack (assets, DNAS tables, Chloe, VU, crt): it becomes a blob to put back at that
point of the order (tools/relink/make_blobs.py). Blobs are only described here
(addresses and size); make_blobs.py extracts the bytes from the user's ELF into build/.
"""
import json, os, re, sys
from elftools.elf.elffile import ELFFile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from paths import OUT as HERE, UNITS, KIT_USRLIB as KIT, GCCLIB, BLOBS, MERGED, INPUTS_SHA1, unit_obj  # noqa: E402

# the kit's crt1.o as is, with its .mdebug (see the note in tools/relink/link.sh)
CRT_HEAD = [f'{KIT}/crt1.o', f'{KIT}/crti.o', f'{GCCLIB}/crtbegin.o']
CRT_TAIL = [f'{GCCLIB}/crtend.o', f'{KIT}/crtn.o']
# our two crtstuff units: crtstuff.c = .text of crti.o (call_gmon_start, 64 B)
# + crtbegin.o; crtstuff_end.c = crtend.o (identical bytes). The link uses the kit's
# objects, not ours (crtstuff.c would duplicate call_gmon_start).
CRT_UNITS = {'ref/gcc-2.95.2/crtstuff.c': 'crtbegin', 'ref/gcc-2.95.2/crtstuff_end.c': 'crtend'}

ORIG_START = {  # readelf -SW orig/bn-jpn.elf
    '.init': 0x47c168, '.text': 0x47c1d0, '.fini': 0x93beb8, '.rodata': 0x93bf40,
    '.data': 0x10000000, '.eh_frame': 0x100cb85c, '.gcc_except_table': 0x100d0cfc,
    '.ctors': 0x100d17a0, '.dtors': 0x100d18ac, '.bss': 0x100db400,
}
ORIG_END = {
    '.init': 0x47c1cc, '.text': 0x93beb8, '.fini': 0x93bf40, '.rodata': 0x978f44,
    '.data': 0x100cb858, '.eh_frame': 0x100d0cfc, '.gcc_except_table': 0x100d17a0,
    '.ctors': 0x100d18ac, '.dtors': 0x100d1914, '.bss': 0x102b146c,
}
LINKONCE = {'.text': '.gnu.linkonce.t.', '.rodata': '.gnu.linkonce.r.', '.data': '.gnu.linkonce.d.'}
# .text: the linker stubs (.MIPS.stubs) come after the last *(.text), before the
# linkonce sections (0x911690..0x912830 stubs, 0x912830..0x912840 zeros, first linkonce 0x912840)
STUBS = (0x911690, 4528)  # 282 stubs of 16 B + 16 zero bytes (the empty last entry): linkonce starts at 0x912840


def read_units():
    return [l.split('|')[0].strip() for l in open(UNITS)
            if l.strip() and not l.startswith('#')]


GATE_JSON = ('build/check.json', 'build/layoutcheck.json')


def check_inputs(paths):
    """Input consistency. (a) No gate .o (build/check/) may be newer than build/check.json or
    build/layoutcheck.json: the order and the blobs are derived from those json files, and a .o
    recompiled later (tools/match, a concurrent make check) may no longer match them. Exits 1
    with the list; RELINK_ALLOW_STALE=1 downgrades it to a warning (for measuring only, never for
    accepting). (b) Writes the sha1 of all the input .o files to build/relink/inputs.sha1
    (sha1sum format): link.sh re-checks them after ld and fails if they changed midway."""
    import hashlib
    ref = min(os.path.getmtime(j) for j in GATE_JSON)
    stale = [p for p in paths if p.startswith('build/check/') and os.path.getmtime(p) > ref]
    if stale:
        msg = (f'{len(stale)} gate .o files newer than {" / ".join(GATE_JSON)}: run a full `make check` '
               'before `make link`.\n  ' + '\n  '.join(stale[:20])
               + (f'\n  ... and {len(stale) - 20} more' if len(stale) > 20 else ''))
        if os.environ.get('RELINK_ALLOW_STALE') != '1':
            sys.exit('error: ' + msg)
        print('warning: RELINK_ALLOW_STALE=1, continuing: ' + msg, file=sys.stderr)
    with open(INPUTS_SHA1, 'w') as f:
        for p in paths:
            f.write(f'{hashlib.sha1(open(p, "rb").read()).hexdigest()}  {p}\n')


def obj_sections(path):
    """{name: (size, align)} of the allocated sections."""
    e = ELFFile(open(path, 'rb'))
    out = {}
    for s in e.iter_sections():
        if s['sh_flags'] & 2:
            out[s.name] = (s['sh_size'], max(1, s['sh_addralign']))
    return out


def align(a, n):
    return (a + n - 1) // n * n


def lis_keep(seq):
    """seq = [(key, value)] already in link order; returns the set of keys of the
    longest increasing subsequence by value (the others are the order exceptions)."""
    import bisect
    tails, tails_i, prev = [], [], [None] * len(seq)
    for i, (_, v) in enumerate(seq):
        j = bisect.bisect_left(tails, v)
        if j == len(tails):
            tails.append(v); tails_i.append(i)
        else:
            tails[j] = v; tails_i[j] = i
        prev[i] = tails_i[j - 1] if j else None
    keep, i = set(), (tails_i[-1] if tails_i else None)
    while i is not None:
        keep.add(seq[i][0]); i = prev[i]
    return keep


def main():
    os.makedirs(HERE, exist_ok=True)
    units = read_units()
    lc = {e['src']: e for e in json.load(open('build/layoutcheck.json'))}
    dc = json.load(open('build/datacheck.json'))['units']
    eh = json.load(open('build/ehcheck.json'))['units']
    rep = []
    P = rep.append

    # ---- 1. .text order ------------------------------------------------------
    text_units = sorted((lc[u]['base'], u) for u in units if lc.get(u, {}).get('base'))
    order = [u for _, u in text_units]
    nobase = [u for u in units if u in lc and not lc[u].get('base')]
    dataonly = [u for u in units if u not in lc]

    # address of each block per output section (non-linkonce sections only)
    def blocks(sec):
        out = {}
        for u in units:
            for s, b in dc.get(u, {}).items():
                if s == sec and b.get('addr') is not None and b.get('state') != 'unanchored':
                    # .bss: only the located blocks (state 'bss') inside the original's .bss;
                    # 'conflict' ones have disagreeing anchors, and some of our .bss are .data in the original
                    if sec == '.bss' and (b['state'] != 'bss' or not ORIG_START['.bss'] <= b['addr'] < ORIG_END['.bss']):
                        continue
                    out[u] = (b['addr'], b['state'])
            for s, b in eh.get(u, {}).items():
                if s == sec and b.get('addr') is not None:
                    out[u] = (b['addr'], b['state'])
        return out

    secs = ['.rodata', '.data', '.bss', '.ctors', '.dtors', '.eh_frame', '.gcc_except_table']
    B = {s: blocks(s) for s in secs}

    # data-only units: after the unit with the immediately preceding block,
    # in the first section that locates them (.rodata, then .data)
    placed = {}
    for u in dataonly:
        for s in ('.rodata', '.data'):
            if u in B[s]:
                a = B[s][u][0]
                cands = [(B[s][v][0], v) for v in order if v in B[s] and B[s][v][0] < a]
                prev = max(cands)[1] if cands else None
                nxt = min([(B[s][v][0], v) for v in order if v in B[s] and B[s][v][0] > a], default=(None, None))[1]
                placed[u] = (s, a, prev, nxt)
                break
    for u, (s, a, prev, nxt) in sorted(placed.items(), key=lambda x: x[1][1]):
        i = order.index(prev) + 1 if prev else 0
        # if the previous unit already has data-only units after it, stay after them
        while i < len(order) and order[i] in placed and placed[order[i]][1] < a:
            i += 1
        order.insert(i, u)

    P('# Link order: consistency with the sections of the original')
    P(f'units in config/units.txt: {len(units)}; with a .text base (layoutcheck): {len(text_units)}; '
      f'without a base : {len(nobase)}; data only: {len(dataonly)}')
    P('')
    P('## Data-only units (placed by their data)')
    for u, (s, a, prev, nxt) in sorted(placed.items(), key=lambda x: x[1][1]):
        P(f'  {u:45s} {s} 0x{a:08x}  after {prev}  (next in the original: {nxt})')
    P('')
    P('## Units without a base (left out of the trial link)')
    for u in nobase:
        P(f'  {u}')
    P('')

    # ---- 2. consistency: LIS of the addresses per section ----------------------
    rank = {u: i for i, u in enumerate(order)}
    P('## Order consistency per section (longest increasing subsequence)')
    exc = {}
    for s in secs:
        seq = sorted(((u, a) for u, (a, st) in B[s].items() if u in rank), key=lambda x: rank[x[0]])
        keep = lis_keep(seq)
        bad = [(u, a) for u, a in seq if u not in keep]
        exc[s] = bad
        P(f'{s:18s} blocks {len(seq):4d}  in order {len(keep):4d}  out of order {len(bad)}')
        for u, a in bad:
            st = B[s][u][1]
            st = 'DIFF' if st == 'fail' else st
            # where it belongs by address
            before = max([(a2, v) for v, a2 in seq if v in keep and a2 < a], default=(0, None))[1]
            P(f'    {u:50s} 0x{a:08x} [{st}]  by address it belongs after {before}')
    P('')

    # ---- 3. placement simulation -----------------------------------------------
    objs = []  # (name, path, unit or None)
    for p in CRT_HEAD:
        objs.append((os.path.basename(p), p, None))
    for u in order:
        if u in CRT_UNITS:
            continue  # replaced by the kit's objects (crtbegin at the head, crtend at the tail)
        objs.append((u, unit_obj(u), u))
    for p in CRT_TAIL:
        objs.append((os.path.basename(p), p, None))
    check_inputs([path for _, path, _ in objs])
    SEC = {name: obj_sections(path) for name, path, _ in objs}

    expected = {}  # (unit, section) -> address in the original
    for u in order:
        if lc.get(u, {}).get('base'):
            expected[(u, '.text')] = lc[u]['base']
        for s in secs:
            if u in B[s]:
                expected[(u, s)] = B[s][u][0]
    # the crt objects: known positions in the original
    expected[('crt1.o', '.text')] = 0x47c1d0
    expected[('crti.o', '.text')] = 0x47c230
    expected[('crtbegin.o', '.text')] = 0x47c270
    expected[('crtend.o', '.text')] = 0x9115f0
    expected[('crt1.o', '.rodata')] = 0x93bf40
    expected[('crt1.o', '.data')] = 0x10000000
    expected[('crtbegin.o', '.data')] = 0x10000010
    expected[('crtbegin.o', '.ctors')] = 0x100d17a0
    expected[('crtbegin.o', '.dtors')] = 0x100d18ac
    expected[('crtend.o', '.ctors')] = 0x100d18a8
    expected[('crtend.o', '.dtors')] = 0x100d1910
    expected[('crtend.o', '.eh_frame')] = 0x100d0cf8

    P('## Placement simulation (ld, default script, base = original)')
    P('For each section the input sections are laid out in link order; where the original puts')
    P('a piece of an object further than expected there is a HOLE (bytes our .o files do not')
    P('have); the simulator realigns on the original and goes on. A negative gap (original')
    P('earlier than expected) is an overlap: two .o files that cannot sit like that in the original.')
    holes, neg = {}, {}
    sim_addr = {}
    for s in ['.text', '.rodata', '.data', '.eh_frame', '.gcc_except_table', '.ctors', '.dtors', '.bss', '.init', '.fini']:
        cur = ORIG_START[s]
        holes[s], neg[s] = [], []
        prev = None
        for name, path, u in objs:
            if s not in SEC[name]:
                continue
            size, al = SEC[name][s]
            a = align(cur, al)
            key = (u or name, s)
            if key in expected and size:
                e = expected[key]
                if e > a:
                    holes[s].append((a, e - a, prev, u or name))
                    a = e
                elif e < a:
                    neg[s].append((e, a - e, prev, u or name))
                    a = e
            sim_addr[key] = a
            if size:
                prev = u or name
            cur = a + size
        if s == '.text':
            # end of the .o files (before the stubs)
            if cur != STUBS[0]:
                holes[s].append((cur, STUBS[0] - cur, prev, '.MIPS.stubs'))
            cur = STUBS[0] + STUBS[1]
        # linkonce: one copy per name, the first in link order
        lo = LINKONCE.get(s)
        nlo = 0
        if lo:
            seen = set()
            for name, path, u in objs:
                for sn, (size, al) in SEC[name].items():
                    if sn.startswith(lo) and sn not in seen:
                        seen.add(sn)
                        cur = align(cur, al) + size
                        nlo += 1
        tail = ORIG_END[s] - cur
        P(f'{s:18s} holes {len(holes[s]):3d} ({sum(h[1] for h in holes[s]):8d} B)  '
          f'overlaps {len(neg[s])}  linkonce {nlo:4d}  expected end 0x{cur:08x}  '
          f'original end 0x{ORIG_END[s]:08x}  ({tail:+d} B)')
        for a, n, p, nx in holes[s]:
            P(f'    HOLE 0x{a:08x} +{n:7d} B  after {p}  before {nx}')
        for a, n, p, nx in neg[s]:
            P(f'    OVERLAP 0x{a:08x} -{n} B  {p} / {nx}')
    P('')

    # ---- 4. output -----------------------------------------------------------------
    regions = []
    for l in open('config/data_regions.txt'):
        if l.startswith('#') or not l.strip():
            continue
        f = [x.strip() for x in l.split('|')]
        regions.append((int(f[0], 16), int(f[1], 16), f[2], f[3]))

    def region_of(a, n):
        r = [x for x in regions if x[0] < a + n and x[1] > a]
        return '; '.join(f'{x[2]} {x[3]}' for x in r) or 'no region of data_regions.txt'

    # blob = bytes our .o files lack, to put back. .rodata/.data: the rows of
    # config/data_regions.txt (except crt: crt1.o provides them); .text and .eh_frame: the
    # holes of the simulation. Mode: 'object' (a .o of its own, if start and end are
    # 16-aligned like every gas 2.9EE .o), 'head' (unaligned end, the bytes sit at the
    # head of the next unit's section, so that unit's .o is rebuilt from its out.s with an
    # .include), 'tail' (unaligned start, at the tail of the previous unit's section).
    def owner(s, a, before):
        cand = [(B[s][u][0], u) for u in order if u in B[s]]
        if before:
            c = [x for x in cand if x[0] < a]
            return max(c)[1] if c else None
        c = [x for x in cand if x[0] >= a]
        return min(c) if c else (None, None)
    blobs = []
    for a0, a1, cls, name in regions:
        if cls in ('crt', 'pad'):   # pad: alignment zeros filled in by ld
            continue
        s = '.rodata' if a0 < 0x10000000 else '.data'
        prev = owner(s, a0, True)
        naddr, nxt = owner(s, a1, False)
        if a1 % 16:
            mode, unit = 'head', nxt
            assert naddr == a1, (name, hex(a1), naddr)
        elif a0 % 16:
            mode, unit = 'tail', prev
        else:
            mode, unit = 'object', prev
        blobs.append(dict(sec=s, start=a0, end=a1, cls=cls, name=name, mode=mode, unit=unit, prev=prev, next=nxt))
    for s in ('.text', '.eh_frame'):
        for a, n, p, nx in holes[s]:
            if nx == '.MIPS.stubs':
                continue
            cls = 'asset-dnas' if s == '.text' else 'eh-debt'
            nm = 'DNAS table in .text' if s == '.text' else 'FDE without a .o of ours (orphan_fdes of ehcheck)'
            blobs.append(dict(sec=s, start=a, end=a + n, cls=cls, name=nm, mode='object', unit=p, prev=p, next=nx))
    for b in blobs:
        b['file'] = f"{BLOBS}/blob_{b['sec'].strip('.').replace('.', '_')}_{b['start']:08x}.o"
    merged = {b['unit']: f"{MERGED}/{re.sub(r'[^A-Za-z0-9_]', '_', b['unit'])}.o"
              for b in blobs if b['mode'] != 'object'}
    P('## Blobs (non-compiled bytes to put back)')
    for b in blobs:
        P(f"  {b['sec']:10s} 0x{b['start']:08x}..0x{b['end']:08x} {b['end'] - b['start']:7d} B  {b['cls']:10s} "
          f"{b['mode']:8s} {b['unit']}  [{b['name']}]")
    P('')

    with open(f'{HERE}/link_order.txt', 'w') as f:
        f.write('# Link order (generated by tools/relink/link_order.py)\n')
        f.write('# One .o per line. Blob lines: objects generated from the user\'s ELF by\n')
        f.write('# tools/relink/make_blobs.py (non-compiled bytes), never tracked.\n')
        f.write('# Unproven choices: see order_report.txt and the comments below.\n')
        f.write('#\n# crt at the head: crt1.o (_start 0x47c1d0, .fini 0x18 zero bytes, .rodata _IO_stdin_used,\n'
                '# .data __data_start), crti.o (call_gmon_start 0x47c230, .init/.fini prologues),\n'
                '# the kit\'s crtbegin.o (GCC-29_1.RPM; .text = bytes of ref/gcc-2.95.2/crtstuff.c\n'
                '# from offset 64 on: our unit also includes call_gmon_start of crti).\n')
        for name, path, u in objs:
            if u in placed:
                f.write(f'# data only: {placed[u][0]} 0x{placed[u][1]:08x}\n')
            if u is None and name == 'crtend.o':
                f.write('# crt at the tail: crtend.o (= ref/gcc-2.95.2/crtstuff_end.c, identical bytes), crtn.o.\n'
                        '# libc_nonshared.a (GROUP of libc.so) is not needed: stat/lstat are src/lib/libc_stat.c and\n'
                        '# src/lib/libc_lstat.c (0x911550/0x9115a0), already above.\n')
            if u in merged:
                f.write(f'# {u}: .o rebuilt with the blobs at head/tail (make_blobs.py)\n')
                path = merged[u]
            f.write(path + '\n')
            for b in blobs:
                if b['mode'] == 'object' and b['unit'] == (u or name):
                    f.write(f"{b['file']}   # {b['sec']} 0x{b['start']:08x} +{b['end'] - b['start']} B {b['cls']}: {b['name']}\n")
    json.dump({'order': order, 'objs': [o[1] for o in objs], 'placed': placed,
               'holes': {s: holes[s] for s in holes}, 'overlaps': neg, 'blobs': blobs, 'merged': merged,
               'exceptions': exc}, open(f'{HERE}/order.json', 'w'), indent=1)
    open(f'{HERE}/order_report.txt', 'w').write('\n'.join(rep) + '\n')
    print('\n'.join(rep))


if __name__ == '__main__':
    main()
