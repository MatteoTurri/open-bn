#!/usr/bin/env python3
"""The decompilation gate: compile every unit and check the matches.

1. Check the SHA1 of orig/bn-jpn.elf against config/bn.yaml.
2. For each unit in config/units.txt: compile with the Sony toolchain, find the
   functions defined in the .o that also exist in the binary's symbol table and
   compare them one by one (same pipeline as tools/match.sh). The local
   functions of the .o (static, or non-exported globals) are compared too: their
   original address comes from the anchors (the .dynsym functions of the same
   .o, tools/statics.py), from data anchors or by content, and their gate name
   is `static_<ADDR>` (config/static_funcs.txt, universe).
3. Write build/check.json and summarize. Exit 1 if something does not match.

Runs in the container: python3 tools/check.py [--verbose] [-jN]

Performance. The dominant cost is the number of processes, not the compilation
(every process is emulated: the container is linux/amd64, under Rosetta/qemu on
arm64 hosts). Hence:

- **in-process normalization**: `normalize_asm` is imported as a module
  (`normalize_asm.normalize_rows`) instead of being spawned per function.
- **original side disassembled once and cached on disk**: the binary never
  changes, so `objdump -z -d` on the ELF runs once, is sliced per symbol in
  memory and the normalized result goes to
  `build/cache/orig-<sha1elf>-<hash normalize_asm>.json.gz`. The key includes
  the hash of the normalizer, so changing it invalidates the cache by itself
  (otherwise comparisons would mix two different rules: silent false matches).
- **our side disassembled once per unit** (`objdump -z -dr` of the whole .o,
  then sliced by address) instead of once per symbol. If a symbol is not found
  (alias at the same address) it falls back to `--disassemble=SYM`.
- the units are independent -> thread pool (the work is all in subprocesses, so
  the GIL is not a constraint). Output and `results` are reassembled in
  units.txt order to stay deterministic.
"""
import gzip
import hashlib
import json
import os
import re
import shutil
import struct
import subprocess
import sys
from bisect import bisect_left
from collections import Counter
from concurrent.futures import ThreadPoolExecutor

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import normalize_asm                                              # noqa: E402
import gotident
import dnas_subst
import srcpath

ELF = 'orig/bn-jpn.elf'
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import universe
import statics

SYMTAB = 'config/symbol_addrs.txt'
UNITS = os.environ.get('UNITS', 'config/units.txt')
YAML = 'config/bn.yaml'
TOOL = os.environ.get('TOOL', 'docker/toolchain/sony')
INCROOT = 'docker/toolchain/include'
OBJDUMP = 'mipsel-linux-gnu-objdump'
AS = 'docker/toolchain/sonybinutils/mipsEEel-linux-as'  # binutils 2.9EE Sony
VERBOSE = '--verbose' in sys.argv     # one line per function, too
# statistics that need no action: only with VERBOSE=1 (environment) or -v/--verbose
STATS = VERBOSE or '-v' in sys.argv or os.environ.get('VERBOSE', '') not in ('', '0')


def info(msg):
    if STATS:
        print(msg)
CACHE_DIR = 'build/cache'
# functions called without a declaration (see do_unit): (unit, name)
IMPLICIT = []
MEASURE_SKIPS = True     # also measure the skip-listed symbols (-> build/skip_distance.json)
PROTECTED = set()        # filled by orig_cache(): DNAS self-modifying functions (decrypted reference)
STATIC_LOG = {}          # {unit: [locals: name, address, size, state]} -> build/check_statics.json
UNIT_ORPHANS = {}        # {unit: orphan linkonce copies of the .o} (identity of the LO16 targets)
BY_SIZE = {}             # size -> [statics of the universe]: identity by content (do_unit)
NROWS = {}               # number of normalized original rows -> [statics]: see content_hits
# Orphan linkonce copies: on by default; CHECK_ORPHAN_LINKONCE=0 turns it off
# (see orphan_linkonce).
ORPHAN_LINKONCE = os.environ.get('CHECK_ORPHAN_LINKONCE', '1') != '0'
# last function before the original's `.gnu.linkonce.t.*` region: `init_dummy` of
# ref/gcc-2.95.2/crtstuff_end.c (the `crtend` of tools/layoutcheck.py); the region
# starts where it ends. Checked at run time in linkonce_lo().
CRTSTUFF_END_LAST = 'static_00911668'


def dnas_ref_mod():
    import dnas_ref
    return dnas_ref
KEEP_ROWS = False        # keep the normalized rows of the skips in the results (LCS triage)

INC_C = f'-nostdinc -Isrc -I{INCROOT}/fixinc -I{INCROOT}/ginclude -Idocker/kit/rootfs/usr/include'
# The libstdc++ headers are included from the path of Sony's installation: the
# `__FILE__` of the asserts of <std/bastring.cc> ends up in .rodata, and the original has
# "/usr/cross/mipsEEel-linux/include/g++-3/std/bastring.cc" in 9 places.
# In the container the path is a link to INCROOT/libstdc++ (cross_include()).
CROSS_CXX = '/usr/cross/mipsEEel-linux/include/g++-3'


def cross_include():
    """Create the link CROSS_CXX -> INCROOT/libstdc++. Exits if it cannot: without the link every C++
    unit fails with a "string: No such file or directory" that does not tell the cause (with a non-root
    container the directory must be created writable in the image, docker/Dockerfile)."""
    target = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                          INCROOT, 'libstdc++')
    try:
        os.makedirs(os.path.dirname(CROSS_CXX), exist_ok=True)
        if os.path.realpath(CROSS_CXX) != os.path.realpath(target):
            if os.path.lexists(CROSS_CXX):
                os.unlink(CROSS_CXX)
            os.symlink(target, CROSS_CXX)
    except OSError as e:
        sys.exit(f'functions  error: cannot create link {CROSS_CXX} -> {target} ({e}): old image? run make env')


SYSROOT_INC = '-Idocker/kit/rootfs/usr/include'


def unit_includes_first(inc, flags):
    """Move the -I flags of a unit in front of the kit sysroot, so that a header of the unit is not
    hidden by a system header with the same name (glibc has an err.h, SSL-C has its own)."""
    toks = flags.split()
    incs = [t for t in toks if t.startswith('-I')]
    rest = [t for t in toks if not t.startswith('-I')]
    return inc.replace(SYSROOT_INC, ' '.join(incs + [SYSROOT_INC])), ' '.join(rest)


INC_CXX = (f'-nostdinc -Isrc -I{INCROOT}/fixinc -I{CROSS_CXX} -I{INCROOT}/libstdc++/stl '
           f'-I{INCROOT}/libstdc++/std -I{INCROOT}/cpinc -I{INCROOT}/libio '
           f'-I{INCROOT}/ginclude -Idocker/kit/rootfs/usr/include')

# e.g.: 00000000 g     F .text  00000098 adler32   (l = local)
OBJ_SYM_RE = re.compile(r'^([0-9a-f]+) [ ]?(?:g|w|l)\s+F (\S+)\s+([0-9a-f]+)\s+(\S+)$')
SECTION_RE = re.compile(r'^Disassembly of section (\S+):\s*$')


def jobs():
    for a in sys.argv[1:]:
        if a.startswith('-j'):
            return max(1, int(a[2:] or 0) or os.cpu_count() or 1)
    return max(1, os.cpu_count() or 1)


def run(cmd, **kw):
    return subprocess.run(cmd, shell=True, capture_output=True, text=True, **kw)


def verify_sha1():
    want = re.search(r'sha1: ([0-9a-f]{40})', open(YAML).read())
    if not want:
        sys.exit('functions  error: sha1 missing in config/bn.yaml')
    got = hashlib.sha1(open(ELF, 'rb').read()).hexdigest()
    if got != want.group(1):
        sys.exit(f'functions  error: SHA1 of {ELF} does not match: {got} != {want.group(1)}')
    info(f'{ELF} sha1 {got[:12]} ok')
    return got


def load_symbols():
    """Raw universe (size-0 aliases included: the caller skips them where
    needed). Parser shared in tools/universe.py, the single home, so that it
    cannot diverge from progress."""
    return universe.symbols(SYMTAB)


def defined_functions(obj, symtab=None):
    """Function symbols (global/weak/local) in the .o -> {name: (addr, size, sec)}.

    All three are needed to cut the body out of the full dump of the .o exactly
    as `--disassemble=SYM` does:
    - size, otherwise the cut drags along the alignment nops between functions
      and every comparison is off by one line;
    - section, because in C++ .o files each function sits in a
      `.gnu.linkonce.t.*` starting at offset 0: without it addresses collide."""
    funcs = {}
    if symtab is None:
        symtab = run(f"{OBJDUMP} -t {obj}").stdout
    for line in symtab.splitlines():
        m = OBJ_SYM_RE.match(line)
        if m:
            funcs[m.group(4)] = (int(m.group(1), 16), int(m.group(3), 16),
                                 m.group(2))
    return funcs


