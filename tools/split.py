#!/usr/bin/env python3
"""Per-function split of the original binary (replaces splat for this target).

bn is a dynamic Linux ELF, not a flat PS2 image: the rebuild goes through
.o + linker, not through a splat linker script. What is needed from the
"split" is the reference asm of every function, for tools/match and for
reading.

One objdump pass over the whole binary, partitioned by the function
boundaries of the universe (config/symbol_addrs.txt + config/static_funcs.txt,
tools/universe.py: unnamed functions included). Output:
    expected/asm/<addr>_<name>.s     (one file per function)
    expected/functions.txt           (index: addr size name file)

Runs in the container: python3 tools/split.py
"""
import hashlib
import os
import re
import subprocess
import sys

ELF = 'orig/bn-jpn.elf'
SYMTAB = 'config/symbol_addrs.txt'
OUTDIR = 'expected/asm'
INDEX = 'expected/functions.txt'
OBJDUMP = 'mipsel-linux-gnu-objdump'

SYM_RE = re.compile(r'^(\S+) = (0x[0-9A-Fa-f]+); // type:func size:(0x[0-9A-Fa-f]+)')
INSN_RE = re.compile(r'^\s*([0-9a-f]+):\s')


def safe_name(name):
    keep = re.sub(r'[^A-Za-z0-9_.$]', '_', name)
    if len(keep) > 80:
        keep = keep[:80] + '_' + hashlib.sha1(name.encode()).hexdigest()[:8]
    return keep


def main():
    # universe = .dynsym + unnamed functions (tools/statics.py): same definition
    # as the gate, so expected/asm/ has a file for every function
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import universe
    allsyms = universe.symbols()
    funcs = sorted((a, s, n) for n, (a, s) in allsyms.items() if s)
    n_static = sum(1 for _, _, n in funcs if universe.is_static(n))

    os.makedirs(OUTDIR, exist_ok=True)

    # one objdump of the whole binary; -z: without it, runs of zeros (>=2 nop)
    # collapse into "..." and the references have holes (false EXT downstream)
    proc = subprocess.Popen(
        [OBJDUMP, '-z', '-d', ELF], stdout=subprocess.PIPE, text=True, bufsize=1 << 20)

    boundaries = [(a, a + s, n) for a, s, n in funcs]
    idx = 0
    cur = None       # (start, end, name, rows)
    written = 0
    index_lines = []

    for line in proc.stdout:
        m = INSN_RE.match(line)
        if not m:
            continue
        addr = int(m.group(1), 16)
        while idx < len(boundaries) and addr >= boundaries[idx][1]:
            idx += 1
        if cur and addr >= cur[1]:
            flush(cur, index_lines)
            written += 1
            cur = None
        if idx < len(boundaries):
            start, end, name = boundaries[idx]
            if start <= addr < end:
                if cur is None:
                    cur = (start, end, name, [])
                cur[3].append(line.rstrip('\n'))
    if cur:
        flush(cur, index_lines)
        written += 1
    proc.wait()

    # DNAS: for the self-modifying functions the reference file is the whole
    # function of the decrypted image, scaffold included (tools/dnas_ref.py),
    # i.e. the same reference the gate uses. The descriptor words are assets:
    # the row keeps the real word but the build's placeholder text, as in the
    # gate's reference.
    import normalize_asm
    import dnas_ref
    import dnas_subst
    prot = dnas_ref.protected(allsyms)     # by name (.dynsym) and by address (statics)
    pfuncs = [(a, s, n) for a, s, n in funcs if n in prot]
    if pfuncs:
        img = dnas_ref.ensure_image()
        dis = subprocess.run([OBJDUMP, '-z', '-d', img], capture_output=True, text=True).stdout
        lines = [l.rstrip('\n') for l in dis.splitlines() if INSN_RE.match(l)]
        desc = dnas_ref.desc_addrs(normalize_asm.parse_rows(lines))
        w = '0x%08x' % dnas_subst.PLACEHOLDER
        by_addr = {int(INSN_RE.match(l).group(1), 16): l for l in lines}
        for a, s, n in pfuncs:
            keep = []
            for x in range(a, a + s, 4):
                line = by_addr[x]
                if x in desc:
                    raw = line.split(':', 1)[1].split()[0]
                    line = f'{x:8x}:\t{raw} \t.word\t{w} <DNAS descriptor: asset, masked>'
                keep.append(line)
            fname = f'{a:08X}_{safe_name(n)}.s'
            with open(os.path.join(OUTDIR, fname), 'w') as f:
                f.write(f'# {n} @ {a:#x} size {s:#x}\n')
                # 'DNAS reference' is the marker tools/match.sh looks for in this header
                f.write(f'# DNAS REFERENCE: whole function from the decrypted image, '
                        f'scaffold included, {sum(1 for x in range(a, a + s, 4) if x in desc)} '
                        f'descriptor words masked; see tools/dnas_ref.py and docs/dnas.md\n')
                f.write('\n'.join(keep) + '\n')

    with open(INDEX, 'w') as f:
        f.write('\n'.join(index_lines) + '\n')
    print(f'split      {written} functions written to {OUTDIR}/ ({n_static} unnamed, '
          f'{len(pfuncs)} DNAS from the decrypted image)')
    if written != len(funcs):
        print(f'warning: {len(funcs)} functions in the universe, {written} written')


def flush(cur, index_lines):
    start, end, name, lines = cur
    fname = f'{start:08X}_{safe_name(name)}.s'
    with open(os.path.join(OUTDIR, fname), 'w') as f:
        f.write(f'# {name} @ {start:#x} size {end - start:#x}\n')
        f.write('\n'.join(lines) + '\n')
    index_lines.append(f'{start:#010x} {end - start:#8x} {name} {fname}')


if __name__ == '__main__':
    main()
