#!/usr/bin/env python3
"""Extract the assets from the user's original ELF (never committed: docs/original-bytes.md).

Reads config/assets.txt (name | address | bytes | sha1 | file), takes the bytes from
orig/bn-jpn.elf, checks their SHA1 (prefix) and writes assets/<file> as the body of
a C initializer (`0x.., 0x.., ...`), which the source includes with
`#include "../assets/<file>"`. assets/ is gitignored: the repo holds only addresses,
sizes and hashes, never the bytes.

    python3 tools/extract_assets.py           # extract (rewrites only the changed files)
    python3 tools/extract_assets.py --check   # verify only, write nothing

Exits 1 if the ELF is missing, if an address is not inside a section or if a hash differs.
Runs in `make check` (before the gate) and in tools/match.sh.
"""
import hashlib
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
CONF = os.path.join(ROOT, 'config', 'assets.txt')
ELF = os.path.join(ROOT, 'orig', 'bn-jpn.elf')
OUT = os.path.join(ROOT, 'assets')


def entries():
    for n, line in enumerate(open(CONF), 1):
        line = line.split('#', 1)[0].strip()
        if not line:
            continue
        f = [x.strip() for x in line.split('|')]
        if len(f) != 5:
            sys.exit(f'assets     error: {CONF}:{n}: expected 5 fields, found {len(f)}')
        yield f[0], int(f[1], 16), int(f[2]), f[3].lower(), f[4]


def bytes_at(elf, addr, size):
    for s in elf.secs:
        if s['type'] != 8 and s['size'] and s['addr'] <= addr and addr + size <= s['addr'] + s['size']:
            o = s['off'] + addr - s['addr']
            return elf.data[o:o + size]
    return None


def render(name, b):
    lines = [f'/* ASSET {name}: extracted from orig/bn-jpn.elf by tools/extract_assets.py.',
             '   not tracked (assets/ is gitignored), do not edit by hand. */']
    for i in range(0, len(b), 12):
        lines.append('    ' + ', '.join(f'0x{x:02x}' for x in b[i:i + 12]) + ',')
    return '\n'.join(lines) + '\n'


def main():
    check_only = '--check' in sys.argv
    if not os.path.exists(ELF):
        sys.exit(f'assets     error: {ELF} is missing (assets are extracted from the original ELF)')
    from datacheck import Elf
    elf = Elf(ELF)
    errs, wrote, n = [], 0, 0
    for name, addr, size, sha, dest in entries():
        n += 1
        b = bytes_at(elf, addr, size)
        if b is None:
            errs.append(f'{name}: 0x{addr:x}+{size} is outside the ELF sections')
            continue
        h = hashlib.sha1(b).hexdigest()
        if not h.startswith(sha):
            errs.append(f'{name}: sha1 {h[:len(sha)]} != {sha} (different ELF?)')
            continue
        if check_only:
            continue
        path = os.path.join(OUT, dest)
        text = render(name, b)
        if not os.path.exists(path) or open(path).read() != text:
            os.makedirs(os.path.dirname(path), exist_ok=True)
            open(path, 'w').write(text)
            wrote += 1
    for e in errs:
        print('assets     error:', e)
    if errs:
        return 1
    print(f'assets     {n} verified' + (f', {wrote} rewritten in assets/' if wrote else ''))
    return 0


if __name__ == '__main__':
    sys.exit(main())