def linkonce_lo(syms):
    """First address of the original's `.gnu.linkonce.t.*` region (end of crtstuff_end).
    Exits with an error if the universe no longer has the expected static (list
    regenerated or crtstuff changed): better to stop than to move the boundary silently."""
    a, sz = syms.get(CRTSTUFF_END_LAST, (None, 0))
    if not sz:
        sys.exit(f'functions  error: {CRTSTUFF_END_LAST} (init_dummy of crtstuff_end) is not in the '
                 f'universe: re-check the linkonce region boundary (tools/check.py)')
    return a + sz


def orphan_linkonce(objsyms, syms):
    """The candidate orphan linkonce copies of this .o: {name: 'dynsym' | 'strong?'}.

    Rule. A function of our .o that sits in its own comdat section
    `.gnu.linkonce.t.<name>` (`.weak` symbol) and whose <name> has a strong definition
    elsewhere cannot be, in the image, the function <name>: the symbol goes to the strong
    definition. The linker (binutils 2.9EE) still keeps the first
    `.gnu.linkonce.t.<name>` section it meets and lays it in the linkonce region, at the
    end of .text after crtstuff_end: code without a .dynsym symbol, i.e. a `static_<ADDR>`
    of the universe. That copy is therefore a candidate, by content (identical body) and
    proximity, for the statics of the linkonce region and nothing else (assign_statics,
    second pass). Strong definition elsewhere:
      'dynsym' the name is in .dynsym at an address outside the linkonce region: the
               original definition is in the .text of a TU (the linkonce sections all
               come after crtstuff_end), so no linkonce copy is that symbol;
      'strong?' the name is not in .dynsym: a candidate only if another unit of the run
               defines it strong (gotident.UNIT_DEFS), decided in run_units.
    A .dynsym name inside the linkonce region (e.g. `__tf6Object`, `_M_insert_aux`)
    is never orphan: the copy kept is the named one.

    Example (docs/compiler.md, emission order): camera.o emits `mat4_identity__Fv`
    comdat (`inline` without an effective `#pragma interface`), the symbol goes to
    vecmath.o (0x786EE8) and the section stays at 0x92CCB0 = `static_0092CCB0`, which
    returns camera.o's static matrix. The five `static_0092C3F0..0092CB60` are the copies
    of apply/multiply/mat4_translate/mat4_scale/mat4_quat emitted by object.o. Looking
    for statics only among local symbols would lose them silently."""
    if not ORPHAN_LINKONCE:
        return {}
    lo = linkonce_lo(syms)
    out = {}
    for n, (off, sz, sec) in objsyms.items():
        if not sz or sec != '.gnu.linkonce.t.' + n:
            continue
        if n in syms:                     # a size-0 alias too: the address is enough
            if syms[n][0] < lo:
                out[n] = 'dynsym'
        else:
            out[n] = 'strong?'
    return out



# --------------------------------------------------------------------------- #
# Data anchors for the locals
# --------------------------------------------------------------------------- #
_ORIG_DATA = None
_ORIG_DATA_LOCK = __import__('threading').Lock()


def _orig_data():
    global _ORIG_DATA
    with _ORIG_DATA_LOCK:
        if _ORIG_DATA is None:
            import datacheck
            _ORIG_DATA = datacheck.Orig()
        return _ORIG_DATA


def data_anchors(obj, objsyms):
    """{local: original address} proven by the data: for each object of the .o that
    has a real name in .dynsym (AMX_NATIVE_INFO tables `smallext_*`, vtables, ...),
    each R_MIPS_32 to a function of the .o says that the corresponding word of the
    original object is the address of that function. A certain identity, not one
    by content.

    Why: the natives `getstring`/`printf` of `ref/small-fmt6/amxcons.cpp` are
    `return 0` stubs with 27-28 identical candidates; without anchors an earlier
    unit would take them by content and the `smallext_cons` table, which points to
    the right ones, would turn red."""
    import datacheck as dc
    try:
        e = dc.Elf(obj)
    except (OSError, ValueError):
        return {}
    orig = _orig_data()
    syms_o = e.symbols()
    secidx = {i: s['nm'] for i, s in enumerate(e.secs)}
    # functions of the .o by (section, offset)
    fn_at = {}
    for lname, (off, sz, sec) in objsyms.items():
        fn_at[(sec, off)] = lname
    # Local objects of the .o by (section, offset): the static tables reached from a
    # named object (below)
    obj_at = {}
    for name, value, size, typ, bind, shndx in syms_o:
        if typ == dc.STT_OBJECT and size and bind == 0 and secidx.get(shndx):
            obj_at[(secidx[shndx], value)] = size
    out = {}

    def scan(sec, value, size, oaddr, follow):
        body = e.bytes_of(e.by_name[sec])
        for roff, symidx, rtype in e.relocs(sec):
            if rtype != dc.R_MIPS_32 or not (value <= roff < value + size):
                continue
            tname, tval, tsz, ttyp, tbind, tsh = syms_o[symidx]
            tsec = secidx.get(tsh)
            addend = struct.unpack_from('<I', body, roff)[0]
            if ttyp == dc.STT_SECTION:
                lname = fn_at.get((tsec, addend))
                toff = addend
            elif ttyp == dc.STT_FUNC:
                lname = tname if addend == 0 else fn_at.get((tsec, tval + addend))
                toff = None
            else:
                lname = None
                toff = tval + addend if ttyp == dc.STT_OBJECT and tbind == 0 else None
            w = orig.bytes(oaddr + (roff - value), 4)
            if not w:
                continue
            if lname:
                out[lname] = struct.unpack('<I', w)[0]
            elif follow and toff is not None and (tsec, toff) in obj_at and tsec in e.by_name:
                # One level: the word of the named object points to a static table of
                # the .o (no name in the binary); the same word of the original gives
                # that table's address, and its R_MIPS_32 to functions are identities
                # as certain as the direct ones. Example: the AM_* of BSAFE
                # (bsafe_am_*.c) -> static method table -> static methods of 20-60 B
                # with up to 38 identical candidates.
                scan(tsec, toff, obj_at[(tsec, toff)], struct.unpack('<I', w)[0], False)

    for name, value, size, typ, bind, shndx in syms_o:
        if typ != dc.STT_OBJECT or not size or name not in orig.objects:
            continue
        sec = secidx.get(shndx)
        if not sec or sec not in e.by_name:
            continue
        oaddr, osize = orig.objects[name]
        if osize != size:
            continue
        scan(sec, value, size, oaddr, True)
    return out

# --------------------------------------------------------------------------- #
# Original side: one objdump on the ELF, sliced per symbol, cached.
# --------------------------------------------------------------------------- #
def orig_cache(sha1, syms):
    """Normalized original rows for every function (cached on disk), and the global
    indexes used by the content identity of the statics (BY_SIZE, NROWS, PROTECTED).
    The indexes are filled here, not in main(): a caller of run_units from outside
    goes through orig_cache, and with empty indexes the content identity would be
    off and a skipped static could get the measurement of another local."""
    cache = _orig_rows(sha1, syms)
    PROTECTED.clear()
    PROTECTED.update(dnas_ref_mod().protected(syms) & set(syms))
    BY_SIZE.clear()
    for n, (_, sz) in syms.items():
        if sz and statics.is_static(n):
            BY_SIZE.setdefault(sz, []).append(n)
    # index by normalized rows of all the statics (see content_hits): needed
    # when the size in our .o differs from the original's because normalize_asm
    # normalized a call form.
    NROWS.clear()
    for n, rows in cache.items():
        if rows and statics.is_static(n):
            NROWS.setdefault(len(rows), []).append(n)
    return cache


