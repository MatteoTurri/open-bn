#!/usr/bin/env python3
"""Data gate: .rodata, .data, .ctors/.dtors (and the vtables) compared byte by byte.

The function gate (tools/check.py) compares the disassembly with relocations
masked, so a wrong string, a wrong initializer, a static table with wrong values
or a vtable in the wrong order would pass. This covers the 1,083,856 B of the
original (.rodata 249,860 + .data 833,624 + .ctors 268 + .dtors 104) that the
compiler emits from our sources.

Where a unit's data sits in the original (identity by address, as for the
statics of tools/statics.py):
  1. Anchors by name: OBJECT symbols of our .o that exist in .dynsym (globals,
     `__vt_*`): their address is known.
  2. Anchors from code: in a matched function the normalized rows are the same
     on both sides, so the `lw rX,%got(.rodata+A)(gp)` / `addiu rX,rX,%lo(...)`
     pair of our .o sits at the same index as the `lw rX,OFF(gp)` /
     `addiu rX,rX,D` pair of the original: the GOT slot at gp+off gives the page
     P, and P+D is the original address of the datum we put at .rodata+A. The
     addend A is (GOT16 imm << 16) + LO16 imm.
  Every anchor gives a delta (original address - our offset) for one section of
  the .o; if all anchors of the section agree, the unit's block is placed and
  compared whole. Disagreeing anchors = different layout (order/set of
  objects), reported as a conflict.
Blocks still without anchors are then placed by link order (link_order_hints)
and by content (content_hints).

Block states (JSON `state`): `ok` (equal), `fail` (placed, different: fails the
gate unless in skip), `conflict` (disagreeing anchors = section layout differs
from the original: measured, not judged), `unanchored` (no anchor: a TU with no
matched function touching that data), `out-of-section`, `order-differs` (placed
only by link order and different), `bss`. A block placed without its own anchors
says how: `by_link_order`, `by_data`, `by_pointers` or `by_content` (true).

Comparison: word by word; one of our words with an R_MIPS_32 relocation is
`base(symbol) + word`, where base is the known address of the symbol (universe
function, .dynsym OBJECT, already placed section of this unit); if the base is
unknown the word is MASKED (counted separately, not as equal).
.bss/.sbss have no bytes: only the size is reported.

Skips: as for functions, in the skip column of config/units.txt, in the form
`data:<section>` (e.g. `data:.rodata`, `data:.gnu.linkonce.d.__vt_11SceneWindow`).
A placed block that does not match and is not in skip makes it exit 1.

Runs in the container (uses mipsel-linux-gnu-objdump on the .o files in
build/check/ produced by check.py, which must have just run); it is the second
step of `make check`:
    python3 tools/datacheck.py [--verbose] [--unit SRC]   (--unit writes build/datacheck-unit-<SRC>.json)
    python3 tools/datacheck.py --populate-skips   (host) differing blocks -> skip column
Writes build/datacheck.json.
"""
import json
import os
import re
import struct
import sys
from bisect import bisect_right

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import normalize_asm                                              # noqa: E402
import universe                                                   # noqa: E402
import statics                                                    # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ELF = 'orig/bn-jpn.elf'
# With a reduced UNITS file (scoped gate) the name carries the basename of the
# units file, exactly as `tools/check.py` does for build/check.json, so a scoped
# run never overwrites build/datacheck.json.
_UNITS = os.environ.get('UNITS', 'config/units.txt')
OUT = ('build/datacheck.json' if _UNITS == 'config/units.txt'
       else 'build/datacheck-%s.json' % re.sub(
           r'[^A-Za-z0-9_.-]', '_', os.path.basename(_UNITS).rsplit('.', 1)[0]))
OBJDUMP = 'mipsel-linux-gnu-objdump'
GP = 0x100D9910
VERBOSE = '--verbose' in sys.argv     # one line per block, too
# statistics that need no action: only with VERBOSE=1 (environment) or -v/--verbose
STATS = VERBOSE or '-v' in sys.argv or os.environ.get('VERBOSE', '') not in ('', '0')


def info(msg):
    if STATS:
        print(msg)
DEBUG = '--debug' in sys.argv

# .o section -> original section that hosts it (ld 2.9 script:
# .data : { *(.data) *(.gnu.linkonce.d*) CONSTRUCTORS }, .rodata likewise)
def host_section(sec):
    if sec == '.rodata' or sec.startswith('.gnu.linkonce.r.'):
        return '.rodata'
    if sec in ('.data', '.sdata') or sec.startswith('.gnu.linkonce.d.'):
        return '.data'
    if sec in ('.ctors', '.dtors'):
        return sec
    if sec in ('.bss', '.sbss') or sec.startswith('.gnu.linkonce.b.'):
        return '.bss'
    return None


DATA_UNIVERSE = ('.rodata', '.data', '.ctors', '.dtors')

# --------------------------------------------------------------------------- #
# ELF32 little-endian: sections, symbols, REL relocations
# --------------------------------------------------------------------------- #
SHT_NOBITS, SHT_REL, SHT_SYMTAB, SHT_DYNSYM = 8, 9, 2, 11
SHF_ALLOC, SHF_EXECINSTR = 0x2, 0x4
STT_OBJECT, STT_FUNC, STT_SECTION = 1, 2, 3
R_MIPS_32, R_MIPS_GOT16, R_MIPS_LO16, R_MIPS_HI16, R_MIPS_GPREL32 = 2, 9, 6, 5, 12


