#!/usr/bin/env python3
"""Prepare build/relink/libs/: the shared libraries of the link.

Usage: python3 tools/relink/make_libs.py [--kit-libc]   (repository root, after fetch.py)

Inputs: docker/kit/rootfs/lib (glibc 2.2.2 of the kit, `make kit`) and build/relink/dl/dp41/
(Definitive Patch v4.1.0, tools/relink/fetch.py). The libraries are third-party binaries: they
live only in build/ (gitignored), never in the repository (docs/original-bytes.md). Every copied
file is checked against a known sha1.

  real:   libc.so.6 of PSBBN (Definitive Patch __linux.1/lib, sha1 744284e5...): unlike the
          kit's (a2d60e50...) it has __register_frame_info 184 B and __deregister_frame_info
          296 B like the original's .dynsym (kit: 68/156); only malloc/free/realloc/calloc
          still differ (they come from ld.so.1, docs/linking.md). --kit-libc uses the
          kit's one (diagnostics, sha1 not checked).
          glibc 2.2.2 of the KIT (docker/kit/rootfs/lib): libm.so.6,
          libpthread.so.0 (0.9, the kit's: 64/64 sizes equal to the original),
          libdl.so.2, libcrypt.so.1;
          libstdc++-libc6.2-2.so.3 of PSBBN (Definitive Patch v4.1.0, __linux.1/usr/lib:
          36/36 sizes equal; the kit's 29/36);
          libocm.so (OpenMG, Definitive Patch __linux.4/bn/openmg/libocm.so.1.3: it has no
          DT_SONAME, so the original's DT_NEEDED "libocm.so" comes from the file name found
          with -locm: it must be copied as libs/libocm.so).
  The stub is interface taken from the original (docs/original-bytes.md, item 4).
  stub:   libfreetype.so.6 generated from the original (libfreetype_stub.s): soname
          libfreetype.so.6, the symbols the original resolves against FreeType (first library
          in NEEDED order), with TYPE, BINDING and SIZE from the original's .dynsym. The
          Definitive Patch libfreetype is a different build (4/10 sizes equal): it only tells
          which symbols it resolves. Assembled with the Sony gas and linked -shared with
          ld 2.9EE. It holds names and sizes (.dynsym metadata, like config/symbol_addrs.txt)
          and two values taken from the original's GOT entries, _gp_disp and
          _procedure_table_size of the real libfreetype (printed in the log): no other byte
          of the original.
  script: libc.so (GROUP ( libc.so.6 ) without libc_nonshared.a: stat/lstat are our own
          units), and the development names libm.so, libpthread.so, libdl.so, libcrypt.so,
          libstdc++.so for -l.

Also writes libs/symbols_report.txt: for each UND symbol of the original, the first library in
NEEDED order that defines it, and the size comparison.
"""
import os, shutil, subprocess, sys
from elftools.elf.elffile import ELFFile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from paths import LIBS, DP, KIT_LIB, ORIG, AS, LD, LINKER_GEN  # noqa: E402

SHA1 = {  # expected third-party files (libc.so.6 and ld.so.1: those of PSBBN)
    'libc.so.6': '744284e54196c4a97948c7498689d0525ec4232b',
    'ld.so.1': 'e2827e1661db147f3024cf80213ab17adf5fe390',
    'libm.so.6': '34bbc64f50c36b55d3c115999dcff8c1e95e5776',
    'libpthread.so.0': '3cdda9d1c3d3b2010cdfdfd7d789c4884ae6e8f1',
    'libdl.so.2': '36df0e2cb1b5d4c714a0906eb01ef8970ef096bf',
    'libcrypt.so.1': 'bddbfbb51d76cbef3a960740ac3ebb2fa79e5ee3',
    'libstdc++-libc6.2-2.so.3': 'cb6dba42929cf2fadf8505961ae1b934e454bcb4',
    'libocm.so': 'b594348548288815bfd054fb531b136ab55db067',
}


def sha1(p):
    import hashlib
    return hashlib.sha1(open(p, 'rb').read()).hexdigest()


