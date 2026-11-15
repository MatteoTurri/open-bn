#!/usr/bin/env python3
"""Build variant: recompile some units with extra flags and put them in the link.

Usage: python3 tools/relink/variant.py <name>    (called by tools/relink/link.sh when VARIANT is
       set, after partial_r.py has written objs.txt)
       python3 tools/relink/variant.py --sha1 <name>   prints the expected SHA1 of the variant

config/variants.txt lists, for each variant, the units to recompile, the flags added to those of
config/units.txt and the SHA1 of the bn.elf the variant must produce (`make link-cmp` compares
against it). The variants reproduce known builds of the original program byte for byte: the
DNAS protection and compact_rel.py take their parameters from the original, so a unit whose
allocated sections change size or alignment is rejected here (symbols and relocations are not
checked: the SHA1 is the judge).

Each unit is compiled by the gate's own compile (check.compile_unit) into
build/relink/variant/<unit>/out.o, and in objs.txt the gate's .o
(build/check/<unit>/out.o) is replaced by it. A unit whose .o is not in objs.txt (part of the
`ld -r` group, or rebuilt in build/relink/merged/) is an error: it would need its own handling.
"""
import os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.dirname(HERE))
from paths import OUT, UNITS, unit_obj  # noqa: E402

VARIANTS = 'config/variants.txt'


def read_variant(name):
    """(sha1, [(unit, extra flags)]) of the variant; exits 1 if it is unknown or inconsistent."""
    sha1, units = None, []
    for raw in open(VARIANTS):
        raw = raw.strip()
        if not raw or raw.startswith('#'):
            continue
        parts = [p.strip() for p in raw.split('|')]
        if len(parts) != 4:
            sys.exit(f'error: {VARIANTS}: malformed line: {raw}')
        v, s, unit, flags = parts
        if v != name:
            continue
        if not re.fullmatch('[0-9a-f]{40}', s):
            sys.exit(f'error: {VARIANTS}: {name}: not a SHA1 (40 lowercase hex digits): {s}')
        if sha1 is not None and s != sha1:
            sys.exit(f'error: {VARIANTS}: variant {name} has two different SHA1s')
        if any(u == unit for u, _ in units):
            sys.exit(f'error: {VARIANTS}: variant {name} lists {unit} twice')
        sha1 = s
        units.append((unit, flags))
    if not units:
        sys.exit(f'error: unknown variant {name!r} (see {VARIANTS})')
    return sha1, units


def build(unit, row, extra):
    """Compile the variant's unit; returns its .o. The gate's .o must exist and be in objs.txt
    (checked by main before this, cheap) and the two must have the same allocated sections."""
    import check  # tools/check.py: the gate's compile (compile_unit)
    from link_order import obj_sections  # {name: (size, align)} of the allocated sections
    _, cflags, asflags, _ = row
    out = f'{OUT}/variant/' + os.path.basename(os.path.dirname(unit_obj(unit)))
    errs, implicit = check.compile_unit(unit, f'{cflags} {extra}', asflags, out)
    if errs:
        sys.exit('\n'.join(['error: variant build failed'] + errs))
    if implicit:
        sys.exit(f'error: {unit}: implicit declaration of {", ".join(implicit)}')
    if obj_sections(f'{out}/out.o') != obj_sections(unit_obj(unit)):
        sys.exit(f'error: {unit}: section sizes or alignments differ from {unit_obj(unit)}: '
                 'a variant must not change the layout')
    return f'{out}/out.o'


def main():
    args = sys.argv[1:]
    if len(args) == 2 and args[0] == '--sha1':
        print(read_variant(args[1])[0])
        return
    if len(args) != 1:
        sys.exit('usage: variant.py <name> | variant.py --sha1 <name>')
    _, units = read_variant(args[0])
    objs = open(f'{OUT}/objs.txt').read().splitlines()
    import check
    rows = {r[0]: r for r in check.read_units()}
    for unit, _ in units:
        gate = unit_obj(unit)
        if unit not in rows:
            sys.exit(f'error: {unit} is not in {UNITS}')
        if not os.path.exists(gate):
            sys.exit(f'error: {gate} missing: run a full make check')
        if objs.count(gate) != 1:
            sys.exit(f'error: {gate} is not in {OUT}/objs.txt once (ld -r group or merged blob?)')
    check.cross_include()  # the container of `make link` is a fresh one: libstdc++ headers
    for unit, extra in units:
        gate = unit_obj(unit)
        objs[objs.index(gate)] = build(unit, rows[unit], extra)
        print(f'variant {args[0]}: {unit} {extra}')
    open(f'{OUT}/objs.txt', 'w').write(''.join(p + '\n' for p in objs))


if __name__ == '__main__':
    main()
