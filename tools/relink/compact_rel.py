#!/usr/bin/env python3
"""Post-link .compact_rel step: copy the entries' `relvaddr` and the header's `cpt.offset` from the original.

Bytes taken verbatim from the original (docs/original-bytes.md): our build does not produce
the two fields below, they are copied from orig/bn-jpn.elf only so that the SHA1 equals the
original's. They mean nothing for a modified `bn`: whoever changes the program drops this step
(link.sh) and keeps the .compact_rel written by ld.

Usage: python3 tools/relink/compact_rel.py IN.elf OUT.elf [--orig orig/bn-jpn.elf]
       (called by tools/relink/link.sh after the strip; `make link`)

Why: ld 2.9EE writes each .compact_rel entry from a local `Elf32_crinfo cptrel` of
mips_elf_relocate_section (bfd/elf32-mips.c of binutils 2.9EE) whose ctype, rtype, dist2to,
konst and vaddr it assigns but never `relvaddr` (for R_MIPS_REL32/R_MIPS_32 as for R_MIPS_26
and GPREL/LITERAL: no memset, no other assignment): the file gets whatever was on the stack of
the ld that did the link. That of Sony's native ld cannot be reproduced by our cross ld.
`cpt.offset` is the file position of .compact_rel before the strip, so it depends only on the
size of the .mdebug (original 7,529,808 B, ours ~6.68 MB), which the strip removed: the binary
keeps nothing else of the .mdebug, and a size rebuilt by trial (per-unit -g levels) would be a
fit to a number, not evidence (docs/linking.md).

Format (elf32-mips.c, struct Elf32_compact_rel / Elf32_External_crinfo and their swap
bfd_elf32_swap_compact_rel_out / bfd_elf32_swap_crinfo_out, little-endian):
  header 24 B: id1, num, id2, offset, reserved0, reserved1 (6 words); num = entries written
         (scpt->reloc_count), fewer than the section is sized for: the rest is zero and is
         left untouched (in the original 6,583 entries out of 15,708 slots)
  entry  12 B: info, konst, vaddr (3 words); info = ctype<<31 | rtype<<27 | dist2to<<19
         | relvaddr, with CRINFO_RELVADDR = 0x7ffff and CRINFO_RELVADDR_SH = 0: `relvaddr` is
         exactly the low 19 bits of the info word. bfd never writes the short format (8 B,
         ctype 0); if one is found the step stops without touching anything.

Rules (all checked, no exceptions):
  - the section must have the same address and size in both ELFs, and the header the same
    id1, num, id2, reserved0, reserved1; otherwise nothing is copied;
  - the header's `cpt.offset` is copied from the original, always and only if all the other
    header fields match, and the report prints both values;
  - for each entry the 19 relvaddr bits are copied only if the rest of the entry is already
    equal (ctype, rtype, dist2to = the high 13 bits of info; konst; vaddr); otherwise the
    entry stays as it is and counts as rejected (the first 20 are listed).
Prints entries copied (relvaddr different, now equal), already equal, rejected. Exit 0 if the
step was applied (even with rejections), 1 if the section is not comparable. Works on OUT.tmp
and renames it to OUT only on success; accepts only 32-bit little-endian ELFs.
"""
import argparse, os, shutil, struct, sys
from elftools.elf.elffile import ELFFile

HDR = 24
ENT = 12
RELVADDR = 0x7ffff          # CRINFO_RELVADDR << CRINFO_RELVADDR_SH
CTYPE_LONG = 1 << 31        # CRF_MIPS_LONG << CRINFO_CTYPE_SH
HDR_FIELDS = ('id1', 'num', 'id2', 'offset', 'reserved0', 'reserved1')


