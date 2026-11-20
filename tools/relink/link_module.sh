#!/bin/sh
# Link a module (BN_TARGET=<name>) with ld 2.9EE: the shared object build/modules/<name>/<name>.so.
#
# Usage: BN_TARGET=<name> sh tools/relink/link_module.sh   (repository root, in the container;
#        `BN_TARGET=<name> make link` calls it after a full `make check` of the module and fetch.py)
# Inputs: the gate's objects (build/modules/<name>/check/<unit>/out.o) in the order of the module's
#   units.txt, the kit's crti.o/crtn.o and the kit's crtbegin.o/crtend.o (GCC-29_1.RPM, fetch.py).
#   Constants come from tools/relink/paths.py and tools/target.py.
# The link line is the one of `g++ -shared`: crti, crtbegin, the objects, crtend, crtn. As for bn,
#   the two crtstuff units are replaced by the kit's crtbegin.o/crtend.o: our crtstuff.c also
#   contains call_gmon_start, the .text of crti.o, and would duplicate it. The modules have no
#   DT_NEEDED: their undefined symbols are resolved by bn and the libraries it has loaded, so no
#   library is linked. ld's default script for elf32lsmip -shared gives the base 0x5ffe0000.
# Outputs (build/modules/<name>/link/), deleted at the start:
#   <name>.unstripped.so, <name>.map   ld output and map
#   <name>.stripped.so                 mipsEEel-linux-strip 2.9EE with no options
#   <name>.so (in build/modules/<name>/) the stripped object with relvaddr and cpt.offset of
#                                      .compact_rel copied from the original (compact_rel.py)
# Comparison: `BN_TARGET=<name> make link-cmp`.
set -e
eval "$(python3 tools/relink/paths.py --sh)"
eval "$(python3 tools/target.py --sh)"
GCCLIB=$(dirname "$LIBGCC")
KIT=docker/kit/rootfs/usr/lib
H=$T_BUILD/link
mkdir -p $H
rm -f $H/$T_NAME.unstripped.so $H/$T_NAME.stripped.so $H/$T_NAME.map $T_BUILD/$T_NAME.so
# the objects in units.txt order, crtstuff units replaced by the kit's crtbegin/crtend
OBJS=$(python3 - <<'EOF'
import os, re, sys
sys.path.insert(0, 'tools')
import target
t = target.get()
crt = {'ref/gcc-2.95.2/crtstuff.c': 'crtbegin.o', 'ref/gcc-2.95.2/crtstuff_end.c': 'crtend.o'}
out = []
for raw in open(t.units):
    line = raw.split('#', 1)[0].strip()
    if not line:
        continue
    src = line.split('|')[0].strip()
    if src in crt:
        out.append('@' + crt[src])
        continue
    o = f"{t.objdir}/{re.sub(r'[^A-Za-z0-9_]', '_', src)}/out.o"
    if not os.path.exists(o):
        sys.exit(f'error: {o} missing: run a full BN_TARGET={t.name} make check first')
    out.append(o)
print(' '.join(out))
EOF
)
OBJS=$(echo "$OBJS" | sed "s#@crtbegin.o#$GCCLIB/crtbegin.o#; s#@crtend.o#$GCCLIB/crtend.o#")
$LD -m elf32lsmip -shared -soname $T_NAME.so -o $H/$T_NAME.unstripped.so -Map $H/$T_NAME.map \
    $KIT/crti.o $OBJS $KIT/crtn.o 2> $H/ld.log || { cat $H/ld.log >&2; exit 1; }
sed 's/^/warning: ld: /' $H/ld.log >&2
$STRIP -o $H/$T_NAME.stripped.so $H/$T_NAME.unstripped.so
# relvaddr and cpt.offset of .compact_rel: bytes taken verbatim from the original, as for bn
# (tools/relink/compact_rel.py, docs/original-bytes.md)
python3 tools/relink/compact_rel.py $H/$T_NAME.stripped.so $T_BUILD/$T_NAME.so --orig $T_ELF \
    > $H/compact_rel.log || { cat $H/compact_rel.log >&2; exit 1; }
n=$(grep -c 'REJECTED' $H/compact_rel.log || true)
[ "$n" = 0 ] || echo "warning: compact_rel: $n .compact_rel entries rejected, see $H/compact_rel.log" >&2
echo "linked $T_BUILD/$T_NAME.so"