def _orig_rows(sha1, syms):
    norm_h = hashlib.sha1(
        open(normalize_asm.__file__, 'rb').read()).hexdigest()[:12]
    # DNAS: for the self-modifying functions the reference is the whole function
    # of the decrypted image, with only the descriptors masked (tools/dnas_ref.py).
    # The cache key includes the decrypted image and the module, so changing them
    # invalidates it (same reason as the normalizer hash).
    import dnas_ref
    prot = dnas_ref.protected(syms) & set(syms)
    dnas_h = ''
    if prot:
        img = dnas_ref.ensure_image()
        dnas_h = '-dnas' + hashlib.sha1(open(img, 'rb').read()).hexdigest()[:8] \
            + hashlib.sha1(open(dnas_ref.__file__, 'rb').read()).hexdigest()[:6]
    # the nameless functions (tools/statics.py): the generated list enters the
    # key, so regenerating it invalidates the cache (otherwise new symbols
    # would be missing and compared against an empty list)
    st_h = ''
    if os.path.exists(statics.LIST):
        st_h = '-st' + hashlib.sha1(open(statics.LIST, 'rb').read()).hexdigest()[:8]
    path = f'{CACHE_DIR}/orig-{sha1[:12]}-{norm_h}{dnas_h}{st_h}.json.gz'
    if os.path.exists(path):
        with gzip.open(path, 'rt') as f:
            cache = json.load(f)
        info(f'orig cache: {len(cache)} functions from {path}')
        return cache

    info(f'orig cache missing: disassembling {ELF} once')
    rows = normalize_asm.parse_rows(
        run(f'{OBJDUMP} -z -d {ELF}').stdout.splitlines())
    addrs = [r[0] for r in rows]
    cache = {}
    for sym, (addr, size) in syms.items():
        if not size:
            continue
        i = bisect_left(addrs, addr)
        j = bisect_left(addrs, addr + size)
        cache[sym] = normalize_asm.normalize_rows(rows[i:j])
    if prot:
        drows = normalize_asm.parse_rows(
            run(f'{OBJDUMP} -z -d {img}').stdout.splitlines())
        desc = dnas_ref.desc_addrs(drows)
        daddrs = [r[0] for r in drows]
        for sym in prot:
            addr, size = syms[sym]
            if not size:
                continue
            i = bisect_left(daddrs, addr)
            j = bisect_left(daddrs, addr + size)
            cache[sym] = normalize_asm.normalize_rows(
                dnas_ref.reference_rows(drows[i:j], desc))
        info(f'DNAS reference for {len(prot)} functions: decrypted image, scaffolding '
             f'included ({len(desc)} descriptor words masked)')
    os.makedirs(CACHE_DIR, exist_ok=True)
    with gzip.open(path, 'wt') as f:
        json.dump(cache, f)
    info(f'orig cache written: {len(cache)} functions in {path}')
    return cache


def rows_by_section(dis_text):
    """`objdump -dr` of a whole .o -> {section: ([addresses], [parsed rows])}.

    It can not be split on the `<name>:` headers as would seem natural: the
    assembler's local labels (`$L300`, `$L301`) are symbols and objdump prints a
    header for them too, so the split would cut functions at every internal
    branch target (branches would become EXT and function tails would be
    truncated). It is sliced by address inside the section, as on the original side.
    """
    per_sec, cur = {}, None
    for line in dis_text.splitlines():
        m = SECTION_RE.match(line)
        if m:
            cur = m.group(1)
            per_sec.setdefault(cur, [])
        elif cur is not None:
            per_sec[cur].append(line)
    out = {}
    for sec, lines in per_sec.items():
        rows = normalize_asm.parse_rows(lines)
        out[sec] = ([r[0] for r in rows], rows)
    return out


def compare(orig, ours):
    """Return (match_bool, total, differing) with the RELOC wildcard.

    orig/ours are lists of already normalized rows."""
    imm = r'(-?\d+|0x[0-9a-f]+)'
    bad = 0
    for i in range(max(len(orig), len(ours))):
        a = orig[i] if i < len(orig) else '<missing>'
        b = ours[i] if i < len(ours) else '<missing>'
        if a == b:
            continue
        if 'RELOC' in b and re.match(re.escape(b).replace('RELOC', imm) + r'$', a):
            continue
        bad += 1
    return bad == 0, len(orig), bad


def validate_units():
    """Fail fast on malformed lines of config/units.txt.

    A continuation line of a comment block that lost its `#` would be parsed as
    a unit line and the gate would exit 1 with a misleading message ("compilation
    failed"): better a precise, immediate error. Duplicate sources are reported
    too, and they are not harmless: `results` is a dict (the last line wins for
    the count) but `failed` is a list appended unconditionally, so a superseded
    line that fails makes the gate exit 1 even if the new one matches.
    """
    problems, seen = [], {}
    for n, raw in enumerate(open(UNITS), 1):
        line = raw.strip()
        if not line or line.startswith('#'):
            continue
        src = line.split('|')[0].strip()
        if '|' not in line:
            problems.append(f'line {n}: no "|" (uncommented prose? missing "#"?): {line[:80]}')
        elif not os.path.exists(src):
            problems.append(f'line {n}: source "{src}" does not exist (comment without "#"?): '
                            f'{line[:80]}')
        elif src in seen:
            problems.append(f'line {n}: duplicate source "{src}" (already on line {seen[src]}): '
                            f'keep only one')
        else:
            seen[src] = n
    if problems:
        for p in problems:
            print(f'functions  error: {UNITS}: {p}')
        sys.exit(1)
    info(f'{UNITS}: {len(seen)} units, no duplicates')


def _parse_unit_line(raw):
    # format: source | cflags | asflags | skip
    parts = [p.strip() for p in raw.split('|')]
    return (parts[0],
            parts[1] if len(parts) > 1 else '',
            parts[2] if len(parts) > 2 else '',
            set(filter(None, (parts[3] if len(parts) > 3 else '').split(','))))


def read_units():
    """The units to test. With a reduced UNITS (scoped gate) the file is a
    selection of units, not a copy of their definitions: flags and skip list
    are always re-read from config/units.txt, so a `unitedit --skip-del` done
    after writing the reduced file counts at once (a stale snapshot would
    silently keep treating those functions as skipped). A line whose source is
    not in config/units.txt is taken as is (a new unit not yet wired)."""
    current = {}
    if UNITS != 'config/units.txt' and os.path.exists('config/units.txt'):
        for raw in open('config/units.txt'):
            raw = raw.strip()
            if raw and not raw.startswith('#'):
                u = _parse_unit_line(raw)
                current[u[0]] = u
    units = []
    for raw in open(UNITS):
        raw = raw.strip()
        if not raw or raw.startswith('#'):
            continue
        u = _parse_unit_line(raw)
        units.append(current.get(u[0], u))
    return units


def normalize_for(rows, sym):
    """Normalized rows of a body, from the point of view of the symbol `sym`.
    Module-level because the global assignment (assign_statics), which runs
    after the units and outside do_unit, needs it too. It no longer depends on
    `sym`: the DNAS-protected functions are compared whole like all the others."""
    return normalize_asm.normalize_rows(rows)


def dnas_substitute(path):
    """DNAS build step: replace in place, in gcc's .s, the calls and markers of
    src/dnas/dnas_prot.h with the scaffolding (tools/dnas_subst.py); the
    original .s stays next to it as *.raw.s. Returns the anomalies (empty list
    = ok, or no markers)."""
    text = open(path).read()
    if not dnas_subst.has_markers(text):
        return []
    shutil.copyfile(path, path[:-2] + '.raw.s')
    lines, anom, _ = dnas_subst.substitute(text.splitlines())
    with open(path, 'w') as f:
        f.write('\n'.join(lines) + '\n')
    return anom


