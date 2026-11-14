#!/bin/sh
# match.sh <symbol> <source> [extra compiler flags]
#
# Compiles <source> with the Sony toolchain (gcc/g++ 2.95.2-3a r5900) and
# compares the function <symbol> with the original in orig/bn-jpn.elf.
# <symbol> can also be a nameless function, `static_<ADDR>`
# (config/static_funcs.txt): the compared function is the local function of our
# .o that the gate assigned to that address (build/check_statics.json: anchors,
# content, similarity) or, failing that, the one the anchors (tools/statics.py)
# place there.
#   LOCAL=name tools/match static_XXXXXXXX file.c   forces the local to compare
#   (without it, if no local sits exactly at that address, the one overlapping its
#   body is taken, with a "warning: out of alignment").
# VERBOSE=1: informational notes (flags used, DNAS reference, chosen local, every
#   differing line). Without it: only the result, warnings and errors.
# Exit 0 = perfect match. Runs inside open-bn-env (use tools/match from the host).
#
# Examples:
#   sh tools/match.sh adler32 ref/zlib-1.1.3/adler32.c
#   sh tools/match.sh AnimChildren__6Object src/scene/object.cpp
set -e

SYM="$1"; SRC="$2"
[ -n "$SYM" ] && [ -n "$SRC" ] || { echo "usage: match.sh <symbol> <source> [flags]"; exit 2; }
# VERBOSE=1: informational notes (flags used, DNAS reference, local of the static functions)
note() { case "${VERBOSE:-}" in ''|0) ;; *) echo "note: $*" ;; esac; }
shift 2
EXTRA="$*"

# --- unit flags -------------------------------------------------------------
# The gate compiles each unit with the cflags of its row in config/units.txt
# (tools/check.py); here they are the default too. Flags passed on the command
# line replace them (to try a different flag), and in that case the units.txt
# flags are printed so the difference is visible. Compiling with the wrong
# flags can still find the function and look plausible (e.g. without
# `-fno-exceptions` a native grows `__throw`/`__builtin_delete`/`terminate`).
# NB: never `gsub` on $1 inside the test: changing a field rebuilds $0 with
# OFS (a space) and the `|` separators disappear, so the cflags extraction
# below would always be empty. Normalize into a temporary variable instead.
UROW=$(awk -F'|' -v s="$SRC" '{t=$1; gsub(/^[ \t]+|[ \t]+$/,"",t); if (t==s) {print; exit}}' config/units.txt)
UCFLAGS=$(printf '%s' "$UROW" | awk -F'|' '{gsub(/^[ \t]+|[ \t]+$/,"",$2); print $2}')
UASFLAGS=$(printf '%s' "$UROW" | awk -F'|' '{gsub(/^[ \t]+|[ \t]+$/,"",$3); print $3}')
if [ -z "$UROW" ]; then
    echo "warning: $SRC has no row in config/units.txt, compiling with -O2${EXTRA:+ $EXTRA} only"
elif [ -n "$EXTRA" ]; then
    note "command-line flags $EXTRA replace the units.txt flags (${UCFLAGS:-<none>})"
else
    EXTRA="$UCFLAGS"
    [ -n "$EXTRA" ] && note "flags from config/units.txt: $EXTRA"
fi
# same for the assembler: an explicit ASEXTRA wins, otherwise units.txt
[ -z "${ASEXTRA:-}" ] && [ -n "$UASFLAGS" ] && { ASEXTRA="$UASFLAGS"; note "asflags from config/units.txt: $ASEXTRA"; }

TOOL=${TOOL:-docker/toolchain/sony}
I=docker/toolchain/include
SYMTAB=config/symbol_addrs.txt
ELF=orig/bn-jpn.elf