def dyn_defs(path):
    """{name: (type, binding, size, version)} of the symbols defined in .dynsym."""
    e = ELFFile(open(path, 'rb'))
    ds = e.get_section_by_name('.dynsym')
    ver = {}
    vs = e.get_section_by_name('.gnu.version')
    vd = e.get_section_by_name('.gnu.version_d')
    names = {}
    if vd is not None:
        for v, aux in vd.iter_versions():
            names[v['vd_ndx']] = next(aux).name
    out = {}
    for i, s in enumerate(ds.iter_symbols()):
        if s['st_shndx'] == 'SHN_UNDEF' or not s.name:
            continue
        v = None
        if vs is not None:
            idx = vs.get_symbol(i)['ndx']
            if isinstance(idx, int):
                v = names.get(idx & 0x7fff)
        out.setdefault(s.name, (s['st_info']['type'], s['st_info']['bind'], s['st_size'], v))
    return out


def orig_imports():
    e = ELFFile(open(ORIG, 'rb'))
    ds = e.get_section_by_name('.dynsym')
    vs = e.get_section_by_name('.gnu.version')
    vr = e.get_section_by_name('.gnu.version_r')
    vnames = {}
    for v, aux in vr.iter_versions():
        for a in aux:
            vnames[a['vna_other']] = (v.name, a.name)
    out = []
    for i, s in enumerate(ds.iter_symbols()):
        if s['st_shndx'] != 'SHN_UNDEF' or not s.name:
            continue
        idx = vs.get_symbol(i)['ndx']
        vv = vnames.get(idx) if isinstance(idx, int) else None
        out.append(dict(name=s.name, type=s['st_info']['type'], bind=s['st_info']['bind'],
                        size=s['st_size'], value=s['st_value'], ver=vv))
    return out


def orig_got_values(names):
    """Values of the original's global GOT entries for the symbols `names` (.dynsym index -> slot:
    (dynindx - DT_MIPS_GOTSYM + DT_MIPS_LOCAL_GOTNO) * 4)."""
    import struct
    e = ELFFile(open(ORIG, 'rb'))
    tags = {t.entry.d_tag: t.entry.d_val for t in e.get_section_by_name('.dynamic').iter_tags()}
    got = e.get_section_by_name('.got').data()
    idx = {s.name: i for i, s in enumerate(e.get_section_by_name('.dynsym').iter_symbols())}
    return [struct.unpack_from('<I', got, (idx[n] - tags['DT_MIPS_GOTSYM'] + tags['DT_MIPS_LOCAL_GOTNO']) * 4)[0]
            for n in names]