def do_unit(unit, syms, cache):
    """Compile and compare one unit. Returns (results, failed, log, plan): no
    direct printing, so the output stays in units.txt order even in parallel."""
    src, flags, asflags, skip = unit
    results, failed, log = {}, [], []

    is_cxx = src.endswith(('.cpp', '.cc', '.cxx', '.C'))
    cc = f'{TOOL}/g++' if is_cxx else f'{TOOL}/gcc'
    inc, flags = unit_includes_first(INC_CXX if is_cxx else INC_C, flags)
    base = re.sub(r'[^A-Za-z0-9_]', '_', src)
    out = f'build/check/{base}'
    os.makedirs(out, exist_ok=True)

    # `-Wimplicit-function-declaration` only in C and only to make stderr talk:
    # gcc 2.95 does not warn by default, so without this flag the check below
    # would be dead code. The flag does not affect code generation (identical
    # `.s` with and without it).
    wimp = '' if is_cxx else ' -Wimplicit-function-declaration'
    # original path for `__FILE__` (config/unit_paths.txt, tools/srcpath.py):
    # cwd and argument as in Sony's build, unchanged for the units not listed
    r = run(srcpath.command(src, f'{cc} -B{TOOL}/{wimp} -S -O2 {inc} {flags} -o {out}/out.s'))
    if r.returncode != 0:
        log.append(f'functions  error: {src}: compilation failed')
        err = r.stderr.rstrip().splitlines()
        if not VERBOSE:
            err = [l for l in err if 'error' in l.lower()][:5] or err[-5:]
        log.extend(f'    {l}' for l in err)
        return results, [src], log, []
    # Implicit declaration. In C, gcc 2.95 does not treat it as an error: a macro
    # left out of the file after moving code to another TU becomes a call, the .o
    # compiles, and the gate silently loses the function as "out of alignment".
    # The check costs nothing: stderr is already at hand.
    for m in re.finditer(r"implicit declaration of function [`'\"]?(\w+)", r.stderr):
        IMPLICIT.append((src, m.group(1)))
    # DNAS: the protection scaffolding is substituted into the .s between gcc and
    # gas in place of the calls and markers of src/dnas/dnas_prot.h
    # (tools/dnas_subst.py). gcc's .s stays next to it as out.raw.s. A failed
    # substitution is a build error of the unit.
    anom = dnas_substitute(f'{out}/out.s')
    if anom:
        log.append(f'functions  error: {src}: DNAS substitution failed (tools/dnas_subst.py)')
        log.extend(f'    {a}' for a in anom)
        return results, [src], log, []
    r = run(f'{AS} -mips2 -m5900 {asflags} -o {out}/out.o {out}/out.s')
    if r.returncode != 0:
        log.append(f'functions  error: {src}: assembly failed')
        return results, [src], log, []

    symtab = run(f'{OBJDUMP} -t {out}/out.o').stdout
    objsyms = defined_functions(f'{out}/out.o', symtab)
    # orphan linkonce copies (on by default; CHECK_ORPHAN_LINKONCE=0 turns them off, see
    # orphan_linkonce): they are not the function of their name, so they give no record
    # to that name; they go to the statics plan as candidates for the linkonce region only.
    orphans = orphan_linkonce(objsyms, syms)
    wanted = [s for s in objsyms
              if s in syms and s not in skip and syms[s][1] and s not in orphans]
    # The skips are measured (not judged): their result goes to
    # build/skip_distance.json, not check.json, and does not fail the gate.
    # It only costs the comparison (the unit is already compiled).
    skipped = [s for s in objsyms if s in syms and s in skip and syms[s][1]] \
        if MEASURE_SKIPS else []
    # The local functions of the .o (static, or non-exported globals) have no
    # name in the binary: their identity is the address (tools/statics.py). Two
    # ways, in order:
    #  1. Anchors: address = .dynsym anchor of the same .o + offset difference
    #     (from the previous one; if that fails, from the next one: between the
    #     two there is a function of the wrong size, and what follows is
    #     realigned from the right). Works if our file has the functions in the
    #     order of the original TU.
    #  2. Content: if (1) gives no known start or does not match, compare with
    #     every static of the universe of the same size; an exact match
    #     (identical rows, RELOCs included) is the identity. On a tie the one
    #     closest to the anchor address wins; a static already taken by another
    #     local of the same unit is not reused.
    # A local without an identity is "out of alignment" (misaligned): measured
    # (log), it does not fail the gate -- it is not an original function at
    # that address.
    located = []          # [(static name from the anchors | None, local name, anchor address)]
    for lname, d in statics.locate(objsyms, syms).items():
        cands = [d['addr']] + ([d['alt']] if d['alt'] not in (None, d['addr']) else [])
        known = [statics.name(c) for c in cands
                 if statics.name(c) in syms and syms[statics.name(c)][1]]
        exact = [sn for sn in known if syms[sn][1] == d['size']]
        located.append(((exact or known or [None])[0], lname, d['addr']))
    if not located:
        # Unit without anchors: no .dynsym function in the .o (a TU of statics
        # only, e.g. ref/tiff-3.4beta037/tif_codec.c = _notConfigured +
        # NotConfigured). statics.locate() returns nothing because it has no
        # base to count offsets from. Way 2 (content) needs no anchors: pass
        # pick=None and anchor 0.
        located = [(None, lname, 0) for lname, (off, sz, sec) in objsyms.items()
                   if sec == '.text' and sz and lname not in syms]
    # Data anchors: an identity proven by a table with a real name wins over the
    # offset anchor and over content (see data_anchors).
    da = data_anchors(f'{out}/out.o', objsyms)
    if da:
        byname = {ln: i for i, (_, ln, _) in enumerate(located)}
        for lname, addr in da.items():
            sn = statics.name(addr)
            if lname in syms or sn not in syms or not syms[sn][1]:
                continue
            if lname in byname:
                located[byname[lname]] = (sn, lname, addr)
            else:
                located.append((sn, lname, addr))
    if not wanted and not skipped and not located and not orphans:
        return results, failed, log, []

    # one objdump for the whole .o, sliced by (section, address)
    dis_text = run(f'{OBJDUMP} -z -dr {out}/out.o').stdout
    secs = rows_by_section(dis_text)
    # GOT symbol identity (tools/gotident.py): the row comparison masks the GOT
    # offsets, so which symbol a function calls through the GOT is measured
    # separately, from the relocations of the same dump. The verdict is given in
    # main(), where the original side is available too.
    got_loc, gotident.UNIT_DEFS[src] = gotident.symtab_info(symtab)
    got_by = gotident.ours_by_func(dis_text, objsyms, got_loc)
    # second and third dimension in one pass over the dump: data through the local GOT
    # (class and name) and LO16 targets (section+offset of each local GOT + LO16 pair, t9 included)
    gotloc_by, gotlo_by = gotident.local_refs_by_func(dis_text, objsyms, symtab)
    gotident.UNIT_FUNCS[src] = objsyms
    gotident.UNIT_TGT[src] = gotlo_by
    UNIT_ORPHANS[src] = set(orphans)

    def ours_rows_of(lname):
        oaddr, osize, osec = objsyms[lname]
        if osize and osec in secs:
            addrs, rows = secs[osec]
            i = bisect_left(addrs, oaddr)
            j = bisect_left(addrs, oaddr + osize)
            return rows[i:j]
        # size 0 in the .o (or section missing from the dump) -> per-symbol fallback
        return normalize_asm.parse_rows(
            run(f"{OBJDUMP} -z -r --disassemble='{lname}' {out}/out.o").stdout.splitlines())

    normalized = normalize_for

    def record(sym, lname, ok, total, bad, ours_n, is_skip, extra=None):
        results[sym] = {'unit': src, 'insns': total, 'bad': bad,
                        'size': syms[sym][1], 'ok': ok, 'ours': ours_n}
        if extra:
            results[sym].update(extra)
        if ok and not is_skip and lname in got_by:
            results[sym]['got_ours'] = got_by[lname]
            results[sym]['gotloc_ours'] = gotloc_by.get(lname, [])
            results[sym]['gotlo_ours'] = gotlo_by.get(lname, [])
        if is_skip:
            results[sym]['skip'] = True
            if KEEP_ROWS:
                results[sym]['rows'] = ours_n_rows[0]
            return
        if not ok:
            failed.append(sym)
        if VERBOSE or not ok:
            who = f'{sym} ({lname})' if lname != sym else sym
            log.append(f'{"ok  " if ok else "DIFF"} {src}: {who} {total - bad}/{total} rows equal')

    ours_n_rows = [None]
    for sym in wanted + skipped:
        rows = ours_rows_of(sym)
        ours = normalized(rows, sym)
        ours_n_rows[0] = ours
        ok, total, bad = compare(cache.get(sym, []), ours)
        record(sym, sym, ok, total, bad, len(ours), sym not in wanted)

    def content_hits(rows, osize, pick):
        """The statics of the universe whose body is identical to these rows.
        The symbol size in our .o is not always the size of the body: `.end`
        rounds it, and the window [addr, addr+size) on the dump can drag along
        up to 3 instructions of the next function's prologue (e.g.
        internal_verify, x509_vfy.c: st_size 1336 against a 1324-byte body).
        So candidates are also searched at +-4/8/12 bytes, tolerating up to 3
        extra rows at the tail (never in the middle: the comparison stays exact
        on the body)."""
        cands = list(BY_SIZE.get(osize, ()))
        for d in (4, 8, 12):
            for sz in (osize - d, osize + d):
                cands += [c for c in BY_SIZE.get(sz, ()) if c not in cands]
        nours = len(normalize_asm.normalize_rows(rows)) if NROWS else 0
        # The symbol size in our .o is not comparable with the original's when
        # normalize_asm normalizes a call form. A `jal` to a global function
        # defined in the same TU comes out of our assembler as GOT_PAGE+LO16
        # (2 instructions) while the original has %call16 (1); normalize_asm
        # masks the difference and the comparison is exact, but the .o is 4 bytes
        # longer per call site (e.g. static_0082A538, http_del_std_headers: 480 B
        # in the .o against 444, 120 rows against 111 normalized), beyond the
        # +-12 window of BY_SIZE. So candidates are also indexed by the number
        # of normalized rows of the original, with an exact match: the
        # comparison stays row by row, so the wider window cannot produce false
        # positives, only extra candidates to discard.
        for c in NROWS.get(nours, ()):
            if c not in cands:
                cands.append(c)
        hits = []
        for cand in cands:
            if cand == pick:
                continue
            o2 = normalized(rows, cand)
            corig = cache.get(cand, [])
            if corig and 0 < len(o2) - len(corig) <= 3:
                o2 = o2[:len(corig)]
            if compare(corig, o2)[0]:
                hits.append(cand)
        return hits

    # Plan, then assignment. The candidates are computed before assigning
    # because the assignment is greedy and the order in which the locals are
    # met must not decide who wins: an anchored local that matches, or one with
    # a single candidate, must not be robbed by a generic stub (`return 0`)
    # with twenty candidates.
    plan = []
    for pick, lname, anchor_addr in located:
        rows = ours_rows_of(lname)
        ooff, osize = objsyms[lname][0], objsyms[lname][1]
        ours, ok, total, bad = None, False, 0, 0
        if pick:
            ours = normalized(rows, pick)
            ok, total, bad = compare(cache.get(pick, []), ours)
        plan.append({'src': src, 'skip': skip,
                     'pick': pick, 'lname': lname, 'anchor': anchor_addr,
                     'rows': rows, 'size': osize, 'off': ooff, 'ok': ok, 'total': total,
                     'bad': bad, 'ours': ours, 'got': got_by.get(lname),
                     'gotloc': gotloc_by.get(lname), 'gotlo': gotlo_by.get(lname),
                     'hits': [] if ok else content_hits(rows, osize, pick)})
    # Orphan linkonce copies: separate plan (key 'orphan'), served by assign_statics
    # after all the locals and only on the still free statics of the linkonce region.
    # The zone is the set of original addresses of the named functions this .o has in
    # the linkonce region (`__tf6Object`, `_M_insert_aux`...): the orphan section that
    # is kept sits in the linkonce block of its TU.
    if orphans:
        lo = linkonce_lo(syms)
        lk_zone = sorted(syms[n][0] for n, (off, sz, sec) in objsyms.items()
                         if sz and sec.startswith('.gnu.linkonce.t.') and n in syms
                         and syms[n][1] and syms[n][0] >= lo)
        # in the order of the .o sections (= emission order, that of the dump)
        secorder = {sec: i for i, sec in enumerate(secs)}
        for lname in sorted(orphans, key=lambda n: (secorder.get(objsyms[n][2], 1 << 30), n)):
            rows = ours_rows_of(lname)
            ooff, osize = objsyms[lname][0], objsyms[lname][1]
            hits = [c for c in content_hits(rows, osize, None) if syms[c][0] >= lo]
            plan.append({'src': src, 'skip': skip, 'orphan': orphans[lname],
                         'lk_zone': lk_zone, 'pick': None, 'lname': lname, 'anchor': 0,
                         'rows': rows, 'size': osize, 'off': ooff, 'ok': False, 'total': 0,
                         'bad': 0, 'ours': None, 'got': got_by.get(lname),
                         'gotloc': gotloc_by.get(lname), 'gotlo': gotlo_by.get(lname),
                         'hits': hits})
    # The assignment is not done here: it is done in assign_statics(), once for
    # all the units. See its docstring for why.
    return results, failed, log, plan