# --- symbol lookup by exact name (grep -F alone matches substrings: 47 symbols
# of the table are suffixes of others, e.g. Color__8gm_graphRC4vec4 would
# resolve to ClearColor__...) ----------------------------------------------------
# Nameless functions (static_<ADDR>, tools/statics.py) are in
# config/static_funcs.txt, same format: their identity is the address.
STATIC=""
case "$SYM" in static_[0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F]) STATIC=1; SYMTAB=config/static_funcs.txt ;; esac
LINE=$(grep -F "$SYM = 0x" "$SYMTAB" | awk -F' = 0x| = ' -v s="$SYM" '{n=$1; if(n==s) print}' | head -1) || true
[ -n "$LINE" ] || { echo "error: symbol '$SYM' not in $SYMTAB"; exit 2; }
ADDR=$(printf '%s' "$LINE" | sed 's/.* = \(0x[0-9A-Fa-f]*\);.*/\1/')
SIZE=$(printf '%s' "$LINE" | sed 's/.*size:\(0x[0-9A-Fa-f]*\).*/\1/')
STOP=$(printf '0x%X' $(( ADDR + SIZE )))

# per-symbol directory name: template symbols (e.g. _Rb_tree keyed on basic_string)
# exceed the 255-byte file name limit -> truncate + hash of the full name
# (collision-safe) when too long.
if [ "${#SYM}" -gt 200 ]; then
    OUT="build/match/$(printf '%s' "$SYM" | cut -c1-160)__$(printf '%s' "$SYM" | md5sum | cut -c1-12)"
else
    OUT="build/match/$SYM"
fi
mkdir -p "$OUT"
rm -f "$OUT/orig.txt" "$OUT/ours.txt"   # never leave stale listings behind if the compilation fails

# --- compilation ------------------------------------------------------------
INC_C="-nostdinc -Isrc -I$I/fixinc -I$I/ginclude -Idocker/kit/rootfs/usr/include"
# libstdc++ headers from the path of Sony's installation (`__FILE__` of the asserts of
# <std/bastring.cc>), as in the gate: tools/check.py, CROSS_CXX
CROSS_CXX=/usr/cross/mipsEEel-linux/include/g++-3
[ -e "$CROSS_CXX" ] || { mkdir -p "$(dirname "$CROSS_CXX")" && ln -sfn "$(pwd)/$I/libstdc++" "$CROSS_CXX"; } 2>/dev/null
[ -e "$CROSS_CXX/std/bastring.h" ] || { echo "error: link $CROSS_CXX not created: old image? rerun make env" >&2; exit 2; }
INC_CXX="-nostdinc -Isrc -I$I/fixinc -I$CROSS_CXX -I$I/libstdc++/stl -I$I/libstdc++/std -I$I/cpinc -I$I/libio -I$I/ginclude -Idocker/kit/rootfs/usr/include"

case "$SRC" in
    *.cpp|*.cc|*.cxx|*.C) CC="$TOOL/g++"; INC="$INC_CXX" ;;
    *.c)                  CC="$TOOL/gcc"; INC="$INC_C" ;;
    *) echo "error: unrecognized source extension: $SRC"; exit 2 ;;
esac

# Assets: the bytes the sources include with `#include "../assets/..."` are extracted
# from the original ELF before compiling (config/assets.txt)
python3 tools/extract_assets.py > /dev/null || { python3 tools/extract_assets.py; exit 2; }
# original path for `__FILE__` (config/unit_paths.txt, tools/srcpath.py), as in the
# gate: cwd and argument as in Sony's build for the listed units, unchanged for the others
# the unit's -I flags go in front of the kit sysroot (as in tools/check.py: unit_includes_first)
UINC=$(printf '%s\n' $EXTRA | grep '^-I' | tr '\n' ' ')
EXTRA=$(printf '%s\n' $EXTRA | grep -v '^-I' | tr '\n' ' ')
INC=$(printf '%s' "$INC" | sed "s|-Idocker/kit/rootfs/usr/include|$UINC-Idocker/kit/rootfs/usr/include|")
sh -c "$(python3 tools/srcpath.py "$SRC" "$CC -B$TOOL/ -S -O2 $INC $EXTRA -o $OUT/out.s")"

