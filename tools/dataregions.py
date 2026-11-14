#!/usr/bin/env python3
"""Data regions that no compiler produced (config/data_regions.txt).

The denominator of the data gate (tools/datacheck.py: .rodata + .data + .ctors +
.dtors, 1,083,856 B) also holds bytes that are not matching work: assets (audio),
encrypted code, high-entropy DNAS tables, VU microcode. This module reads them
and counts them per class.

    python3 tools/dataregions.py           # per-class summary + consistency + overlapping ok blocks
As a module: regions() -> [(start, end, class, name)], by_class().
"""
import json
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PATH = os.path.join(ROOT, 'config', 'data_regions.txt')
CLASSES = ('asset', 'encrypted', 'asset-dnas', 'vu', 'crt', 'pad')


def regions():
    out = []
    for line in open(PATH):
        line = line.split('#', 1)[0].strip()
        if not line:
            continue
        f = [x.strip() for x in line.split('|')]
        a, b, cls, name = int(f[0], 16), int(f[1], 16), f[2], f[3]
        if cls not in CLASSES:
            raise SystemExit(f'regions    error: {PATH}: unknown class {cls!r}')
        out.append((a, b, cls, name))
    return sorted(out)


def check(regs, located):
    """Regions sorted and disjoint; no located block (ok/fail) inside a region."""
    errs = []
    for (a0, b0, _, n0), (a1, _, _, n1) in zip(regs, regs[1:]):
        if a1 < b0:
            errs.append(f'overlapping regions: {n0} and {n1}')
    for unit, sec, a, size in located:
        for ra, rb, cls, name in regs:
            if a < rb and a + size > ra:
                errs.append(f'{unit} {sec} [{a:#x}+{size}] falls inside {name} ({cls})')
    return errs


def ok_overlaps(path=os.path.join(ROOT, 'build', 'datacheck.json')):
    """Two `ok` blocks of different units (`.rodata`/`.data`, not linkonce) that overlap
    by 16 B or more (beyond an alignment tail): at least one is a false ok, typically
    MASKED trailing bytes covering the block of the next TU."""
    d = json.load(open(path))
    bl = sorted((x['addr'], x['addr'] + x['size'], u, s) for u, b in d['units'].items()
                for s, x in b.items() if s in ('.rodata', '.data') and x.get('state') == 'ok'
                and x.get('addr') is not None)
    errs = []
    for a, b in zip(bl, bl[1:]):
        if b[0] < a[1] and a[1] - b[0] >= 16 and a[2] != b[2]:
            errs.append(f'ok blocks overlap by {a[1] - b[0]} B: {a[2]} {a[3]} '
                        f'[{a[0]:#x}+{a[1] - a[0]}] and {b[2]} {b[3]} [{b[0]:#x}]')
    return errs


def located_blocks(path=os.path.join(ROOT, 'build', 'datacheck.json')):
    # the uncompared alignment tail (`tail`, tools/datacheck.py) does not belong to the unit:
    # in the original those bytes already belong to the next object, which may start a region
    d = json.load(open(path))
    return [(u, s, x['addr'], x['size'] - x.get('tail', 0))
            for u, bl in d['units'].items() for s, x in bl.items()
            if x.get('state') in ('ok', 'fail') and x.get('addr') is not None
            and x.get('host') in ('.rodata', '.data')]


def by_class():
    tot = {}
    for a, b, cls, _ in regions():
        tot[cls] = tot.get(cls, 0) + (b - a)
    return tot


def main():
    regs = regions()
    if os.environ.get('VERBOSE', '') not in ('', '0') or '-v' in sys.argv:
        for cls, n in sorted(by_class().items()):
            print(f'{cls:11s} {n:9,} B')
    errs = check(regs, located_blocks()) + ok_overlaps()
    for e in errs[:20]:
        print('regions    error:', e)
    if len(errs) > 20:
        print(f'... and {len(errs) - 20} more')
    if not errs:
        print(f'regions    {len(regs)} non-compiled regions ({sum(by_class().values()):,} B), '
              f'consistent with the data gate')
    return 1 if errs else 0


if __name__ == '__main__':
    sys.exit(main())