def section(path):
    e = ELFFile(open(path, 'rb'))
    # the format below (4-byte little-endian words) holds only for elf32-littlemips
    assert e.elfclass == 32 and e.little_endian, f'{path}: not a 32-bit little-endian ELF'
    s = e.get_section_by_name('.compact_rel')
    if s is None:
        sys.exit(f'error: {path}: no .compact_rel')
    return dict(addr=s['sh_addr'], size=s['sh_size'], off=s['sh_offset'], data=s.data())


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('inp')
    ap.add_argument('out')
    ap.add_argument('--orig', default='orig/bn-jpn.elf')
    a = ap.parse_args()
    # work on OUT.tmp and rename only on success: an error does not leave a stale or
    # half-written OUT that looks good
    tmp = a.out + '.tmp'
    shutil.copyfile(a.inp, tmp)
    try:
        apply(a.orig, tmp)
    except BaseException:
        os.remove(tmp)
        raise
    os.replace(tmp, a.out)


def apply(orig, out):
    o, l = section(orig), section(out)
    if (o['addr'], o['size']) != (l['addr'], l['size']):
        print(f'error: .compact_rel not comparable: orig 0x{o["addr"]:x}+{o["size"]}, '
              f'link 0x{l["addr"]:x}+{l["size"]}: no entry copied')
        sys.exit(1)
    ho = dict(zip(HDR_FIELDS, struct.unpack_from('<6I', o['data'], 0)))
    hl = dict(zip(HDR_FIELDS, struct.unpack_from('<6I', l['data'], 0)))
    bad = [f for f in HDR_FIELDS if f != 'offset' and ho[f] != hl[f]]
    n = ho['num']   # entries written (scpt->reloc_count); the section is sized on a larger
    # count (compact_rel_size of check_relocs) and the rest stays zero
    if bad or HDR + n * ENT > o['size']:
        print(f'error: .compact_rel header differs ({", ".join(bad) or "num beyond the size"}): '
              'no entry copied')
        sys.exit(1)
    lo, ll = o['data'], bytearray(l['data'])
    short = [i for i in range(n) if not (struct.unpack_from('<I', lo, HDR + i * ENT)[0] & CTYPE_LONG)
             or not (struct.unpack_from('<I', ll, HDR + i * ENT)[0] & CTYPE_LONG)]
    if short:
        print(f'error: {len(short)} entries in short format (first: {short[0]}): unexpected format, '
              'no entry copied')
        sys.exit(1)
    copied = same = 0
    rejected = []
    for i in range(n):
        p = HDR + i * ENT
        io, ko, vo = struct.unpack_from('<3I', lo, p)
        il, kl, vl = struct.unpack_from('<3I', ll, p)
        if (io & ~RELVADDR) != (il & ~RELVADDR) or ko != kl or vo != vl:
            rejected.append((i, (io, ko, vo), (il, kl, vl)))
            continue
        if (io & RELVADDR) == (il & RELVADDR):
            same += 1
            continue
        struct.pack_into('<I', ll, p, (il & ~RELVADDR) | (io & RELVADDR))
        copied += 1
    tail = 'same' if lo[HDR + n * ENT:] == bytes(ll[HDR + n * ENT:]) else 'different (left untouched)'
    print(f'.compact_rel: {n} entries; relvaddr copied from the original in {copied}, already equal in {same}, '
          f'rejected {len(rejected)} (rest of the entry differs: nothing copied); '
          f'tail after the {n} entries ({o["size"] - HDR - n * ENT} B) {tail}')
    for i, (io, ko, vo), (il, kl, vl) in rejected[:20]:
        print(f'  rejected entry {i} (0x{o["addr"] + HDR + i * ENT:08x}): orig info=0x{io:08x} konst=0x{ko:08x} '
              f'vaddr=0x{vo:08x} / link info=0x{il:08x} konst=0x{kl:08x} vaddr=0x{vl:08x}')
    # cpt.offset: header, word 3 (after id1, num, id2); then one write of the whole section
    struct.pack_into('<I', ll, 12, ho['offset'])
    with open(out, 'r+b') as f:
        f.seek(l['off'])
        f.write(bytes(ll))
    print(f'  cpt.offset copied from the original: 0x{ho["offset"]:x} (link 0x{hl["offset"]:x}; size of the '
          '.mdebug before strip)')
    print('  note: relvaddr and cpt.offset are copied verbatim from the original: docs/original-bytes.md')


if __name__ == '__main__':
    main()
