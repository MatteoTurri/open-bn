#!/usr/bin/env python3
"""Resolve the GOT-direct call graph (%call16, `lw $t9, OFF($gp)`) of one or
more functions with certainty: it reads the real value in .got and looks it up
in config/symbol_addrs.txt (plus the unnamed `static_<ADDR>` functions), not
by name heuristics.

Usage:
    python3 tools/got_callees.py SYMBOL [SYMBOL...]

It also resolves same-TU calls in the two-instruction GOT_PAGE+LO16 form: when
the instruction after `lw $t9,OFF($gp)` is `addiu $t9,$t9,DELTA`, the value in
the GOT slot is only the base page (e.g. "0x8d0000", which alone shows
sym=[]) and the real target is val+DELTA. Printed on an extra
`[GOT_PAGE+LO16 same-TU]` line; if no dynsym sits at that target it is a
static/gap function, and for the caller a local stub with the right signature
is enough (its content does not affect the layout).

GOTSYM resolution: many calls to external functions (imported libc,
lazy-bound) have a GOT slot not yet materialized in the file (val=0 in this
static dump, patched by ld.so only at runtime), so the lookup by address
fails (sym=[]). For these, MIPS o32 still gives a certain identity without
the value: the layout of the global (dynamic) GOT is positional, the entry at
index `local_gotno + k` always corresponds to dynsym symbol `gotsym + k`, with
`local_gotno`/`gotsym` read from the `.dynamic` tags DT_MIPS_LOCAL_GOTNO /
DT_MIPS_GOTSYM (and the dynsym table size from DT_MIPS_SYMTABNO). These lines
are marked `[via GOTSYM]` and printed only when the lookup by address gave no
name.

Parses ELF32 directly (no readelf in the container, no
mipsel-linux-gnu-objdump on the host). gp is the fixed global constant of the
binary (0x100D9910). Also used as a module (load_elf, make_addr_to_off,
load_symbols) by the DNAS tools.
"""
import struct
import re
import sys
import os

ELF = 'orig/bn-jpn.elf'
SYMBOL_ADDRS = 'config/symbol_addrs.txt'
GP = 0x100D9910

# .dynamic tags relevant to the MIPS o32 layout of the global (dynamic) GOT
DT_MIPS_LOCAL_GOTNO = 0x7000000a
DT_MIPS_GOTSYM = 0x70000013
DT_MIPS_SYMTABNO = 0x70000011


def load_elf(path):
    with open(path, 'rb') as f:
        data = f.read()
    e_shoff = struct.unpack_from('<I', data, 0x20)[0]
    e_shentsize = struct.unpack_from('<H', data, 0x2e)[0]
    e_shnum = struct.unpack_from('<H', data, 0x30)[0]
    e_shstrndx = struct.unpack_from('<H', data, 0x32)[0]
    sections = []
    for i in range(e_shnum):
        off = e_shoff + i * e_shentsize
        name, stype, flags, addr, offset, size, link, info, align, entsize = \
            struct.unpack_from('<10I', data, off)
        sections.append(dict(name=name, addr=addr, offset=offset, size=size))
    shstr_off = sections[e_shstrndx]['offset']

    def secname(n):
        end = data.index(b'\0', shstr_off + n)
        return data[shstr_off + n:end].decode()

    for s in sections:
        s['nm'] = secname(s['name'])
    return data, sections


def make_addr_to_off(sections):
    def addr_to_off(addr):
        for s in sections:
            if s['nm'] and s['addr'] != 0 and s['size'] > 0 and \
                    s['addr'] <= addr < s['addr'] + s['size']:
                return s['offset'] + (addr - s['addr'])
        return None
    return addr_to_off


def load_symbols(path):
    sym_by_addr = {}
    sym_info = {}
    with open(path) as f:
        for line in f:
            m = re.match(
                r'\s*([A-Za-z_][A-Za-z0-9_@$.]*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;'
                r'(?:.*size:(0x[0-9A-Fa-f]+))?', line)
            if m:
                addr = int(m.group(2), 16)
                sym_by_addr.setdefault(addr, []).append(m.group(1))
                size = int(m.group(3), 16) if m.group(3) else 0
                sym_info[m.group(1)] = (addr, size)
    # The unnamed functions (`static_<ADDR>`, config/static_funcs.txt GENERATED
    # by tools/statics.py) are not in symbol_addrs.txt; they are needed in both
    # directions: as a target (sym_info) and as the name of a resolved callee
    # (sym_by_addr).
    try:
        sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
        import statics
        for n, (addr, size) in statics.load().items():
            sym_by_addr.setdefault(addr, []).append(n)
            sym_info.setdefault(n, (addr, size))
    except Exception:
        pass          # list missing (config/static_funcs.txt): fall back to named symbols only
    return sym_by_addr, sym_info