def _assign_order(plans, syms, claimed, zones):
    """Order in which assign_statics serves the plans: first the certain anchors,
    then unambiguous content, in units.txt order (sorted is stable). The ambiguous
    ones (several candidates) are instead served by distance, globally: at each
    step the plan whose best free candidate is the closest to its anchor or zone
    goes first.

    Why: in units.txt order, small identical stubs (e.g. the six `aval*` natives
    of `src/natives/natives_dat.cpp`, 20 B) would be credited to look-alike stubs
    of units megabytes away, only because those units come earlier in the file.
    It is a generator: lazy evaluation is needed because `claimed`/`zones` change at
    each assignment made by the caller.
    """
    ordered = sorted(plans, key=lambda e: 0 if e['ok'] else (1 if len(e['hits']) == 1 else 2))
    rest = []
    for e in ordered:
        if e['ok'] or len(e['hits']) == 1:
            yield e
        else:
            rest.append(e)

    def best(e):
        z, ref = zones.get(e['src']), e['anchor']
        free = [c for c in e['hits'] if c not in claimed]
        if not free:
            return float('inf')
        def d(c):
            a = syms[c][0]
            ds = [abs(a - ref)] if ref else []
            if z:
                ds.append(min(abs(a - q) for q in z))
            return min(ds) if ds else a
        return min(d(c) for c in free)

    while rest:
        i = min(range(len(rest)), key=lambda k: best(rest[k]))
        yield rest.pop(i)


def assign_statics(plans, syms, cache):
    """Assign the local bodies (statics without a name in the binary) to the
    original addresses. Global, done once, after every unit has produced its plan.

    Why global: with a per-unit `claimed` set (and units running on several
    threads) two units with a body that normalizes the same and the same pair of
    candidates could claim the same address, leaving another one uncovered: a
    real match would vanish from the gate (e.g. `dnasextension.cpp` and
    `natives_cdl.cpp` both claiming 0x004EA858, the two originals differing only
    in the `gp` setup, indistinguishable after normalize_asm).

    Assignment order: first the certain identities (anchors that match), then
    unambiguous content, the ambiguous ones last. `sorted` is stable, so within a
    class the units.txt order wins: the result does not depend on thread scheduling.

    The tie-break zone stays per unit (the addresses that unit has already taken):
    making it global would pull a body towards the addresses of an unrelated unit.

    Returns ({unit: {sym: record}}, [red syms], [log lines]).
    """
    out, failed, log = {}, [], []
    claimed, zones = set(), {}
    # the orphan linkonce copies are served after everything else, in
    # _assign_orphans: the locals see exactly the same plans as without them
    orphan_plans = [e for e in plans if e.get('orphan')]
    plans = [e for e in plans if not e.get('orphan')]
    # Twin groups: locals of the same unit with the same set of candidates (>1).
    # They are placed in .o order on the closest candidates (see below).
    groups = {}
    for e in plans:
        if not e['ok'] and len(e['hits']) > 1 and e.get('off') is not None:
            groups.setdefault((e['src'], frozenset(e['hits'])), []).append(e)
    done_ids = set()   # id() of the plans already served
    leftovers = []
    for e in _assign_order(plans, syms, claimed, zones):
        src, skip = e['src'], e['skip']
        pick, lname, anchor_addr = e['pick'], e['lname'], e['anchor']
        sym, ours, ok, total, bad = pick, e['ours'], e['ok'], e['total'], e['bad']
        ident = 'anchors'
        if ok and sym in claimed:
            # two units claim the same address by anchors: the first in
            # units.txt order keeps it, this one falls back to content.
            ok = False
        if not ok:
            hits = [c for c in e['hits'] if c not in claimed]
            if hits:
                def dist(c, _ref=anchor_addr, _z=zones.get(src)):
                    a = syms[c][0]
                    ds = [abs(a - _ref)] if _ref else []
                    if _z:
                        ds.append(min(abs(a - z) for z in _z))
                    return min(ds) if ds else a

                hits.sort(key=dist)
                # Indistinguishable twins of the same unit (same set of candidates)
                # still to place: take the len(pend) closest candidates, sort them by
                # address and give this local the one of its position in the .o among
                # the pending twins. The linker does not reorder a TU (e.g. in
                # `amxmath.cpp` `n_sin` and `n_cos` differ only in the GOT callee,
                # which is masked; without this the gate could swap them and turn the
                # `smallext_math` table red). Only within the group: an ordering rule
                # over all the ambiguous ones would move identities across units.
                gr = groups.get((src, frozenset(e['hits'])), ())
                pend = [g for g in gr if id(g) not in done_ids]
                if len(pend) > 1 and e in pend:
                    top = sorted(hits[:len(pend)], key=lambda c: syms[c][0])
                    rank = sum(1 for g in pend if g['off'] < e['off'])
                    hits.remove(top[min(rank, len(top) - 1)])
                    hits.insert(0, top[min(rank, len(top) - 1)])
                sym = hits[0]
                ours = normalize_for(e['rows'], sym)
                # the .o window may have up to 3 trailing rows belonging to the next
                # function: the body that counts is the original's
                corig = cache.get(sym, [])
                if corig and 0 < len(ours) - len(corig) <= 3:
                    ours = ours[:len(corig)]
                ok, total, bad = True, len(ours), 0
                ident = 'content' + (f' ({len(hits)} candidates)' if len(hits) > 1 else '')
                if pick and pick != sym:
                    ident += f', the anchors said {pick}'
            elif sym is not None and sym in claimed:
                # contested anchors and no free candidate: this body does not
                # correspond to any still uncovered address.
                sym = None
        done_ids.add(id(e))
        if sym is None:
            leftovers.append(e)
            continue
        claimed.add(sym)
        zones.setdefault(src, []).append(syms[sym][0])
        is_skip = sym in skip
        STATIC_LOG.setdefault(src, []).append(
            {'local': lname, 'addr': syms[sym][0], 'size': e['size'], 'orig_size': syms[sym][1],
             'state': 'ok' if ok else ('skip' if is_skip else 'fail'),
             'insns': total, 'bad': bad, 'ident': ident})
        rec = {'unit': src, 'insns': total, 'bad': bad, 'size': syms[sym][1],
               'ok': ok, 'ours': len(ours), 'local': lname, 'ident': ident}
        if is_skip:
            rec['skip'] = True
            if KEEP_ROWS:
                rec['rows'] = ours
        else:
            if ok and e.get('got') is not None:
                rec['got_ours'] = e['got']   # GOT identity: judged by main() (gotident)
                rec['gotloc_ours'] = e.get('gotloc') or []
                rec['gotlo_ours'] = e.get('gotlo') or []
            if not ok:
                failed.append(sym)
            if VERBOSE or not ok:
                who = f'{sym} ({lname})' if lname != sym else sym
                log.append(f'{"ok  " if ok else "DIFF"} {src}: {who} {total - bad}/{total} rows equal')
        out.setdefault(src, {})[sym] = rec
    # the orphans before the pairing of the skips: that is only measurement (never ok),
    # and served first it could give another unit, as "skip", a static of the region
    # that an orphan copy covers exactly
    if orphan_plans:
        _assign_orphans(orphan_plans, syms, cache, claimed, out, log)
    _pair_skip_leftovers(leftovers, syms, cache, claimed, out)
    return out, failed, log