class Elf:
    def __init__(self, path):
        self.data = open(path, 'rb').read()
        d = self.data
        shoff, shentsize, shnum, shstrndx = (struct.unpack_from('<I', d, 0x20)[0],
                                             struct.unpack_from('<H', d, 0x2e)[0],
                                             struct.unpack_from('<H', d, 0x30)[0],
                                             struct.unpack_from('<H', d, 0x32)[0])
        secs = []
        for i in range(shnum):
            f = struct.unpack_from('<10I', d, shoff + i * shentsize)
            secs.append(dict(name=f[0], type=f[1], flags=f[2], addr=f[3], off=f[4],
                             size=f[5], link=f[6], info=f[7], align=f[8], entsize=f[9]))
        strtab = secs[shstrndx]['off']
        for s in secs:
            s['nm'] = self._str(strtab, s['name'])
        self.secs = secs
        self.by_name = {s['nm']: s for s in secs}

    def _str(self, tab, n):
        end = self.data.index(b'\0', tab + n)
        return self.data[tab + n:end].decode('latin-1')

    def bytes_of(self, s):
        if s['type'] == SHT_NOBITS:
            return b''
        return self.data[s['off']:s['off'] + s['size']]

    def symbols(self, kind=SHT_SYMTAB):
        """[(name, value, size, type, bind, shndx)]"""
        out = []
        for s in self.secs:
            if s['type'] != kind:
                continue
            strtab = self.secs[s['link']]['off']
            for i in range(s['size'] // 16):
                name, value, size, info, other, shndx = struct.unpack_from(
                    '<IIIBBH', self.data, s['off'] + i * 16)
                out.append((self._str(strtab, name), value, size, info & 0xf, info >> 4, shndx))
        return out

    def relocs(self, target_name):
        """[(offset, symidx, type)] of the .rel.<target_name> section."""
        s = self.by_name.get('.rel' + target_name)
        if not s:
            return []
        out = []
        for i in range(s['size'] // 8):
            off, info = struct.unpack_from('<II', self.data, s['off'] + i * 8)
            out.append((off, info >> 8, info & 0xff))
        return out

    def word_at_addr(self, addr):
        for s in self.secs:
            if s['flags'] & SHF_ALLOC and s['type'] != SHT_NOBITS and s['addr'] <= addr < s['addr'] + s['size']:
                return struct.unpack_from('<I', self.data, s['off'] + addr - s['addr'])[0]
        return None


# --------------------------------------------------------------------------- #
# original side
# --------------------------------------------------------------------------- #
class Orig:
    def __init__(self):
        self.elf = Elf(os.path.join(ROOT, ELF))
        self.sections = {n: self.elf.by_name[n] for n in DATA_UNIVERSE + ('.bss', '.sbss')
                         if n in self.elf.by_name}
        # OBJECT symbols with a size in .dynsym: anchors by name
        self.objects = {}
        for name, value, size, typ, bind, shndx in self.elf.symbols(SHT_DYNSYM):
            if typ == STT_OBJECT and size:
                self.objects[name] = (value, size)
        self.funcs = universe.symbols()     # name -> (addr, size), statics included
        # every .dynsym name (UND too: the libc functions it imports)
        self.dynnames = {name for name, *_ in self.elf.symbols(SHT_DYNSYM)}

    def got(self, off):
        return self.elf.word_at_addr(GP + off)

    def bytes(self, addr, size):
        for s in self.elf.secs:
            if s['flags'] & SHF_ALLOC and s['type'] != SHT_NOBITS and s['addr'] <= addr and addr + size <= s['addr'] + s['size']:
                o = s['off'] + addr - s['addr']
                return self.elf.data[o:o + size]
        return None

    def section_of(self, addr):
        for n, s in self.sections.items():
            if s['addr'] <= addr < s['addr'] + s['size']:
                return n
        return None


# --------------------------------------------------------------------------- #
# original rows from expected/asm (make split): file by address
# --------------------------------------------------------------------------- #
_ASM_INDEX = None


def build_asm_index():
    """Must be called before the thread pool: building it lazily inside the
    threads let the others see a half-filled dictionary."""
    global _ASM_INDEX
    d = os.path.join(ROOT, 'expected/asm')
    idx = {}
    if os.path.isdir(d):
        for f in os.listdir(d):
            if f.endswith('.s') and len(f) > 9 and f[8] == '_':
                try:
                    idx[int(f[:8], 16)] = f
                except ValueError:
                    pass
    _ASM_INDEX = idx
    return len(idx)


def orig_rows(addr):
    d = os.path.join(ROOT, 'expected/asm')
    if _ASM_INDEX is None:
        build_asm_index()
    f = _ASM_INDEX.get(addr)
    if not f:
        return None
    lines = [l.rstrip('\n') for l in open(os.path.join(d, f), encoding='utf-8', errors='replace')
             if not l.startswith('#')]
    return normalize_asm.parse_rows(lines)


# --------------------------------------------------------------------------- #
# our .o: disassembly with relocations per row
# --------------------------------------------------------------------------- #
RELOC_LINE = re.compile(r'^\s*([0-9a-f]+):\s+(R_MIPS_\w+)\s+(\S+)')
INSN_LINE = re.compile(r'^\s*([0-9a-f]+):\s+[0-9a-f]{8}\s+')
SECTION_RE = re.compile(r'^Disassembly of section (\S+):\s*$')
IMM = re.compile(r'(-?\b\d+\b|\b0x[0-9a-f]+\b)')
GPOFF = re.compile(r'(-?\d+)\(gp\)')


def our_text_rows(obj):
    """{section: (parse_rows rows, {addr: (type, symbol)}, {addr: RAW ops})} of the .o.
    The raw ops are needed because parse_rows replaces with RELOC exactly the
    immediate that has to be read here (the addend split between GOT16 and LO16)."""
    import subprocess
    txt = subprocess.run([OBJDUMP, '-z', '-dr', obj], capture_output=True, text=True).stdout
    per = {}
    cur = None
    for line in txt.splitlines():
        m = SECTION_RE.match(line)
        if m:
            cur = m.group(1)
            per[cur] = ([], {}, {})
            continue
        if cur is None:
            continue
        r = RELOC_LINE.match(line)
        if r:
            per[cur][1][int(r.group(1), 16)] = (r.group(2), r.group(3))
        elif INSN_LINE.match(line):
            per[cur][0].append(line)
            lm = normalize_asm.LINE.match(line)
            if lm:
                per[cur][2][int(lm.group(1), 16)] = lm.group(3)
    return {sec: (normalize_asm.parse_rows(lines), rel, raw) for sec, (lines, rel, raw) in per.items()}


def last_imm(ops):
    ms = list(IMM.finditer(ops))
    return int(ms[-1].group(1), 0) if ms else None


def s16(v):
    return v - 0x10000 if v & 0x8000 else v


# --------------------------------------------------------------------------- #
def read_units():
    import check
    return check.read_units()


def do_unit(unit, orig, check_json, hints=None):
    src, flags, asflags, skip = unit
    base = re.sub(r'[^A-Za-z0-9_]', '_', src)
    obj = os.path.join(ROOT, f'build/check/{base}/out.o')
    if not os.path.exists(obj):
        return None, [f'[X] {src}: {obj} is missing (run tools/check.py first)']
    o = Elf(obj)
    log = []
    syms = o.symbols()
    secidx = {i: s for i, s in enumerate(o.secs)}
    # data sections of the .o
    dsecs = {s['nm']: s for s in o.secs
             if s['flags'] & SHF_ALLOC and not (s['flags'] & SHF_EXECINSTR)
             and host_section(s['nm']) and s['size']}
    if not dsecs:
        return {}, log

    anchors = {n: [] for n in dsecs}      # section -> [(our_off, orig_addr, source)]

    # 1. anchors by name
    for name, value, size, typ, bind, shndx in syms:
        if typ == STT_OBJECT and shndx in secidx and secidx[shndx]['nm'] in dsecs \
                and name in orig.objects:
            anchors[secidx[shndx]['nm']].append((value, orig.objects[name][0], f'name {name}'))

    # 2. anchors from the code of matched functions, including the typeinfo `__tf`
    # functions in `.gnu.linkonce.t.*` (the only functions referencing the base
    # lists in `.data`), provided the function gate credited them to this unit.
    # Only the `__tf`: opening it to all weak functions, the deduplicated STL
    # copies (credited by content to any unit) misplaced small `.rodata` blocks
    # ("bad_alloc"...).
    text_funcs = {}
    for name, value, size, typ, bind, shndx in syms:
        if typ == STT_FUNC and size and shndx in secidx:
            nm = secidx[shndx]['nm']
            if nm == '.text' or (nm.startswith('.gnu.linkonce.t.') and name.startswith('__tf')):
                text_funcs.setdefault(nm, []).append((value, size, name))
    # original address of each function of the .o (named: universe; local: check.json)
    local_addr = {}
    for sym, r in check_json.items():
        if r.get('unit') == src and r.get('ok') and r.get('local'):
            local_addr[r['local']] = orig.funcs[sym][0]
    rows_by_sec = our_text_rows(obj)
    for tsec_name, tfuncs in sorted(text_funcs.items()):
        if tsec_name not in rows_by_sec:
            continue
        trows, trel, traw = rows_by_sec[tsec_name]
        taddrs = [r[0] for r in trows]
        for fstart, fsize, fname in sorted(tfuncs):
            if fname in orig.funcs and check_json.get(fname, {}).get('ok') and check_json[fname]['unit'] == src:
                oaddr = orig.funcs[fname][0]
            elif fname in local_addr:
                oaddr = local_addr[fname]
            else:
                if DEBUG:
                    log.append(f'      ? {fname[:50]}: not matched in this unit')
                continue
            orows = orig_rows(oaddr)
            if orows is None:
                if DEBUG:
                    log.append(f'      ? {fname[:50]}: expected/asm missing for 0x{oaddr:08X} (make split)')
                continue
            i = bisect_right(taddrs, fstart - 1)
            j = bisect_right(taddrs, fstart + fsize - 1)
            ours = normalize_asm.normalize_rows(trows[i:j], with_addrs=True)
            theirs = normalize_asm.normalize_rows(orows, with_addrs=True)
            if len(ours) != len(theirs):
                if DEBUG:
                    log.append(f'      ? {fname[:50]}: rows {len(ours)} vs {len(theirs)}')
                continue
            idx_of = {a: k for k, (a, _) in enumerate(ours)}
            # GOT16(section) + LO16(section) pairs in offset order
            rel_sorted = sorted((a, t, s) for a, (t, s) in trel.items()
                                if fstart <= a < fstart + fsize)
            pending = None
            for a, t, s in rel_sorted:
                if t == 'R_MIPS_GOT16' and s in dsecs:
                    pending = (a, s)
                elif t == 'R_MIPS_LO16' and pending and pending[1] == s:
                    ga, sec = pending
                    pending = None
                    if ga not in idx_of or a not in idx_of:
                        continue
                    # our addend
                    hi = last_imm(traw.get(ga, ''))
                    lo = last_imm(traw.get(a, ''))
                    if hi is None or lo is None:
                        continue
                    our_off = ((hi << 16) + s16(lo & 0xffff)) & 0xffffffff
                    # original: same row -> lw rX,OFF(gp) -> page; LO16 row -> D
                    oa_g, oa_l = theirs[idx_of[ga]][0], theirs[idx_of[a]][0]
                    og = next((r for r in orows if r[0] == oa_g), None)
                    ol = next((r for r in orows if r[0] == oa_l), None)
                    if not og or not ol or og[1] != 'lw':
                        continue
                    m = GPOFF.search(og[2])
                    d = last_imm(ol[2])
                    if not m or d is None:
                        continue
                    page = orig.got(int(m.group(1)))
                    if page is None:
                        continue
                    if tsec_name != '.text' and host_section(sec) != '.data':
                        # a `__tf` is deduplicated across TUs: every TU that emits
                        # it has its typeinfo name in `.rodata`
                        continue
                    anchors[sec].append((our_off, (page + d) & 0xffffffff, f'code {fname}'))
                elif t != 'R_MIPS_LO16':
                    pending = None

    # 3. delta per section: for all of them first (a relocated .data word pointing
    #    into .rodata needs the .rodata delta, whatever their order)
    results = {}
    sec_delta = {}
    for sec, s in dsecs.items():
        hs = host_section(sec)
        res = {'host': hs, 'size': s['size'], 'anchors': len(anchors[sec]), 'skip': f'data:{sec}' in skip,
               'align': s['align']}
        deltas = {}
        for our_off, oaddr, why in anchors[sec]:
            deltas.setdefault(oaddr - our_off, []).append(why)
        results[sec] = res
        if hints and sec in hints and (not deltas or sec in ('.ctors', '.dtors')):
            # no anchor, but link order places it (step 2 of main): the block
            # sits between two placed blocks of the neighbouring units
            deltas = {hints[sec]: ['link order']}
            res['by_link_order'] = True
        if not deltas:
            res['state'] = 'unanchored'
            continue
        delta = max(deltas, key=lambda k: len(deltas[k]))
        if len(deltas) > 1:
            # disagreeing anchors: the objects of the section do not sit at the
            # same relative offsets as in the original (a function in skip in
            # the same TU, function order in our file, extra or missing
            # literals). Comparing the block against one delta would give a
            # meaningless DIFF: it is layout debt, measured and not judged
            # (like "out of alignment" for statics). The majority delta stays
            # available to resolve the relocated words of the other sections.
            res['conflict'] = {hex(k): v[:3] for k, v in deltas.items()}
            # all anchors (our offset, original address), to tell which object
            # is out of place
            res['anchor_list'] = sorted({(o_, a_) for o_, a_, _ in anchors[sec]})
            res['state'] = 'conflict'
            res['addr'] = delta
            sec_delta[sec] = delta
            if VERBOSE:
                log.append(f'conflict {src} {sec}: anchors disagree ({len(deltas)} deltas over {len(anchors[sec])} anchors)')
            continue
        res['addr'] = delta
        sec_delta[sec] = delta
    # 3bis. Anchors from data: a section without anchors (strings no function
    # references, only a table) is placed from the R_MIPS_32 words of an already
    # placed section of the same unit pointing to it: the original word at the
    # same offset is the real address of the target (e.g. ssl_err.c: its .rodata
    # is reached only through the 320 words of its .data). All words must give
    # the same delta.
    for sec, s in dsecs.items():
        res = results[sec]
        if res.get('state') != 'unanchored' or s['type'] == SHT_NOBITS:
            continue
        sidx = next((i for i, x in secidx.items() if x is s), None)
        dd = {}
        for ref, rdelta in sec_delta.items():
            if ref == sec or results[ref].get('state') == 'conflict' or dsecs[ref]['type'] == SHT_NOBITS:
                continue
            ours_ref = o.bytes_of(dsecs[ref])
            theirs_ref = orig.bytes(rdelta, dsecs[ref]['size'])
            if theirs_ref is None:
                continue
            for off, symidx, rtype in o.relocs(ref):
                if rtype != R_MIPS_32 or off + 4 > len(ours_ref):
                    continue
                name, value, size, typ, bind, shndx = syms[symidx]
                if typ != STT_SECTION or shndx != sidx:
                    continue
                add = struct.unpack_from('<I', ours_ref, off)[0]
                tgt = struct.unpack_from('<I', theirs_ref, off)[0]
                dd.setdefault((tgt - add) & 0xffffffff, []).append(ref)
        if len(dd) == 1:
            delta = next(iter(dd))
            res.update({'addr': delta, 'anchors': len(dd[delta]), 'by_data': True})
            res.pop('state', None)
            sec_delta[sec] = delta
    # 3ter. Search by pointers: a section still without anchors, with R_MIPS_32
    # words to already placed sections of this unit (a .data pointing to its
    # .rodata), has known values at those offsets: search the original for the
    # only position (aligned, inside the host section) where they all appear. At
    # least 2 pointers, and the position must be unique: otherwise it stays
    # unanchored.
    for sec, s in dsecs.items():
        res = results[sec]
        if res.get('state') != 'unanchored' or s['type'] == SHT_NOBITS:
            continue
        ours_b = o.bytes_of(s)
        want = []
        for off, symidx, rtype in o.relocs(sec):
            if rtype != R_MIPS_32 or off + 4 > len(ours_b):
                continue
            name, value, size, typ, bind, shndx = syms[symidx]
            word = struct.unpack_from('<I', ours_b, off)[0]
            if typ == STT_SECTION and shndx in secidx and secidx[shndx]['nm'] in sec_delta \
                    and results.get(secidx[shndx]['nm'], {}).get('state') != 'conflict':
                want.append((off, (sec_delta[secidx[shndx]['nm']] + word) & 0xffffffff))
            elif typ == STT_FUNC and name in orig.funcs:
                want.append((off, (orig.funcs[name][0] + word) & 0xffffffff))
        if len(want) < 2:
            continue
        hsec = orig.sections.get(res['host'])
        if not hsec:
            continue
        data = orig.bytes(hsec['addr'], hsec['size'])
        al = max(s['align'] or 1, 4)
        off0, v0 = want[0]
        needle = struct.pack('<I', v0)
        cands = []
        i = data.find(needle)
        while i >= 0 and len(cands) < 2:
            base = i - off0
            if base >= 0 and (hsec['addr'] + base) % al == 0 and base + s['size'] <= len(data) and \
                    all(struct.unpack_from('<I', data, base + o_)[0] == v for o_, v in want):
                cands.append(hsec['addr'] + base)
            i = data.find(needle, i + 1)
        if len(cands) == 1:
            res.update({'addr': cands[0], 'anchors': len(want), 'by_pointers': True})
            res.pop('state', None)
            sec_delta[sec] = cands[0]
    # 4. comparison
    for sec, s in dsecs.items():
        res = results[sec]
        hs = res['host']
        if 'addr' not in res or res.get('state') == 'conflict':
            continue
        delta = res['addr']
        if s['type'] == SHT_NOBITS:
            res['state'] = 'bss'
            continue
        # inside the right original section?
        osec = orig.section_of(delta)
        oend = orig.section_of(delta + s['size'] - 1) if s['size'] else osec
        if osec != hs or oend != hs:
            res['state'] = 'out-of-section'
            res['orig_section'] = osec
            continue
        # word by word with relocations (R_MIPS_32: S+A; R_MIPS_GPREL32,
        # the switch jump tables in .rodata: S+A-gp)
        ours = o.bytes_of(s)
        theirs = orig.bytes(delta, s['size'])
        if theirs is None:
            res['state'] = 'out-of-section'
            continue
        rel = {off: (symidx, typ) for off, symidx, typ in o.relocs(sec)}
        eq = masked = bad = 0
        first_bad = None
        n = s['size']
        off = 0
        while off < n:
            if off in rel and rel[off][1] in (R_MIPS_32, R_MIPS_GPREL32) and off + 4 <= n:
                symidx, rtype = rel[off]
                name, value, size, typ, bind, shndx = syms[symidx]
                word = struct.unpack_from('<I', ours, off)[0]
                base_addr = None
                if typ == STT_SECTION and shndx in secidx:
                    tsec = secidx[shndx]['nm']
                    if tsec in sec_delta:
                        base_addr = sec_delta[tsec]
                    elif tsec == '.text':
                        # function of the .o at that offset -> original address
                        tf = sorted(text_funcs.get('.text', []))
                        k = bisect_right([f[0] for f in tf], word) - 1
                        if k >= 0:
                            fs, fz, fn = tf[k]
                            oa = orig.funcs[fn][0] if (fn in orig.funcs and check_json.get(fn, {}).get('ok')) \
                                else local_addr.get(fn)
                            if oa is not None:
                                base_addr = oa - fs
                elif name in orig.funcs:
                    base_addr = orig.funcs[name][0]
                elif name in orig.objects:
                    base_addr = orig.objects[name][0]
                exp = struct.unpack_from('<I', theirs, off)[0]
                if base_addr is not None and rtype == R_MIPS_GPREL32:
                    base_addr -= GP
                if base_addr is None and typ != STT_SECTION and shndx == 0 \
                        and name and name not in orig.dynnames:
                    # an external symbol the original does not have at all
                    # (neither defined nor imported): the word cannot be masked,
                    # in the fork that field is another value (typically NULL),
                    # e.g. EVP_CIPHER_get/set_asn1_iv in e_cbc_d.c/e_cbc_3d.c
                    bad += 4
                    first_bad = first_bad if first_bad is not None else off
                elif base_addr is None:
                    masked += 4
                elif ((base_addr + word) & 0xffffffff) == exp:
                    eq += 4
                else:
                    bad += 4
                    first_bad = first_bad if first_bad is not None else off
                off += 4
                continue
            if ours[off] == theirs[off]:
                eq += 1
            else:
                bad += 1
                first_bad = first_bad if first_bad is not None else off
            off += 1
        # Alignment tail: our gas pads every section to 16 with zeros; in the
        # original, if the next unit's section is 4-aligned, ld places it right
        # after the content (e.g. natives_dat.cpp: 944 B of .rodata here, while
        # the original already has "13thPosixThread" of the next section at
        # +0x3ac). The trailing zeros (from an offset multiple of 4, less than
        # 16 B, no relocations) are not data of the unit: if the only
        # differences are there, the block is equal and the tail does not count.
        if bad:
            t = n
            while t > 0 and ours[t - 1] == 0:
                t -= 1
            t = (t + 3) & ~3
            # never inside an object with a symbol (a vtable declaring 16 B and
            # ending with a zero entry is data, not padding)
            sidx = next((i for i, x in secidx.items() if x is s), None)
            t = max([t] + [v + z for _, v, z, ty, _, sh in syms if sh == sidx and ty == STT_OBJECT])
            if n - t >= 16 and t < n and ours[t] == 0 and t > 0 and ours[t - 1] != 0:
                # a string ending at 3 mod 4 has its NUL in the next word: the
                # terminator is data, the tail starts after its word (e.g.
                # amxmath.cpp `"sqrt"`)
                t += 4
            if n - t < 16 and first_bad >= t and not any(t <= k < n for k in rel):
                # the part of the tail before the first difference is padding
                # equal on both sides (zeros up to the next .o) and stays
                # verified; only from there on it does not count (e.g.
                # theimagedecoder.cpp, 4 B of zeros before "bad_alloc")
                k0 = next(k for k in range(t, n) if ours[k] != theirs[k])
                tail_eq = sum(1 for k in range(k0, n) if ours[k] == theirs[k])
                eq, bad, first_bad = eq - tail_eq, 0, None
                res['tail'] = n - k0
        res.update({'eq': eq, 'masked': masked, 'bad': bad, 'first_bad': first_bad,
                    'state': 'ok' if bad == 0 else 'fail'})
        if bad and res.get('by_link_order'):
            # placed only by link order and different: the position is
            # inferred, not proven by an anchor, so it is measured, not judged
            res['state'] = 'order-differs'
        if VERBOSE and bad:
            for our_off, oaddr, why in anchors[sec][:8]:
                log.append(f'        anchor {sec}+0x{our_off:x} -> 0x{oaddr:08X}  ({why})')
        if VERBOSE or (res['state'] == 'fail' and not res['skip']):
            tag = 'ok  ' if bad == 0 else ('skip' if res['skip'] else 'DIFF')
            log.append(f'{tag} {src} {sec}: {eq}/{n} B equal'
                       + (f', {masked} masked' if masked else '')
                       + (f', first difference at +0x{first_bad:x} (orig 0x{delta + first_bad:08X})' if bad else '')
                       + (f'  anchor conflict {res["conflict"]}' if 'conflict' in res else ''))
    return results, log


def populate_skips():
    import subprocess
    d = json.load(open(os.path.join(ROOT, OUT)))
    n = 0
    for unit, secs in sorted(d['units'].items()):
        names = [f'data:{sec}' for sec, r in secs.items() if r.get('state') == 'fail' and not r.get('skip')]
        if not names:
            continue
        r = subprocess.run([sys.executable, 'tools/unitedit.py', unit, '--skip-add', ','.join(names)],
                           cwd=ROOT, capture_output=True, text=True)
        if r.returncode:
            print(f'error: {unit}: {r.stderr.strip() or r.stdout.strip()}')
        else:
            n += len(names)
    print(f'{n} differing data blocks put in skip')


def summary(d):
    """Summary per original section. Everything as a union of intervals (two
    units placing the same block -- STL dedup, same-TU stubs -- count once):
    `located` = bytes covered by a placed block, `eq` = bytes covered by an
    equal block, `bad`/`skipbad` = differing bytes in the DIFF blocks (not in
    skip / in skip), `masked` = words with a relocation to an unknown base."""
    tot = {n: d['orig'][n] for n in DATA_UNIVERSE}
    agg = {n: {'located': 0, 'eq': 0, 'bad': 0, 'skipbad': 0, 'masked': 0, 'union': 0}
           for n in DATA_UNIVERSE}
    loc = {n: [] for n in DATA_UNIVERSE}
    good = {n: [] for n in DATA_UNIVERSE}
    for unit, secs in d['units'].items():
        for sec, r in secs.items():
            hs = r['host']
            if hs not in agg or r['state'] not in ('ok', 'fail'):
                continue
            a = agg[hs]
            # the alignment tail not compared (`tail`) is neither verified nor
            # placed: the bytes after it belong to the next .o, which places
            # them itself
            end = r['addr'] + r['size'] - (r.get('tail') or 0)
            a['located'] += end - r['addr']
            a['masked'] += r['masked']
            loc[hs].append((r['addr'], end))
            if r['state'] == 'ok':
                good[hs].append((r['addr'], end))
            else:
                a['skipbad' if r['skip'] else 'bad'] += r['bad']

    def union(iv):
        u, prev = 0, None
        for a, b in sorted(iv):
            if prev is not None and a < prev:
                a = prev
            if b > a:
                u += b - a
                prev = b
        return u
    for hs in DATA_UNIVERSE:
        agg[hs]['union'] = union(loc[hs])
        agg[hs]['eq'] = union(good[hs])
    return tot, agg


def link_order_hints(units, results, orig, check_json):
    """Step 2: place blocks without anchors by link order. ld concatenates the
    same input section (`.rodata`, `.data`, `.ctors`, `.dtors`) of the .o files
    in link order, which is the order of their .text (units sorted by function
    address). A chain of unanchored blocks between two placed (ok/fail) blocks
    of the same section is laid out in sequence with each one's alignment; the
    chain is valid only if it ends exactly where the next block starts (it
    checks itself: a unit that has data in the original that we do not emit
    makes it fail). -> {unit: {section: address}}"""
    # order key: the median of the unit's function addresses, not the first:
    # deduplicated template instances are credited to a unit even when in the
    # binary they sit in another TU's .text, and the first function got the
    # order wrong. For a unit with static constructors the key is the address
    # of its `_GLOBAL_$I$...` (gcc emits it at the end of the TU's .text, and it
    # is what the .ctors entry points to).
    addrs, gl = {}, {}
    for sym, r in check_json.items():
        u = r.get('unit')
        if u and sym in orig.funcs:
            addrs.setdefault(u, []).append(orig.funcs[sym][0])
            if r.get('ok') and str(r.get('local', '')).startswith('_GLOBAL_$I$'):
                gl[u] = orig.funcs[sym][0]
    key = {u: gl.get(u, sorted(v)[len(v) // 2]) for u, v in addrs.items()}
    order = sorted((u for u in key if u in results), key=key.get)
    # second key for .rodata/.data: the unit's first function. The two keys
    # fail on different units; each chain checks itself (it must close exactly
    # on the next placed block), so the union is taken
    first = {u: min(v) for u, v in addrs.items()}
    order_first = sorted((u for u in first if u in results), key=first.get)
    hints = {}
    import dataregions
    region_ends = sorted(b for a, b, _, _ in dataregions.regions())
    # .ctors/.dtors: one 4-byte entry per unit with static constructors/
    # destructors, from the start to the end of the section (crtstuff first,
    # crtstuff_end last): the whole chain is inferred, and valid only if it
    # fills the section exactly. Here the order wins over the anchors (the one
    # of crtstuff_end, from its code, falls outside the section).
    for sec in ('.ctors', '.dtors'):
        os_ = orig.sections.get(sec)
        chain = [(u, results[u][sec]) for u in order if sec in results[u]]
        if not os_ or not chain:
            continue
        pos, place = os_['addr'], []
        for u, r in chain:
            al = max(r.get('align') or 1, 1)
            pos = (pos + al - 1) // al * al
            place.append((u, pos))
            pos += r['size']
        if pos == os_['addr'] + os_['size']:
            for u, a in place:
                hints.setdefault(u, {})[sec] = a
    for sec, order_ in [(x, o_) for x in ('.rodata', '.data') for o_ in (order, order_first)]:
        chain = [(u, results[u][sec]) for u in order_ if sec in results[u]]
        run, prev_end = [], None
        for u, r in chain:
            if r.get('state') in ('ok', 'fail') and 'addr' in r:
                if run and prev_end is not None:
                    pos, place = prev_end, []
                    for ru, rr in run:
                        al = max(rr.get('align') or 1, 1)
                        pos = (pos + al - 1) // al * al
                        place.append((ru, pos))
                        pos += rr['size']
                    al = max(r.get('align') or 1, 1)
                    if (pos + al - 1) // al * al == r['addr']:
                        for ru, a in place:
                            hints.setdefault(ru, {}).setdefault(sec, a)
                        run = []
                if run:
                    # the chain may also start from the end of a non-compiled
                    # region (config/data_regions.txt: asset tables none of our
                    # blocks emits) between the previous block and this one,
                    # with a suffix of the chain (e.g. the -pg counters after
                    # the DNAS2 tables); valid only if it closes exactly
                    al = max(r.get('align') or 1, 1)
                    for e in region_ends:
                        if not ((prev_end or 0) <= e < r['addr']):
                            continue
                        for k in range(len(run)):
                            pos, place = e, []
                            for ru, rr in run[k:]:
                                a2 = max(rr.get('align') or 1, 1)
                                pos = (pos + a2 - 1) // a2 * a2
                                place.append((ru, pos))
                                pos += rr['size']
                            if (pos + al - 1) // al * al == r['addr']:
                                for ru, a in place:
                                    hints.setdefault(ru, {}).setdefault(sec, a)
                                break
                run, prev_end = [], r['addr'] + r['size']
            elif r.get('state') == 'unanchored':
                run.append((u, r))
            else:
                # conflict / out of section: its extent is unknown, the chain breaks
                run, prev_end = [], None
    return hints


def content_hints(units, results, orig):
    """Step 3: place by content a `.rodata`/`.data` block still without anchors
    and without relocations, when its content (without the trailing zeros, at
    least 8 B) appears in the original, at the section's alignment, in one
    place not already covered by a placed block or a non-compiled region, and
    no other unit claims that place. Same rule as the attribution by content of
    the statics (tools/check.py): the bytes are verified, the address is
    inferred, and the block says so (`by_content: true`)."""
    import dataregions
    cov = [(r['addr'], r['addr'] + r['size']) for res in results.values() for r in res.values()
           if r.get('addr') is not None and r.get('state') in ('ok', 'fail', 'conflict')
           and r.get('host') in ('.rodata', '.data')]
    cov += [(a, b) for a, b, _, _ in dataregions.regions()]
    secbytes = {h: orig.bytes(orig.sections[h]['addr'], orig.sections[h]['size'])
                for h in ('.rodata', '.data')}
    cands = []
    for unit in units:
        src = unit[0]
        res = results.get(src) or {}
        cand = [sec for sec, r in res.items() if r.get('state') == 'unanchored'
                and r.get('host') in ('.rodata', '.data')]
        if not cand:
            continue
        base = re.sub(r'[^A-Za-z0-9_]', '_', src)
        try:
            o = Elf(os.path.join(ROOT, 'build', 'check', base, 'out.o'))
        except Exception:
            continue
        for sec in cand:
            s = next((x for x in o.secs if x['nm'] == sec), None)
            if s is None or list(o.relocs(sec)):
                continue
            core = o.bytes_of(s).rstrip(b'\0')
            if len(core) >= 8:
                cands.append((src, sec, res[sec]['host'], core, max(s['align'] or 1, 1)))
    # from the longest content to the shortest, and taken places count as
    # covered for the next ones: a block of generic literals only ("bad_alloc")
    # does not contend the place with one that contains it and is unique there
    hints = {}
    for n in sorted({len(c[3]) for c in cands}, reverse=True):
        claims = {}
        for src, sec, h, core, al in (c for c in cands if len(c[3]) == n):
            base_a = orig.sections[h]['addr']
            hits, i = [], secbytes[h].find(core)
            while i >= 0:
                a = base_a + i
                if a % al == 0 and not any(x < a + n and a < y for x, y in cov):
                    hits.append(a)
                i = secbytes[h].find(core, i + 1)
            if len(hits) == 1:
                claims.setdefault((h, hits[0]), []).append((src, sec))
        for (h, a), who in claims.items():
            if len(who) == 1:
                src, sec = who[0]
                hints.setdefault(src, {})[sec] = a
                cov.append((a, a + n))
    return hints


def main():
    if '--populate-skips' in sys.argv:
        return populate_skips()
    orig = Orig()
    try:
        check_json = json.load(open(os.path.join(ROOT, 'build/check.json')))
    except FileNotFoundError:
        sys.exit('data       error: build/check.json is missing: run tools/check.py first')
    # Scoped gate: the anchors come from the functions matched in this run,
    # which are in the scoped check (build/check-<base>.json, same name as
    # check.py), not in the global check.json, which may be stale. Without the
    # overlay a data block anchored only by a freshly matched function would be
    # unanchored in the scoped run and DIFF in the full gate.
    if _UNITS != 'config/units.txt':
        scoped = os.path.join(ROOT, 'build/check-%s.json' % re.sub(
            r'[^A-Za-z0-9_.-]', '_', os.path.basename(_UNITS).rsplit('.', 1)[0]))
        if os.path.exists(scoped):
            check_json.update(json.load(open(scoped)))
        else:
            print(f'warning: {os.path.relpath(scoped, ROOT)} is missing: anchors from the global check.json')
    units = read_units()
    n_asm = build_asm_index()
    if not n_asm:
        print('warning: expected/asm/ is empty: without `make split` there are no anchors from the code')
    only = sys.argv[sys.argv.index('--unit') + 1] if '--unit' in sys.argv else None
    if only:
        units = [u for u in units if u[0] == only]
    info(f'data: {len(units)} units')
    out = {'orig': {n: orig.sections[n]['size'] for n in DATA_UNIVERSE if n in orig.sections},
           'units': {}}
    failed = []
    shown = 0
    from concurrent.futures import ThreadPoolExecutor
    with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
        for unit, (res, log) in zip(units, pool.map(lambda u: do_unit(u, orig, check_json), units)):
            for line in log:
                if not STATS and line.startswith('DIFF'):
                    shown += 1
                    if shown > 20:
                        continue
                print(line)
            if res is None:
                continue
            out['units'][unit[0]] = res
    # step 2: link order (full gate only: it needs the blocks of the neighbouring units)
    if not only and _UNITS == 'config/units.txt':
        hints = link_order_hints(units, out['units'], orig, check_json)
        redo = [u for u in units if u[0] in hints]
        with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
            for unit, (res, log) in zip(redo, pool.map(
                    lambda u: do_unit(u, orig, check_json, hints[u[0]]), redo)):
                if res is not None:
                    out['units'][unit[0]] = res
        info(f'link order: {sum(len(h) for h in hints.values())} blocks without anchors placed')
        # step 3: by content, on the blocks still without anchors
        chints = content_hints(units, out['units'], orig)
        redo = [u for u in units if u[0] in chints]
        with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
            for unit, (res, log) in zip(redo, pool.map(
                    lambda u: do_unit(u, orig, check_json, {**hints.get(u[0], {}), **chints[u[0]]}),
                    redo)):
                if res is not None:
                    for sec in chints[unit[0]]:
                        if sec in res:
                            res[sec]['by_content'] = True
                    out['units'][unit[0]] = res
        info(f'by content: {sum(len(h) for h in chints.values())} blocks without anchors placed')
    for u, res in out['units'].items():
        for sec, r in res.items():
            if r['state'] == 'fail' and not r['skip']:
                failed.append((u, sec))
    os.makedirs(os.path.join(ROOT, 'build'), exist_ok=True)
    # --unit measures one unit: it writes next to the gate JSON, never over it
    dest = OUT if not only else OUT[:-len('.json')] + '-unit-%s.json' % re.sub(
        r'[^A-Za-z0-9_.-]', '_', only)
    json.dump(out, open(os.path.join(ROOT, dest), 'w'), indent=1)
    tot, agg = summary(out)
    nstates = {}
    for secs in out['units'].values():
        for r in secs.values():
            nstates[r['state']] = nstates.get(r['state'], 0) + 1
    if not STATS and shown > 20:
        print(f'... and {shown - 20} more, see {dest}')
    secs = [hs for hs in DATA_UNIVERSE if tot[hs]]
    eq_b = sum(agg[hs]['eq'] for hs in secs)
    line = (f'data       {nstates.get("ok", 0)} blocks match ({" ".join(secs)}, {eq_b:,} B equal)'
            + (f', {len(failed)} differ' if failed else ''))
    if _UNITS != 'config/units.txt' or only:
        line += f' (scoped: {dest})'
    print(line)
    info('blocks by state: ' + ', '.join(f'{"DIFF" if k == "fail" else k} {v}'
                                         for k, v in sorted(nstates.items())))
    for hs in DATA_UNIVERSE:
        a = agg[hs]
        info(f'   {hs:8s} {tot[hs]:8,} B original, placed {a["union"]:8,} B '
             f'({100 * a["union"] / tot[hs] if tot[hs] else 0:5.1f}%), equal {a["eq"]:8,} B,'
             f' masked {a["masked"]:6,}, red {a["bad"]:6,} (+{a["skipbad"]:,} in skip)')
    info(f'   -> {dest}')
    sys.exit(1 if failed else 0)


if __name__ == '__main__':
    main()
