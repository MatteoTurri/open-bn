#!/usr/bin/env python3
"""Compare two normalized listings: orig (linked) vs ours (.o with RELOC).

A line of ours containing 'RELOC' matches any value in the corresponding
immediate of orig (the linker decides that value).
Exit 0 = perfect match; otherwise prints the differing lines
(first line `MATCH n/n` or `DIFF n/m (k lines differ)`: other tools parse it).
"""
import os
import re
import sys

# VERBOSE=1: print every differing line instead of the first CAP
CAP = None if os.environ.get('VERBOSE', '') not in ('', '0') else 20

orig = open(sys.argv[1]).read().splitlines()
ours = open(sys.argv[2]).read().splitlines()

IMM = r'(-?\d+|0x[0-9a-f]+)'
mismatches = []
for i in range(max(len(orig), len(ours))):
    a = orig[i] if i < len(orig) else '<missing>'
    b = ours[i] if i < len(ours) else '<missing>'
    if a == b:
        continue
    if 'RELOC' in b:
        pattern = re.escape(b).replace('RELOC', IMM) + r'$'
        if re.match(pattern, a):
            continue
    mismatches.append((i + 1, a, b))

total = len(orig)
if not mismatches:
    print(f'MATCH {total}/{total}')
    sys.exit(0)

print(f'DIFF {total - len(mismatches)}/{total} ({len(mismatches)} lines differ)')
for n, a, b in mismatches[:CAP]:
    print(f'  line {n}:')
    print(f'    orig: {a}')
    print(f'    ours: {b}')
if CAP and len(mismatches) > CAP:
    print(f'  ... and {len(mismatches) - CAP} more (VERBOSE=1 for all)')
sys.exit(1)
