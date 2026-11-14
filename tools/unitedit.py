#!/usr/bin/env python3
"""Targeted edit of one row of config/units.txt, without rewriting the file.

Rewriting units.txt whole risks losing other rows, and a blind append
for a source that already has a row produces a duplicate that makes the whole
gate exit 1. This tool does read-modify-write of the one row concerned, writes
atomically and validates the file again before leaving it.

Usage:
  python3 tools/unitedit.py src/foo.cpp --skip-del SYM1,SYM2      # matched: remove them
  python3 tools/unitedit.py src/foo.cpp --skip-add SYM            # body not matching: add it
  python3 tools/unitedit.py src/foo.cpp --cflags '-O2 -fno-exceptions'
  python3 tools/unitedit.py src/foo.cpp --asflags -g2
  python3 tools/unitedit.py src/foo.cpp --note-file /tmp/note.txt # comment above the row
  python3 tools/unitedit.py src/foo.cpp --show
  python3 tools/unitedit.py --new src/new.cpp --cflags '-O2'      # new row (at the end)
  python3 tools/unitedit.py src/old.c --remove                    # removes the row (TU merge)
  python3 tools/unitedit.py src/old.c --rename src/dir/old.c      # file moved: row kept in place

`--remove` is for unit merges: when two "units" turn out to be a single TU of
the original, the code moves into the host file and the absorbed unit's row
must be removed -- not emptied, not commented out, or the gate compiles a source
that no longer exists. It also removes the comment block attached above the row
(the notes written with --note-file), which would be orphaned without its row.
It cannot be combined with the other options.

`--rename NEW` changes only the source column, in place: the row stays where it
is (the order of units.txt is the order of tools/check.py:assign_statics) and
the rest of the row stays as is, spaces included. `--remove` + `--new` would
move it to the end. It cannot be combined with the other options; it refuses if
NEW is already a unit or if the file NEW does not exist.

Exits 1 without writing anything if: the unit does not exist (and --new was not
given), it already exists (with --new), the row is malformed, or the file would
contain a duplicate unit.
"""
import argparse
import os
import sys

UNITS = 'config/units.txt'


def parse(line):
    p = [x.strip() for x in line.split('|')]
    while len(p) < 4:
        p.append('')
    return p[0], p[1], p[2], [s for s in (x.strip() for x in p[3].split(',')) if s]


def fmt(src, cflags, asflags, skips):
    return '%s | %s | %s | %s' % (src, cflags, asflags, ','.join(skips))


