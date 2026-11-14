#!/usr/bin/env python3
"""Paths and constants shared by the link pipeline (docs/linking.md).

Single source of these constants: the Python scripts import them, tools/relink/link.sh reads
them with `eval "$(python3 tools/relink/paths.py --sh)"` (quoted shell assignments).
All are relative to the repository root and must stay so: the paths given to `as` end up in
the .mdebug of the link (bn.unstripped.elf; the cpt.offset of bn.elf is copied from the
original). Outputs go to build/relink/ (gitignored), downloaded third-party files to
build/relink/dl/; none of this is tracked (copyrighted bytes stay out of the repo,
docs/original-bytes.md).
"""

OUT = 'build/relink'              # pipeline outputs
DL = f'{OUT}/dl'                  # downloaded/extracted third-party files (fetch.py)
LIBS = f'{OUT}/libs'              # shared libraries for -L / -rpath-link (make_libs.py)
BLOBS = f'{OUT}/blobs'            # non-compiled bytes as .o files (make_blobs.py)
MERGED = f'{OUT}/merged'          # unit .o files rebuilt with a blob at head/tail (make_blobs.py)
INPUTS_SHA1 = f'{OUT}/inputs.sha1'  # sha1 of the input .o files (link_order.py; re-checked by link.sh)

ORIG = 'orig/bn-jpn.elf'
UNITS = 'config/units.txt'
LDSCRIPT = 'config/bn.x'          # linker script (tracked; choices and notes inside)
PARTIAL_R = 'config/relink_partial_r.txt'   # objects merged with `ld -r` (partial_r.py, link.sh)

KIT_LIB = 'docker/kit/rootfs/lib'          # glibc 2.2.2 of the kit (make kit)
KIT_USRLIB = 'docker/kit/rootfs/usr/lib'   # crt1.o, crti.o, crtn.o of the kit
KIT_DISC2 = 'docker/kit/tmp/disc2.iso'     # disc 2 of the kit (tools/fetch_kit.py)

# GCC-29_1.RPM of the kit (crtbegin.o, crtend.o, libgcc.a), extracted by fetch.py
GCC29 = f'{DL}/gcc29/x'
GCCLIB = f'{GCC29}/usr/lib/gcc-lib/mipsEEel-linux/2.95.2'
LIBGCC = f'{GCCLIB}/libgcc.a'
LIBGCC_AR = f'{DL}/ar/libgcc'     # members of libgcc.a (missing_syms.py only)
DP = f'{DL}/dp41'                 # files of the Definitive Patch v4.1.0 in use (fetch.py)

BINUTILS = 'docker/toolchain/sonybinutils'
AS = f'{BINUTILS}/mipsEEel-linux-as'
LD = f'{BINUTILS}/mipsEEel-linux-ld'
AR = f'{BINUTILS}/mipsEEel-linux-ar'
STRIP = f'{BINUTILS}/mipsEEel-linux-strip'

# symbols that no .o and no library defines: ld generates them (weak, value 0 in the original).
# They are the only missing symbols allowed (make_libs.py, missing_syms.py)
LINKER_GEN = frozenset({'__gmon_start__', '__start___vpu_symtab', '__stop___vpu_symtab',
                        '__start___dmamem_symtab', '__stop___dmamem_symtab'})


def unit_obj(u):
    """The gate's .o for unit u (same rule as tools/check.py)."""
    import re
    return 'build/check/' + re.sub(r'[^A-Za-z0-9_]', '_', u) + '/out.o'


if __name__ == '__main__':
    import shlex, sys
    if sys.argv[1:] != ['--sh']:
        sys.exit('usage: python3 tools/relink/paths.py --sh')
    for k in ('OUT', 'LIBS', 'INPUTS_SHA1', 'LDSCRIPT', 'PARTIAL_R', 'LIBGCC', 'AS', 'LD', 'STRIP'):
        print(f'{k}={shlex.quote(globals()[k])}')
