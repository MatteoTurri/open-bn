#!/usr/bin/env python3
"""Normalize an objdump listing for the matching diff.

- keeps only mnemonic + operands
- turns branch/jump targets into offsets relative in instructions (e.g. B+12)
  so two functions at different addresses diff cleanly
- masks the GOT/$gp offsets and the relocations (decided by the linker)
- collapses the local-page call form `lw t9,GOT(gp); addiu t9,t9,X` into the
  `lw t9,GOT(gp)` form (CALL16). The choice does not depend on the link layout
  but on the source: gas emits the local-page form for a callee defined in the
  same .o (static, or global of the same TU) and CALL16 for an external one.
  For a global callee the collapse hides a wrong TU boundary (and the .o is 4 B
  longer per site): tools/gotident.py reports it (diagnosis `[TU]`). t9 ($25)
  is the call register: the lw+addiu pair on t9 is always the computation of
  the call address.
Reads the `objdump -d` listing from stdin (also with --start/stop-address).

Use as a module (tools/check.py): `normalize_asm.normalize(text)` -> str.
The logic is in normalize()/parse_rows(): the gate calls them in-process
instead of spawning a python interpreter for every function.
"""
import re
import sys

LINE = re.compile(r'^\s*([0-9a-f]+):\s+[0-9a-f]{8}\s+(\S+)\s*(.*?)(?:\s+<[^>]*>)?\s*$')
RELOC = re.compile(r'^\s*[0-9a-f]+:\s+R_MIPS_\S+')
IMM = re.compile(r'(-?\b\d+\b|\b0x[0-9a-f]+\b)')
GPOFF = re.compile(r'(-?\d+|RELOC)\(gp\)')
HEXTARGET = re.compile(r'^(.*?)([0-9a-f]{2,8})$')


def parse_rows(lines):
    """objdump listing -> [[addr, mnem, ops], ...] with the RELOCs wildcarded."""
    rows = []
    for line in lines:
        m = LINE.match(line)
        if m:
            rows.append([int(m.group(1), 16), m.group(2), m.group(3)])
        elif RELOC.match(line) and rows:
            # with objdump -dr: the previous instruction has an immediate decided
            # by the linker -> wildcard (last non-offset constant in the operands)
            ops = rows[-1][2]
            matches = list(IMM.finditer(ops))
            if matches:
                last = matches[-1]
                rows[-1][2] = ops[:last.start()] + 'RELOC' + ops[last.end():]
    return rows


def normalize_rows(rows, with_addrs=False):
    """Normalize already parsed rows -> list of 'mnem ops' strings.
    With with_addrs=True it returns [(addr, 'mnem ops')]: tools/datacheck.py uses
    it to align by index one of our functions with the original (matched = same
    normalized rows) and to read on both sides the instruction that loads a
    datum (the GOT16/LO16 pair -> original address of the datum)."""
    if not rows:
        return []
    # --- normalize the operands (gp/GOT) and collapse the large-GOT pair ---
    cleaned = []  # [(addr, mnem, ops), ...]
    for addr, mnem, ops in rows:
        # lui/addiu pair of _gp_disp (.cpload): the value is decided by the linker
        if mnem == 'lui' and ops.startswith('gp,'):
            ops = 'gp,GPDISP'
        elif mnem == 'addiu' and ops.startswith('gp,gp,'):
            ops = 'gp,gp,GPDISP'
        # GOT/sdata offset via $gp: slots assigned by the linker
        ops = GPOFF.sub('GOT(gp)', ops)
        # collapse the large-GOT low part: 'addiu t9,t9,X' after 'lw t9,GOT(gp)'
        if (mnem == 'addiu' and ops.startswith('t9,t9,')
                and cleaned and cleaned[-1][1] == 'lw'
                and cleaned[-1][2] == 't9,GOT(gp)'):
            continue
        cleaned.append((addr, mnem, ops))

    index_of = {addr: i for i, (addr, _, _) in enumerate(cleaned)}
    out = []
    for i, (addr, mnem, ops) in enumerate(cleaned):
        if mnem.startswith(('b', 'j')) and ops:
            m = HEXTARGET.match(ops)
            if m:
                target = int(m.group(2), 16)
                if target in index_of:
                    ops = f'{m.group(1)}B{index_of[target] - i:+d}'
                else:
                    # jump out of the function (epilogue/tail in the same TU):
                    # the address depends on the layout, not comparable
                    ops = f'{m.group(1)}EXT'
        text = f'{mnem} {ops}'.strip()
        out.append((addr, text) if with_addrs else text)
    return out


def normalize(text):
    """API for the gate: objdump listing (str) -> normalized listing (str)."""
    rows = normalize_rows(parse_rows(text.splitlines()))
    return ''.join(r + '\n' for r in rows)


if __name__ == '__main__':
    rows = parse_rows(sys.stdin.read().splitlines())
    sys.stdout.write(''.join(r + '\n' for r in normalize_rows(rows)))