def main():
    kit_libc = '--kit-libc' in sys.argv
    if not os.path.isdir(DP):
        sys.exit(f'error: {DP} missing: run python3 tools/relink/fetch.py')
    os.makedirs(LIBS, exist_ok=True)
    copies = {
        'libc.so.6': (f'{KIT_LIB}/libc.so.6' if kit_libc else f'{DP}/libc-2.2.2.so'),
        'libm.so.6': f'{KIT_LIB}/libm.so.6',
        'libpthread.so.0': f'{KIT_LIB}/libpthread.so.0', 'libdl.so.2': f'{KIT_LIB}/libdl.so.2',
        'libcrypt.so.1': f'{KIT_LIB}/libcrypt.so.1',
        # ld.so.1 only lets ld resolve libc's dependencies (-rpath-link), it is not in
        # NEEDED; it also provides the sizes of malloc/free/realloc/calloc
        'ld.so.1': (f'{KIT_LIB}/ld.so.1' if kit_libc else f'{DP}/ld-2.2.2.so'),
        'libstdc++-libc6.2-2.so.3': f'{DP}/libstdc++-3-libc6.2-2-2.10.0.so',
        'libocm.so': f'{DP}/libocm.so.1.3',
    }
    for dst, src in copies.items():
        shutil.copyfile(os.path.realpath(src), f'{LIBS}/{dst}')
        if not (kit_libc and dst in ('libc.so.6', 'ld.so.1')) and sha1(f'{LIBS}/{dst}') != SHA1[dst]:
            sys.exit(f'error: unexpected sha1 of {dst} ({src})')
    ft_ref = f'{DP}/libfreetype.so.6.1.0'
    # development names for -l (ld 2.9 looks for libX.so and libX.a)
    open(f'{LIBS}/libc.so', 'w').write(
        '/* GNU ld script: like the kit\'s but without libc_nonshared.a\n'
        '   (stat/lstat are src/lib/libc_stat.c, src/lib/libc_lstat.c) */\n'
        'GROUP ( libc.so.6 )\n')
    for dev, real in [('libm.so', 'libm.so.6'), ('libpthread.so', 'libpthread.so.0'),
                      ('libdl.so', 'libdl.so.2'), ('libcrypt.so', 'libcrypt.so.1'),
                      ('libstdc++-libc6.2-2.so', 'libstdc++-libc6.2-2.so.3')]:
        if os.path.lexists(f'{LIBS}/{dev}'):
            os.remove(f'{LIBS}/{dev}')
        os.symlink(real, f'{LIBS}/{dev}')

    # NEEDED order of the original (readelf -d): the first that defines a symbol wins
    needed = [('libfreetype.so.6', ft_ref), ('libpthread.so.0', f'{LIBS}/libpthread.so.0'),
              ('libcrypt.so.1', f'{LIBS}/libcrypt.so.1'), ('libm.so.6', f'{LIBS}/libm.so.6'),
              ('libdl.so.2', f'{LIBS}/libdl.so.2'),
              ('libstdc++-libc6.2-2.so.3', f'{LIBS}/libstdc++-libc6.2-2.so.3'),
              ('libocm.so', f'{LIBS}/libocm.so'), ('libc.so.6', f'{LIBS}/libc.so.6')]
    defs = {n: dyn_defs(p) for n, p in needed}
    imps = orig_imports()
    rep, stub, nodef = [], [], []
    same = diff = 0
    by_lib = {}
    for s in imps:
        lib = next((n for n, _ in needed if s['name'] in defs[n]), None)
        if lib is None:
            nodef.append(s)
            continue
        by_lib.setdefault(lib, 0)
        by_lib[lib] += 1
        t, b, sz, v = defs[lib][s['name']]
        if lib == 'libfreetype.so.6':
            stub.append(s)
        ok = sz == s['size']
        same += ok
        diff += not ok
        if not ok:
            rep.append(f"  DIFFERS {s['name']:28s} orig {s['size']:6d}  {lib} {sz:6d} ({t}, {b}, version {v}); "
                       f"version required by the original {s['ver']}")
    head = [f'libc.so.6: {copies["libc.so.6"]} (sha1 {sha1(LIBS + "/libc.so.6")[:12]})',
            f'UND symbols of the original: {len(imps)}',
            'Per library (first in NEEDED order that defines them): '
            + ', '.join(f'{k} {v}' for k, v in by_lib.items()),
            f'Size equal to the original: {same}; different: {diff} (libfreetype here is the one of the '
            'Definitive Patch, only to know which symbols it resolves: in the stub the sizes are those of the original)',
            f'Without a definition: {len(nodef)} ' + ', '.join(s['name'] for s in nodef)]
    if any(s['name'] not in LINKER_GEN for s in nodef):
        head.append('  WARNING: symbols without a definition that are not generated by the linker!')
        print('warning: make_libs: symbols without a definition that are not generated by the linker: '
              + ', '.join(s['name'] for s in nodef if s['name'] not in LINKER_GEN), file=sys.stderr)
    open(f'{LIBS}/symbols_report.txt', 'w').write('\n'.join(head + [''] + rep) + '\n')
    print('\n'.join(head + rep))

    # ---- libfreetype.so.6 stub ----------------------------------------------------
    L = ['# Stub of libfreetype.so.6 for the link (generated by make_libs.py).',
         '# Interface only: names, type, binding and size from the .dynsym of orig/bn-jpn.elf',
         '# (ld copies the size of the definition into the executable\'s .dynsym). The bodies',
         '# are zeros: the library is not executable. Symbols = those the original resolves',
         '# against the first NEEDED library (libfreetype), taken from the definitions of the',
         '# Definitive Patch libfreetype.so.6.1.0 (a different build from the one Sony linked).',
         '\t.set\tnoreorder', '\t.text', '\t.align\t4']
    for s in sorted(stub, key=lambda x: x['name']):
        n, sz = s['name'], s['size']
        if s['type'] == 'STT_FUNC':
            L += [f'\t.globl\t{n}' if s['bind'] == 'STB_GLOBAL' else f'\t.weak\t{n}',
                  f'\t.type\t{n},@function', f'\t.ent\t{n}', f'{n}:',
                  f'\t.space\t{max(sz, 4)}', f'\t.end\t{n}', f'\t.size\t{n},{sz}', '\t.align\t2']
        else:
            L += ['\t.data', f'\t.globl\t{n}', f'\t.type\t{n},@object', f'{n}:',
                  f'\t.space\t{max(sz, 4)}', f'\t.size\t{n},{sz}', '\t.text']
    # _gp_disp and _procedure_table_size: ld 2.9EE writes into the global GOT entry of a symbol
    # without GOT relocations the value of the first definition (elflink.h elf_link_output_extsym
    # -> elf32-mips.c mips_elf_finish_dynamic_symbol, gval written before st_value is forced): in
    # the original that is the definition of the real libfreetype, i.e. Its gp and its
    # procedure_count (#PDR + 2, only with -shared and an input .rtproc section). The stub
    # reproduces them: gp from the script (_gp = ...), one procedure with .cpload so that
    # _gp_disp enters its .dynsym, empty local procedures up to #PDR = procedure_count - 2, and
    # .rtproc. The two values are read from the original's GOT entries (metadata, like the sizes).
    gp_val, pts_val = orig_got_values(['_gp_disp', '_procedure_table_size'])
    npdr = sum(1 for s in stub if s['type'] == 'STT_FUNC') + 1
    L += ['\t.text', '\t.align\t2', '\t.ent\t__ft_gpref', '__ft_gpref:', '\t.set\tnoreorder',
          '\t.cpload\t$25', '\t.set\treorder', '\tj\t$31', '\t.end\t__ft_gpref']
    for i in range(pts_val - 2 - npdr):
        L += [f'\t.ent\t__ft_p{i}', f'__ft_p{i}:', '\t.space\t4', f'\t.end\t__ft_p{i}']
    L += ['\t.section\t.rtproc,"a",@progbits', '\t.word\t0']
    open(f'{LIBS}/libfreetype_stub.s', 'w').write('\n'.join(L) + '\n')
    # assembled with the Sony gas (like tools/check.py) and linked -shared with the original's soname
    subprocess.run([AS, '-mips2', '-m5900', '-KPIC', '-o', f'{LIBS}/libfreetype_stub.o',
                    f'{LIBS}/libfreetype_stub.s'], check=True)
    script = subprocess.run([LD, '-shared', '--verbose'], check=True, capture_output=True,
                            text=True).stdout.split('=' * 50)[1]
    assert script.count('_gp = ALIGN(16) + 0x7ff0;') == 1
    open(f'{LIBS}/libfreetype_stub.x', 'w').write(
        script.replace('_gp = ALIGN(16) + 0x7ff0;', f'_gp = {gp_val:#x};'))
    subprocess.run([LD, '-shared', '-T', f'{LIBS}/libfreetype_stub.x', '-soname', 'libfreetype.so.6',
                    '-o', f'{LIBS}/libfreetype.so.6', f'{LIBS}/libfreetype_stub.o'], check=True)
    got = {s.name: s['st_value'] for s in
           ELFFile(open(f'{LIBS}/libfreetype.so.6', 'rb')).get_section_by_name('.dynsym').iter_symbols()}
    if (got.get('_gp_disp'), got.get('_procedure_table_size')) != (gp_val, pts_val):
        sys.exit(f'error: libfreetype stub: _gp_disp/_procedure_table_size {got.get("_gp_disp")}/'
                 f'{got.get("_procedure_table_size")}, expected {gp_val:#x}/{pts_val:#x}')
    print(f'libfreetype stub: _gp_disp 0x{gp_val:x} and _procedure_table_size {pts_val} taken from the original '
          f'(GOT entries of orig/bn-jpn.elf, docs/original-bytes.md item 4); {pts_val - 2 - npdr} empty procedures')
    if os.path.lexists(f'{LIBS}/libfreetype.so'):
        os.remove(f'{LIBS}/libfreetype.so')
    os.symlink('libfreetype.so.6', f'{LIBS}/libfreetype.so')
    print(f'libfreetype stub: {len(stub)} symbols -> {LIBS}/libfreetype.so.6')


if __name__ == '__main__':
    main()