def _assign_orphans(orphan_plans, syms, cache, claimed, out, log):
    """Second pass of assign_statics: the orphan linkonce copies (orphan_linkonce).

    Why a separate, final pass: (1) the locals, anchors and skip pairing see exactly
    the same state as without it, so the rule cannot move any of their assignments;
    (2) the candidates are already restricted (do_unit) to the statics of the linkonce
    region with an identical body, so an orphan copy never takes a static outside the
    region nor a named symbol (the candidates are only `static_<ADDR>`), and never an
    address already taken by anyone.

    Order: first the plans with one free candidate, then the others, ties in units.txt
    order (sorted is stable). Among several candidates the one closest to the zone of
    its unit wins (the named functions its .o has in the linkonce region: the orphan
    section sits in the block of its TU), then the lowest address. Several units may
    emit the same comdat copy: the first in units.txt order keeps it, approximating
    the link order (the linker keeps the first section with that name).

    A copy without a candidate, if another copy of the same name took a static, is the
    copy discarded by the linker (the normal case for a comdat): not recorded. If
    instead no copy of that name finds a static, either the original lacks that
    section (our link would keep it: extra code in the region) or our copy differs (the
    static stays without an outcome and the full gate only says `lost`): it is logged
    by name, because that generic line does not tell where it comes from. The
    `'dynsym'` ones (a fact of the original) are served before the `'strong?'` ones
    (which depend on the run)."""
    def free(e):
        return [c for c in e['hits'] if c not in claimed]

    ordered = sorted(orphan_plans, key=lambda e: (e['orphan'] != 'dynsym',
                                                  0 if len(free(e)) == 1 else 1))
    served = set()
    for e in ordered:
        hits = free(e)
        if not hits:
            continue
        z = e.get('lk_zone') or []
        hits.sort(key=lambda c: (min((abs(syms[c][0] - q) for q in z), default=0),
                                 syms[c][0]))
        sym, src, lname = hits[0], e['src'], e['lname']
        served.add(lname)
        ours = normalize_for(e['rows'], sym)
        corig = cache.get(sym, [])
        if corig and 0 < len(ours) - len(corig) <= 3:
            ours = ours[:len(corig)]
        total = len(ours)
        ident = (f'orphan linkonce copy ({e["orphan"]}): content'
                 + (f' ({len(hits)} candidates)' if len(hits) > 1 else ''))
        claimed.add(sym)
        is_skip = sym in e['skip']
        STATIC_LOG.setdefault(src, []).append(
            {'local': lname, 'addr': syms[sym][0], 'size': e['size'],
             'orig_size': syms[sym][1], 'state': 'skip' if is_skip else 'ok',
             'insns': total, 'bad': 0, 'ident': ident})
        rec = {'unit': src, 'insns': total, 'bad': 0, 'size': syms[sym][1],
               'ok': True, 'ours': len(ours), 'local': lname, 'ident': ident}
        if is_skip:
            rec['skip'] = True
            if KEEP_ROWS:
                rec['rows'] = ours
        else:
            if e.get('got') is not None:
                rec['got_ours'] = e['got']
                rec['gotloc_ours'] = e.get('gotloc') or []
                rec['gotlo_ours'] = e.get('gotlo') or []
            if VERBOSE:
                log.append(f'ok   {src}: {sym} ({lname}, orphan linkonce copy) {total}/{total}')
        out.setdefault(src, {})[sym] = rec
    for lname in sorted({e['lname'] for e in orphan_plans} - served):
        units_ = sorted({e['src'] for e in orphan_plans if e['lname'] == lname})
        log.append(f'warning: orphan linkonce copy {lname} ({", ".join(units_)}): no free identical '
                   f'static in the linkonce region (extra copy, or a differing copy: then a static of '
                   f'the region is left without a result)')


def _mn_overlap(a, b):
    """Rough similarity between two normalized bodies: overlap of the mnemonic
    multisets, over the length of the longer one. O(n), only used to choose the
    pair."""
    ca = Counter(r.split(None, 1)[0] for r in a if r)
    cb = Counter(r.split(None, 1)[0] for r in b if r)
    return sum((ca & cb).values()) / max(len(a), len(b), 1)


def _pair_skip_leftovers(leftovers, syms, cache, claimed, out):
    """Pair the local bodies left "out" (misaligned) with the `static_<ADDR>` skips that
    the same unit declares in its skip column and nobody took. Measurement only:
    the record comes out as skip (never ok, never in `failed`), so the gate's
    verdict does not change; only the skip measurement sees it.

    Why: a static body that does not match finds its address only through
    anchors; if the anchors do not fit (file order different from the TU, wrong
    size upstream) it stayed "out" and the skip at its address had no
    measurement at all.

    Pairing: greedy on the score (mnemonic overlap, then size proximity), one
    address per body. The identity is probable, not proven, and `ident` says so."""
    byu = {}
    for e in leftovers:
        byu.setdefault(e['src'], []).append(e)
    for src, es in byu.items():
        skip = es[0]['skip']
        # sorted(): `skip` is a set of strings, and with hash randomization its
        # order changes between processes; on equal scores (twin bodies, e.g.
        # dnas1_check_tag2/tag32 at 1.00) the pair would change between two runs
        # and the measurement would oscillate.
        cands = sorted(s for s in skip if statics.is_static(s) and s in syms
                       and syms[s][1] and s not in claimed)
        scored = []
        for i, e in enumerate(es):
            for c in cands:
                ours = normalize_for(e['rows'], c)
                corig = cache.get(c, [])
                if not corig or not ours:
                    continue
                sc = _mn_overlap(corig, ours)
                szd = abs(len(corig) - len(ours)) / max(len(corig), 1)
                scored.append((sc, szd, e['anchor'], syms[c][0], i, c, ours))
        # On equal scores (twin bodies) the order is respected: the local that
        # comes first in the file (lower anchor) takes the lower address. Not
        # the distance from the anchor: in the DNAS units the anchors are all
        # shifted by the scaffolding, and the minimum distance crossed the pairs.
        scored.sort(key=lambda t: (-t[0], t[1], t[2], t[3], t[5]))
        used = set()
        for sc, _szd, _anc, _a, i, c, ours in scored:
            if i in used or c in claimed or sc < 0.5:
                continue
            used.add(i)
            claimed.add(c)
            e = es[i]
            _ok, total, bad = compare(cache.get(c, []), ours)
            ident = f'measure: paired by similarity ({sc:.2f}), probable identity'
            STATIC_LOG.setdefault(src, []).append(
                {'local': e['lname'], 'addr': syms[c][0], 'size': e['size'],
                 'orig_size': syms[c][1], 'state': 'skip', 'insns': total,
                 'bad': bad, 'ident': ident})
            rec = {'unit': src, 'insns': total, 'bad': bad, 'size': syms[c][1],
                   'ok': False, 'ours': len(ours), 'local': e['lname'],
                   'ident': ident, 'skip': True}
            if KEEP_ROWS:
                rec['rows'] = ours
            out.setdefault(src, {})[c] = rec
        for i, e in enumerate(es):
            if i not in used:
                STATIC_LOG.setdefault(src, []).append(
                    {'local': e['lname'], 'addr': e['anchor'], 'size': e['size'],
                     'state': 'misaligned'})



