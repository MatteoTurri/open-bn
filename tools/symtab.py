#!/usr/bin/env python3
"""Generate the symbol_addrs.txt of a target from the .dynsym of its original.

One row per defined symbol of .dynsym, in .dynsym order, first occurrence of a name:
    <name> = 0x<ADDR>; // type:func size:0x<SIZE>      functions
    <name> = 0x<ADDR>; // size:0x<SIZE>                everything else

The .dynsym size of a function is not trusted: some are wrong (an STL template instance
whose size is that of another TU's copy runs into the next function; some leave out the
delay slot of the final jump). The size written is the one the code gives, with the rule
tools/statics.py uses for the unnamed functions: the function ends where the next one
starts (next .dynsym symbol inside .text, the next `.cpload` prologue at an 8-aligned address,
or the linker's lazy-binding stubs)
minus the zero padding, keeping a delay slot. A size that differs from .dynsym is written
with `(fixed from 0x..: ...)` after it, like the hand fixes of bn's list.

Checked on bn: `--check bn` regenerates config/symbol_addrs.txt from orig/bn-jpn.elf (the
DNAS-decrypted image) and compares name, address and size of every row with the
committed file, which carries 9 sizes fixed by hand.

    python3 tools/symtab.py <target> --write    write the target's symbol_addrs.txt
    python3 tools/symtab.py <target> --check    compare with the file in place (exit 1 if it differs)
    python3 tools/symtab.py <target>            print the rows whose size was fixed
"""
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

ROW_RE = re.compile(r'^(\S+) = 0x([0-9A-Fa-f]+); // (type:func )?size:0x([0-9A-Fa-f]+)')
STT = {0: 'NOTYPE', 1: 'OBJECT', 2: 'FUNC', 3: 'SECTION', 4: 'FILE'}


def dynsym(path):
    """[(name, addr, size, type, shndx)] of the defined symbols, in .dynsym order."""
    import target
    secs = target.elf_sections(path)
    d = open(path, 'rb').read()
    _, sym_off, sym_size, _ = secs['.dynsym']
    _, str_off, _, _ = secs['.dynstr']
    out = []
    for i in range(1, sym_size // 16):
        nm, value, size, info, _other, shndx = struct.unpack_from('<IIIBBH', d, sym_off + i * 16)
        typ = STT.get(info & 0xF, str(info & 0xF))
        if shndx == 0 or typ in ('SECTION', 'FILE'):
            continue
        end = d.index(b'\0', str_off + nm)
        out.append((d[str_off + nm:end].decode('latin-1'), value, size, typ, shndx))
    return out


def rows(statics):
    """[(name, addr, size, type, dynsym_size)] with the function sizes from the code."""
    text = statics.text_image()
    t0, t1 = statics.TEXT_START, statics.TEXT_START + statics.TEXT_SIZE
    seen, out = set(), []
    syms = [s for s in dynsym(statics.TARGET.path(statics.ELF)) if not (s[0] in seen or seen.add(s[0]))]
    # every defined symbol inside .text bounds the function before it: the DNAS data
    # tables of bn are OBJECT symbols in .text, right after a function
    starts = sorted({a for _, a, sz, typ, _ in syms if t0 <= a < t1})
    for name, a, sz, typ, _ in syms:
        size = sz
        if typ == 'FUNC' and sz and t0 <= a < t1:
            nxt = next((s for s in starts if s > a), t1)
            va = (a + 12 + 7) // 8 * 8
            while va < nxt:                     # an unnamed function right after this one
                if statics.is_prologue(text, va) or (va % 16 == 0 and statics.is_stub(text, va)):
                    nxt = va
                    break
                va += 8
            size = statics.func_end(text, a, nxt) - a
        out.append((name, a, size, typ, sz))
    return out


def fmt(name, a, size, typ, dsz):
    line = f'{name} = 0x{a:08X}; // ' + ('type:func ' if typ == 'FUNC' else '') + f'size:0x{size:X}'
    if size != dsz:
        how = 'runs into the next function' if dsz > size else 'leaves out the end'
        line += (f' (fixed from 0x{dsz:X}: the .dynsym size {how}; the function ends where '
                 f'the next one starts, minus the padding: tools/symtab.py)')
    return line


def load(path):
    out = {}
    for line in open(path):
        m = ROW_RE.match(line)
        if m:
            out[m.group(1)] = (int(m.group(2), 16), int(m.group(4), 16), bool(m.group(3)))
    return out


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    if len(args) != 1:
        sys.exit(__doc__)
    os.environ['BN_TARGET'] = args[0]
    import statics  # reads BN_TARGET at import
    t = statics.TARGET
    t.check_original()
    r = rows(statics)
    fixed = [x for x in r if x[2] != x[4]]
    if '--check' in sys.argv:
        have = load(t.path(t.symtab))
        want = {n: (a, s, typ == 'FUNC') for n, a, s, typ, _ in r}
        diff = sorted(set(have) ^ set(want)) + sorted(n for n in set(have) & set(want)
                                                      if have[n] != want[n])
        for n in diff[:20]:
            print(f'  {n[:70]}: file {have.get(n)}  generated {want.get(n)}')
        print(f'symtab     {t.name}: {len(want)} rows generated, {len(have)} in {t.symtab}, '
              f'{len(diff)} different; {len(fixed)} sizes fixed against .dynsym')
        sys.exit(1 if diff else 0)
    if '--write' in sys.argv:
        os.makedirs(t.path(t.config), exist_ok=True)
        with open(t.path(t.symtab), 'w') as f:
            f.write('\n'.join(fmt(*x) for x in r) + '\n')
        print(f'wrote {t.symtab}: {len(r)} rows, {sum(1 for x in r if x[3] == "FUNC")} functions, '
              f'{len(fixed)} sizes fixed against .dynsym')
        return
    for x in fixed:
        print(fmt(*x)[:160])
    print(f'{len(fixed)} function sizes differ from .dynsym')


if __name__ == '__main__':
    main()