def make_gotsym_resolver(data, sections):
    """Return resolve(target_addr) -> (dynsym_name, got_idx, symidx) or None,
    using the positional DT_MIPS_GOTSYM/DT_MIPS_LOCAL_GOTNO index instead of
    the value in the GOT slot (which is 0 in the file for lazy-bound imports).
    See the module docstring. Returns None (no extra resolution) if the binary
    lacks the expected sections/tags.
    """
    sec_by_name = {s['nm']: s for s in sections}
    dyn = sec_by_name.get('.dynamic')
    got = sec_by_name.get('.got')
    dynsym = sec_by_name.get('.dynsym')
    dynstr = sec_by_name.get('.dynstr')
    if not (dyn and got and dynsym and dynstr):
        return None

    tags = {}
    off = dyn['offset']
    while True:
        d_tag, d_val = struct.unpack_from('<iI', data, off)
        if d_tag == 0:
            break
        tags[d_tag] = d_val
        off += 8
    local_gotno = tags.get(DT_MIPS_LOCAL_GOTNO)
    gotsym = tags.get(DT_MIPS_GOTSYM)
    symtabno = tags.get(DT_MIPS_SYMTABNO)
    if local_gotno is None or gotsym is None or symtabno is None:
        return None
    dynsym_count = dynsym['size'] // 16

    def dynsym_name(idx):
        o = dynsym['offset'] + idx * 16
        st_name = struct.unpack_from('<I', data, o)[0]
        end = data.index(b'\0', dynstr['offset'] + st_name)
        return data[dynstr['offset'] + st_name:end].decode()

    def resolve(target_addr):
        if not (got['addr'] <= target_addr < got['addr'] + got['size']):
            return None
        if (target_addr - got['addr']) % 4 != 0:
            return None
        got_idx = (target_addr - got['addr']) // 4
        if got_idx < local_gotno:
            return None  # LOCAL region of the GOT: not mapped to dynsym
        symidx = gotsym + (got_idx - local_gotno)
        if not (0 <= symidx < dynsym_count):
            return None
        name = dynsym_name(symidx)
        if not name:
            return None
        return (name, got_idx, symidx)

    return resolve


def analyze(sym, data, addr_to_off, sym_by_addr, sym_info, gotsym_resolve=None):
    if sym not in sym_info:
        print(sym, 'not found in', SYMBOL_ADDRS)
        return
    addr, size = sym_info[sym]
    off = addr_to_off(addr)
    if off is None or size == 0:
        print(sym, '@', hex(addr), '-- no size/offset info')
        return
    b = data[off:off + size]
    calls = []
    targets = []
    n = len(b) // 4
    words = struct.unpack_from('<%dI' % n, b)
    for i, w in enumerate(words):
        op = (w >> 26) & 0x3f
        rt = (w >> 16) & 0x1f
        rs = (w >> 21) & 0x1f
        imm = w & 0xffff
        if imm >= 0x8000:
            imm -= 0x10000
        if op == 0x23 and rs == 28 and rt == 25:  # lw $t9, imm($gp)
            target = GP + imm
            toff = addr_to_off(target)
            val = struct.unpack_from('<I', data, toff)[0] if toff is not None else None
            names = sym_by_addr.get(val, []) if val is not None else []
            # GOT_PAGE+LO16 (same-TU call): if the next instruction is
            # `addiu $t9,$t9,DELTA`, the real target is val+DELTA, not val
            # (val is only the base page, e.g. 0x620000). Printed on an
            # extra line.
            page_delta = None
            if i + 1 < n:
                w2 = words[i + 1]
                if ((w2 >> 26) & 0x3f) == 0x09 and \
                   ((w2 >> 21) & 0x1f) == 25 and ((w2 >> 16) & 0x1f) == 25:
                    d = w2 & 0xffff
                    if d >= 0x8000:
                        d -= 0x10000
                    page_delta = d
            calls.append((hex(addr + i * 4), imm, hex(target),
                          hex(val) if val is not None else None, names))
            targets.append((target, val, page_delta))
    print('=== %s @ %s size %d ===' % (sym, hex(addr), size))
    for c, (target, val, page_delta) in zip(calls, targets):
        print('  insn@%s got_off=%s got_addr=%s -> val=%s sym=%s' % c)
        names = c[4]
        if page_delta is not None and val is not None:
            real = val + page_delta
            rnames = sym_by_addr.get(real, [])
            print('    [GOT_PAGE+LO16 same-TU] +%d -> %s sym=%s%s' % (
                page_delta, hex(real), rnames,
                '' if rnames else '  (no dynsym: static/gap -- a local stub'
                                  ' with the right signature is enough)'))
        if not names and gotsym_resolve is not None:
            hit = gotsym_resolve(target)
            if hit is not None:
                name, got_idx, symidx = hit
                print('    [via GOTSYM] got_idx=%d dynsym=%d -> %s' %
                      (got_idx, symidx, name))
    if not calls:
        print('  (no direct GOT call found -- leaf)')


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    if not os.path.exists(ELF):
        print(ELF, 'is missing -- run from the repo root')
        sys.exit(2)
    data, sections = load_elf(ELF)
    addr_to_off = make_addr_to_off(sections)
    sym_by_addr, sym_info = load_symbols(SYMBOL_ADDRS)
    gotsym_resolve = make_gotsym_resolver(data, sections)
    for sym in sys.argv[1:]:
        analyze(sym, data, addr_to_off, sym_by_addr, sym_info, gotsym_resolve)


if __name__ == '__main__':
    main()