def run_units(units, syms, cache, jobs_n=None, echo=True):
    """Compile and compare a set of units, static assignment included. This is
    the path external callers must use too: `do_unit` alone does not assign
    addresses to the local bodies -- it returns a plan, and the assignment is
    global (see assign_statics). A caller of `do_unit` alone would see no statics.

    Returns (per_unit, failed) with per_unit = [(src, {sym: record}), ...]
    in the order of `units`.
    """
    per_unit, plans, failed = [], [], []
    # Every caller needs the g++-3 link, not only main(): without it every C++ unit
    # fails ("string: No such file", counted as a failed unit). Idempotent; here and
    # not in do_unit, which runs in parallel (concurrent unlink/symlink).
    cross_include()
    # UNIT_DEFS is module-level: a unit that does not reach the dump in this run (failed
    # compilation) must not leave the definitions of a previous run ('strong?' filter)
    for u in units:
        gotident.UNIT_DEFS.pop(u[0], None)
    with ThreadPoolExecutor(max_workers=jobs_n or jobs()) as pool:
        # map() preserves the units.txt order: output and "last line wins"
        # stay identical to a sequential run
        for unit, (res, fail, log, plan) in zip(units, pool.map(
                lambda u: do_unit(u, syms, cache), units)):
            per_unit.append((unit[0], res))
            failed.extend(fail)
            plans.extend(plan)
            if echo:
                for line in log:
                    _echo(line)
    # linkonce copies whose name is not in .dynsym ('strong?', orphan_linkonce): orphan only
    # if another unit of the run has the strong definition; the others are ordinary comdat
    # copies (discarded or never linked) and stay out of the plan
    plans = [e for e in plans if e.get('orphan') != 'strong?'
             or any(e['lname'] in gotident.UNIT_DEFS.get(u[0], ())
                    for u in units if u[0] != e['src'])]
    static_out, static_failed, static_log = assign_statics(plans, syms, cache)
    failed.extend(static_failed)
    if echo:
        for line in static_log:
            _echo(line)
        if _ECHO_CAP['dropped']:
            print(f"... and {_ECHO_CAP['dropped']} more (VERBOSE=1 for all, or see the JSON report)")
    for src, res in per_unit:
        res.update(static_out.get(src, {}))
    lo16_context(units, syms, plans)
    return per_unit, failed


# failure lines of the units: at most 20 on screen unless VERBOSE (the JSON report has them all)
_ECHO_CAP = {'shown': 0, 'dropped': 0, 'last': False}


def _echo(line):
    if not STATS:
        if line.startswith('    '):         # detail of the previous line: same fate
            if _ECHO_CAP['last']:
                return
        elif line.startswith(('DIFF', 'functions  error')):
            _ECHO_CAP['last'] = _ECHO_CAP['shown'] >= 20
            if _ECHO_CAP['last']:
                _ECHO_CAP['dropped'] += 1
                return
            _ECHO_CAP['shown'] += 1
    print(line)


def lo16_context(units, syms, plans):
    """Identity of the functions of our .o for the third dimension (gotident.lo16_judge):
    address in the original = the universe name for named functions (not the orphan
    linkonce copies, which are not the function of their name), the gate's placement for
    the locals (STATIC_LOG, states ok/fail/skip; not "misaligned": that is an anchor, not an
    identity); and the content candidates of each local (identical body: referring to
    any of them is the same)."""
    for u in units:
        src = u[0]
        F = gotident.UNIT_FUNCS.get(src, {})
        orph = UNIT_ORPHANS.get(src, set())
        ad = {n: syms[n][0] for n in F if n in syms and syms[n][1] and n not in orph}
        for r in STATIC_LOG.get(src, ()):
            if r['state'] != 'misaligned':
                ad[r['local']] = r['addr']
        gotident.UNIT_ADDR[src] = ad
        gotident.UNIT_HITS[src] = {}
    for e in plans:
        if e.get('hits'):
            gotident.UNIT_HITS.setdefault(e['src'], {})[e['lname']] = {syms[c][0] for c in e['hits']}