def validate(lines):
    seen, errs = set(), []
    for i, raw in enumerate(lines, 1):
        t = raw.strip()
        if not t or t.startswith('#'):
            continue
        if t.count('|') != 3:
            errs.append('line %d: %d separators instead of 3' % (i, t.count('|')))
            continue
        src = t.split('|')[0].strip()
        if src in seen:
            errs.append('line %d: duplicate unit %s' % (i, src))
        seen.add(src)
    return errs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('unit')
    # they accumulate: `--skip-add A,B` and `--skip-add A --skip-add B` are equivalent
    ap.add_argument('--skip-add', action='append', default=[])
    ap.add_argument('--skip-del', action='append', default=[])
    ap.add_argument('--cflags')
    ap.add_argument('--asflags')
    ap.add_argument('--note')
    ap.add_argument('--note-file')
    ap.add_argument('--new', action='store_true',
                    help='create the row (fails if it already exists)')
    ap.add_argument('--remove', action='store_true',
                    help='Remove the row and its comment block (TU merge)')
    ap.add_argument('--rename', metavar='NEW',
                    help='change the source path keeping the row in place')
    ap.add_argument('--show', action='store_true')
    a = ap.parse_args()

    if a.rename is not None and (a.new or a.remove or a.show or a.skip_add or a.skip_del
                                 or a.cflags is not None or a.asflags is not None
                                 or a.note or a.note_file):
        print('[X] --rename cannot be combined with the other options')
        sys.exit(1)

    if a.remove and (a.new or a.show or a.skip_add or a.skip_del or a.cflags
                     is not None or a.asflags is not None or a.note or a.note_file):
        print('[X] --remove cannot be combined with the other options')
        sys.exit(1)

    lines = open(UNITS).readlines()
    idx = None
    for i, raw in enumerate(lines):
        t = raw.strip()
        if t and not t.startswith('#') and t.split('|')[0].strip() == a.unit:
            idx = i
            break

    if a.rename is not None:
        if idx is None:
            print('[X] %s has no row in %s: nothing to rename' % (a.unit, UNITS))
            sys.exit(1)
        if not os.path.isfile(a.rename):
            print('[X] %s does not exist: move the file first (git mv)' % a.rename)
            sys.exit(1)
        raw = lines[idx]
        col, rest = raw.split('|', 1)
        lines[idx] = col.replace(a.unit, a.rename, 1) + '|' + rest
        errs = validate(lines)
        if errs:
            print('[X] rename refused, file untouched:')
            for e in errs:
                print('   ', e)
            sys.exit(1)
        tmp = UNITS + '.tmp%d' % os.getpid()
        with open(tmp, 'w') as f:
            f.writelines(lines)
        os.replace(tmp, UNITS)                   # atomic: no half-written file
        print('[OK] %s -> %s (line %d)' % (a.unit, a.rename, idx + 1))
        return

    if a.remove:
        if idx is None:
            print('[X] %s has no row in %s: nothing to remove' % (a.unit, UNITS))
            sys.exit(1)
        first = idx
        while first and lines[first - 1].lstrip().startswith('#'):
            first -= 1          # the comment block above the row goes with it
        removed = lines[first:idx + 1]
        del lines[first:idx + 1]
        errs = validate(lines)
        if errs:
            print('[X] removal refused, file untouched:')
            for e in errs:
                print('   ', e)
            sys.exit(1)
        tmp = UNITS + '.tmp%d' % os.getpid()
        with open(tmp, 'w') as f:
            f.writelines(lines)
        os.replace(tmp, UNITS)                   # atomic: no half-written file
        print('[OK] %s: line %d removed (%d lines in all, %d of comment)'
              % (a.unit, idx + 1, len(removed), len(removed) - 1))
        return

    if a.new:
        if idx is not None:
            print('[X] %s already has a row (%d): use the form without --new' %
                  (a.unit, idx + 1))
            sys.exit(1)
        src, cf, af, sk = a.unit, a.cflags or '', a.asflags or '', []
        idx = len(lines)
        lines.append('')
    else:
        if idx is None:
            print('[X] %s has no row in %s (to create it: --new)' % (a.unit, UNITS))
            sys.exit(1)
        src, cf, af, sk = parse(lines[idx])

    if a.show:
        print('unit     %s\ncflags   %r\nasflags  %r\nskip (%d)' % (src, cf, af, len(sk)))
        for s in sk:
            print('   ', s)
        return

    if a.cflags is not None:
        cf = a.cflags
    if a.asflags is not None:
        af = a.asflags
    add = [s.strip() for v in a.skip_add for s in v.split(',') if s.strip()]
    to_delete = {s.strip() for v in a.skip_del for s in v.split(',') if s.strip()}
    missing = to_delete - set(sk)
    if missing:
        print('[X] not in the skip list, nothing to remove: %s' % ', '.join(sorted(missing)))
        sys.exit(1)
    sk = [s for s in sk if s not in to_delete] + [s for s in add if s not in sk]

    lines[idx] = fmt(src, cf, af, sk) + '\n'

    note_text = a.note
    if a.note_file:
        note_text = open(a.note_file).read()
    if note_text:
        note_block = [('# ' + l).rstrip() + '\n' for l in note_text.rstrip().splitlines()]
        lines[idx:idx] = note_block

    errs = validate(lines)
    if errs:
        print('[X] edit refused, file untouched:')
        for e in errs:
            print('   ', e)
        sys.exit(1)

    tmp = UNITS + '.tmp%d' % os.getpid()
    with open(tmp, 'w') as f:
        f.writelines(lines)
    os.replace(tmp, UNITS)                       # atomic: no half-written file
    print('[OK] %s: %d skip%s' % (src, len(sk),
          (', +%d note' % len(note_block)) if note_text else ''))


if __name__ == '__main__':
    main()