# DNAS (docs/dnas.md): the protection scaffolding is substituted into the .s
# between gcc and gas in place of the calls and markers of src/dnas/dnas_prot.h, as in
# the gate (tools/check.py). gcc's .s stays next to it as out.raw.s.
if grep -qE '#DNASSTR|#DNASEND|#DNASBEG' "$OUT/out.s"; then
    cp "$OUT/out.s" "$OUT/out.raw.s"
    python3 tools/dnas_subst.py "$OUT/out.raw.s" "$OUT/out.s" || { echo "error: DNAS substitution failed (tools/dnas_subst.py)"; exit 2; }
fi

# period assembler: rebuilt Sony binutils 2.9EE (no emulation)
# per-library asflags via ASEXTRA (SQLite uses -g2 = mips_optimize=1, no
# branch swap; zlib/libpng without -g = swap enabled)
docker/toolchain/sonybinutils/mipsEEel-linux-as -mips2 -m5900 ${ASEXTRA:-} -o "$OUT/out.o" "$OUT/out.s"

# --- extraction and comparison ----------------------------------------------
# -r on ours: the relocations mark the immediates decided by the linker (wildcard)
# -z always: without it objdump collapses runs of zeros (>=2 consecutive MIPS nops)
# into "...": the addresses vanish from the branch-target table of
# normalize_asm.py and a branch to a label made only of nops becomes a false
# EXT (e.g. SHA1_Final, label $L89 of 2 nops).
# DNAS: for the 163 self-modifying functions the reference
# is not the shipped binary (which carries the ciphertext) but the whole function
# of the decrypted image, scaffolding included, already written by `make split`
# into expected/asm/ with the header `# DNAS REFERENCE` (a marker written
# by tools/split.py: keep the two in sync) and the descriptor words written with the
# text of the build's placeholder. It is the same reference as the gate's
# (tools/check.py via tools/dnas_ref.py).
REF="expected/asm/$(printf '%08X' "$ADDR")_$SYM.s"
if [ -f "$REF" ] && head -2 "$REF" | grep -q 'DNAS REFERENCE'; then
    note "DNAS reference (decrypted image, scaffolding included, descriptors masked): $REF"
    python3 tools/normalize_asm.py < "$REF" > "$OUT/orig.txt"
else
    mipsel-linux-gnu-objdump -z -d --start-address="$ADDR" --stop-address="$STOP" "$ELF" \
        | python3 tools/normalize_asm.py > "$OUT/orig.txt"
fi
# static: in our .o the function carries the name we gave it; the link with
# static_<ADDR> is made by the anchors (tools/statics.py --locate)
OURSYM="$SYM"
# LOCAL=name forces the local (when the anchors are not enough: several
# overlapping locals, or a different file order). Without it, automatic fallback
# to the local overlapping the original body ("out of alignment" warning).
if [ -n "$STATIC" ]; then
    OURSYM=$(python3 tools/statics.py --locate "$OUT/out.o" --for "$SYM" ${LOCAL:+--local "$LOCAL"}) || exit 2
    note "$SYM = local function '$OURSYM' of our .o${LOCAL:+ (forced by LOCAL=)}"
fi
mipsel-linux-gnu-objdump -z -r --disassemble="$OURSYM" "$OUT/out.o" \
    | python3 tools/normalize_asm.py > "$OUT/ours.txt"

[ -s "$OUT/ours.txt" ] || { echo "error: '$OURSYM' not emitted by the compiler (check the source)"; exit 2; }

rc=0
python3 tools/compare_asm.py "$OUT/orig.txt" "$OUT/ours.txt" || rc=$?
# GOT symbol identity (tools/gotident.py): MATCH n/n says the instructions are
# identical, but the comparison masks the GOT offsets and does not see which global
# symbol is called or read. The gate judges it; here it shows at once, and a MATCH
# with a different identity outside the debt exits 1 like the gate.
if [ $rc -eq 0 ]; then
    python3 tools/gotident.py --obj "$OUT/out.o" --local "$OURSYM" --unit "$SRC" "$SYM" || rc=1
fi
exit $rc