def main():
    cross_include()
    sha1 = verify_sha1()
    validate_units()
    syms = load_symbols()
    cache = orig_cache(sha1, syms)
    units = read_units()
    nj = jobs()
    info(f'{len(units)} units on {nj} threads')

    results, skipres = {}, {}
    # The local bodies are assigned to addresses once for all the units
    # (assign_statics, inside run_units).
    per_unit, failed = run_units(units, syms, cache, jobs_n=nj)
    # GOT identity: the green copies are collected before merging by symbol,
    # because a deduplicated STL symbol has one copy per unit and all are judged
    # (with only the last copy, the scoped and the full gate would disagree).
    got_copies = []
    for _, res in per_unit:
        for s, r in res.items():
            if 'got_ours' in r:
                g = r.pop('got_ours')
                gl = r.pop('gotloc_ours', None)
                gt = r.pop('gotlo_ours', None)
                if r.get('ok') and not r.get('skip'):
                    got_copies.append((s, r['unit'], g, r.get('ident'), gl, gt,
                                       r.get('local') or s))
    for _, res in per_unit:
        for s, r in res.items():
            if r.get('skip'):
                # a symbol may be skipped in several units (same-TU stub +
                # real body): keep the measurement of the larger body and
                # remember the units. Never overwrite a match.
                prev = skipres.get(s)
                if prev is None or r['ours'] > prev['ours']:
                    r['units'] = sorted(set((prev or {}).get('units', [])) | {r['unit']})
                    skipres[s] = r
                else:
                    prev['units'] = sorted(set(prev['units']) | {r['unit']})
            else:
                results[s] = r

    os.makedirs('build', exist_ok=True)
    # With a reduced UNITS (gate on a few units) do not write build/check.json:
    # it is the "how much is matched" state read by progress and the link, and
    # overwriting it with a subset would silently corrupt that state.
    # The scoped file name carries the basename of the units file, so that
    # concurrent scoped runs on the same checkout do not read each other's results.
    out_json = ('build/check.json' if UNITS == 'config/units.txt'
                else 'build/check-%s.json' % re.sub(
                    r'[^A-Za-z0-9_.-]', '_', os.path.basename(UNITS).rsplit('.', 1)[0]))
    # GOT symbol identity: verdict on the green copies (tools/gotident.py). The
    # accepted debt is in config/got_debt.txt; only what is outside it fails.
    full_got = (UNITS == 'config/units.txt')
    got = gotident.judge(got_copies, {s: v for s, v in syms.items() if v[1]}, PROTECTED,
                         gotident.load_debt(), results=results, scoped=not full_got,
                         ldebt=gotident.load_debt(gotident.LDEBT),
                         l16debt=gotident.load_debt(gotident.TDEBT))
    gloc = got['loc']
    glo = got['lo16']
    got_bad = got['new'] + got['worse'] + gloc['new'] + gloc['worse'] \
        + glo['new'] + glo['worse']
    if full_got:
        json.dump({'got': got['per_copy'], 'loc': gloc['per_copy'], 'lo16': glo['per_copy']},
                  open(gotident.COPIES, 'w'), indent=0)
    # a skip already green elsewhere (STL dedup) is not a candidate: drop it
    skipres = {s: r for s, r in skipres.items()
               if not (s in results and results[s]['ok'])}
    for r in skipres.values():
        r.pop('rows', None)
    json.dump(results, open(out_json, 'w'), indent=1)
    # the locals of each unit with the address derived from the anchors and the
    # state (ok/fail/skip/misaligned): read by tools/match.sh
    static_json = out_json.replace('check', 'check_statics')
    json.dump(STATIC_LOG, open(static_json, 'w'), indent=1)
    if skipres and out_json == 'build/check.json':
        json.dump(skipres, open('build/skip_distance.json', 'w'), indent=1)
        info(f'{len(skipres)} skipped symbols measured -> build/skip_distance.json')

    ok_n = sum(1 for r in results.values() if r['ok'])
    st = [s for s in results if statics.is_static(s)]
    st_ok = sum(1 for s in st if results[s]['ok'])
    misaligned = sum(1 for rows in STATIC_LOG.values() for r in rows if r['state'] == 'misaligned')
    head = f'functions  {ok_n}/{len(results)} match'
    if skipres:
        head += f', {len(skipres)} skipped'
    if got['tot']:
        head += (f', GOT/data/LO16 identity ok on {got["tot"]} copies' if not got_bad
                 else f', {len(got_bad)} identity problems')
    if failed:
        head += f', {len(failed)} failed'
    if out_json != 'build/check.json':
        head += f' (scoped: {out_json}, build/check.json untouched)'
    print(head)

    def debt_warning(label, R, path):
        if R['clean'] or R['orphans'] or R['improved'] or R['moved']:
            print(f'warning: {label} debt ({path}) can be updated: {len(R["clean"])} copies now '
                  f'clean, {len(R["orphans"])} orphan rows, {len(R["improved"])} improved, '
                  f'{len(R["moved"])} moved: run `make got-debt` after a full gate')

    # 1. GOT global symbols
    info(f'GOT symbol identity: {got["ok"]}/{got["tot"]} green copies use the same global symbols '
         f'as the original; {got["debt"]} in debt ({gotident.DEBT}), {got["ambiguous"]} attributed '
         f'by content, {len(got["new"])} new outside the debt, {len(got["worse"])} worse')
    for k, diag in sorted(got['new'], key=lambda x: (x[0].split('@', 1)[-1], x[0]))[:20]:
        print(f'GOT symbol differs (not in debt; put the real name, [TU] = move the body): {k}: {diag}')
    if len(got['new']) > 20:
        print(f'... and {len(got["new"]) - 20} more')
    for k, diag, d0, d1 in got['worse'][:20]:
        print(f'GOT debt row got worse: {k}: dist {d0} -> {d1}: {diag}')
    if len(got['worse']) > 20:
        print(f'... and {len(got["worse"]) - 20} more')
    for k, diag in got['warnings'][:20]:
        print(f'warning: GOT identity differs on a static attributed by content (scoped run, the full '
              f'gate judges it): {k}: {diag}')
    debt_warning('GOT symbol', got, gotident.DEBT)

    # 2. data through the local GOT
    info(f'local GOT data identity: {gloc["ok"]}/{gloc["tot"]} green copies reach the same objects; '
         f'{gloc["debt"]} in debt ({gotident.LDEBT}), {gloc["ambiguous"]} by content, '
         f'{len(gloc["new"])} new, {len(gloc["worse"])} worse')
    for tag, lst in (('differs (not in debt)', gloc['new']), ('debt row got worse', gloc['worse'])):
        for x in lst[:20]:
            extra = f' (dist {x[2]} -> {x[3]})' if len(x) > 3 else ''
            print(f'local GOT data {tag}: {x[0]}{extra}: {x[1]}')
        if len(lst) > 20:
            print(f'... and {len(lst) - 20} more')
    if gloc['warnings']:
        print(f'warning: {len(gloc["warnings"])} statics attributed by content reach different local '
              f'data (scoped run, the full gate judges them)')
    debt_warning('local GOT data', gloc, gotident.LDEBT)

    # 3. LO16 targets
    info(f'LO16 target identity (local GOT + LO16, t9 calls included): {glo["ok"]}/{glo["tot"]} '
         f'green copies reach the same addresses ({glo["refs"]} references, {glo["unverifiable"]} not '
         f'verifiable'
         + (': ' + ', '.join(f'{n} {k}' for k, n in sorted(glo['unverifiable_why'].items(), key=lambda kv: -kv[1]))
            if glo['unverifiable_why'] else '') + f'); {glo["debt"]} in debt ({gotident.TDEBT}), '
         f'{glo["ambiguous"]} by content, {len(glo["new"])} new, {len(glo["worse"])} worse')
    for tag, lst in (('differs (not in debt)', glo['new']), ('debt row got worse', glo['worse'])):
        for x in lst[:20]:
            extra = f' (dist {x[2]} -> {x[3]})' if len(x) > 3 else ''
            print(f'LO16 target {tag}: {x[0]}{extra}: {x[1]}')
        if len(lst) > 20:
            print(f'... and {len(lst) - 20} more')
    for k, diag in glo['warnings'][:20]:
        print(f'warning: LO16 targets differ on a static attributed by content (scoped run, the '
              f'full gate judges it): {k}: {diag}')
    debt_warning('LO16 target', glo, gotident.TDEBT)
    if got_bad:
        print('details with both sides: python3 tools/gotident.py SYM')

    if IMPLICIT:
        # With ~90 pre-existing cases (headers not included by the vendored
        # libraries) the total is not a signal: what matters is what appears
        # (e.g. a matched body deleted while its call stays). So the run is
        # compared with the previous one (build/implicit.json) and the new ones
        # are always printed.
        implicit_by_unit = {}
        for src, fn in IMPLICIT:
            implicit_by_unit.setdefault(src, set()).add(fn)
        current_calls = {u: sorted(v) for u, v in implicit_by_unit.items()}
        previous = {}
        # The baseline is valid only for a full run. A scoped gate (`UNITS=...`)
        # sees a handful of units and would overwrite the baseline with its
        # subset: the next full run would report as "new" calls that were
        # always there.
        full_run = (UNITS == 'config/units.txt')
        pj = 'build/implicit.json'
        have_baseline = full_run and os.path.exists(pj)
        if have_baseline:
            try:
                previous = json.load(open(pj))
            except Exception:
                previous = {}
        new_calls = {u: [f for f in fs if f not in set(previous.get(u, []))]
                     for u, fs in current_calls.items()}
        new_calls = {u: fs for u, fs in new_calls.items() if fs}
        with_misaligned = {u for u, rows in STATIC_LOG.items()
                           if any(r['state'] == 'misaligned' for r in rows)} & set(implicit_by_unit)
        info(f'{len(IMPLICIT)} calls to undeclared functions in {len(implicit_by_unit)} units (not an '
             f'error for gcc 2.95 in C); {len(with_misaligned)} of those units also have an out-of-alignment body')
        if not have_baseline:
            new_calls = {}      # scoped run (the baseline is left alone) or first full run (it is written)
        for u in sorted(new_calls):
            print(f'warning: {u}: new call to undeclared {", ".join(new_calls[u])} (a removed or renamed '
                  f'function whose call stayed?)')
        for src in sorted(with_misaligned)[:8]:
            if src not in new_calls:
                print(f'warning: {src}: undeclared calls {", ".join(sorted(implicit_by_unit[src]))} and an '
                      f'out-of-alignment body')
        if len(with_misaligned) > 8:
            print(f'... and {len(with_misaligned) - 8} more units with both symptoms')
        if full_run:
            try:
                json.dump(current_calls, open(pj, 'w'), indent=1, sort_keys=True)
            except Exception:
                pass
    if st or STATIC_LOG:
        info(f'unnamed (static): {st_ok}/{len(st)} match, '
             f'{len(skipres and [s for s in skipres if statics.is_static(s)])} skipped and measured, '
             f'{misaligned} out of alignment ({static_json})')
        if misaligned:
            print(f'warning: {misaligned} local bodies out of alignment (match no function of the original: '
                  f'layout debt, see {static_json})')
    # How much of the count rests on an ambiguous attribution. The gate accepts
    # a local whose body is identical to the original's at that address; when
    # the same body is also identical to N other candidates (typical of small
    # bodies: 68 B with 65 candidates), the bytes are verified but which of the
    # N functions we wrote is not established, and the credit moves as soon as
    # the real TU writes its body. Not a false match: a match whose identity is
    # not proven, and it is reported here.
    amb = [(s, r) for s, r in results.items()
           if r['ok'] and 'candidates' in (r.get('ident') or '')]
    many_candidates = [(s, r) for s, r in amb
                       if int(re.search(r'(\d+) candidates', r['ident']).group(1)) >= 20]
    if amb:
        nb = lambda g: sum(r.get('size') or 0 for _, r in g)
        info(f'ambiguous attribution by content: {len(amb)} functions ({nb(amb)} B) with more than '
             f'one candidate, {len(many_candidates)} ({nb(many_candidates)} B) with >= 20: bytes verified, address '
             f'identity not proven')
        if many_candidates:
            print(f'warning: {len(many_candidates)} functions ({nb(many_candidates)} B) matched by content with >= 20 '
                  f'candidates: bytes verified, address identity not proven')
    # In a full run every function of the universe must have an outcome (green,
    # red or skipped): one that disappears is not "0 problems" (e.g. functions
    # that lose their C linkage still match as statics by content, while their
    # named symbols silently vanish).
    lost = []
    if out_json == 'build/check.json':
        lost = sorted(set(universe.achievable()) - set(results) - universe.skips())
        if lost:
            print(f'functions  error: {len(lost)} functions of the universe have no result (not green, '
                  f'not red, not skipped): ' + ', '.join(lost[:10]) + (' ...' if len(lost) > 10 else ''))
    sys.exit(1 if failed or got_bad or lost else 0)

if __name__ == '__main__':
    main()
