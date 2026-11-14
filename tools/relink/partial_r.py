#!/usr/bin/env python3
"""Final list of objects for ld, with the `ld -r` group checked.

Usage: python3 tools/relink/partial_r.py   (called by tools/relink/link.sh, before ld)

Inputs: build/relink/link_order.txt (link_order.py; `#` lines and trailing comments ignored) and
config/relink_partial_r.txt (the objects to merge with `ld -r`: the reason is in the file itself).
A line that is a source (.c/.cpp, a unit of config/units.txt) stands for its gate .o
(paths.unit_obj), so the file does not depend on the name of the build folder.
Check (exits 1 with the reason): every line of relink_partial_r.txt must be in
link_order.txt, once only, and all contiguous and in the same order; otherwise `ld -r`
would silently move objects relative to the original's order.
Writes to build/relink/:
  partial_r.txt  the objects of the group (input of `ld -r`; empty if the group is empty)
  objs.txt       the order for ld: that of link_order.txt with the group replaced, at the
                 place of its first object, by build/relink/partial_r.o
"""
import os, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from paths import OUT, PARTIAL_R, unit_obj  # noqa: E402


def lines(path):
    out = []
    for l in open(path):
        l = l.split('#', 1)[0].strip()
        if l:
            out.append(l)
    return out


def main():
    order = lines(f'{OUT}/link_order.txt')
    group = [unit_obj(p) if p.endswith(('.c', '.cpp')) else p for p in lines(PARTIAL_R)]
    if group:
        dup = sorted({p for p in group if order.count(p) != 1})
        if dup:
            sys.exit(f'error: {PARTIAL_R}: lines missing or repeated in {OUT}/link_order.txt:\n  ' + '\n  '.join(dup))
        i = order.index(group[0])
        if order[i:i + len(group)] != group:
            sys.exit(f'error: {PARTIAL_R}: the objects are not contiguous and in the same order in '
                     f'{OUT}/link_order.txt (from the line of {group[0]})')
        objs = order[:i] + [f'{OUT}/partial_r.o'] + order[i + len(group):]
    else:
        objs = order
    open(f'{OUT}/partial_r.txt', 'w').write(''.join(p + '\n' for p in group))
    open(f'{OUT}/objs.txt', 'w').write(''.join(p + '\n' for p in objs))
    if os.environ.get('VERBOSE') or '-v' in sys.argv[1:]:
        print(f'objects for ld: {len(objs)} (ld -r group: {len(group)})')


if __name__ == '__main__':
    main()
