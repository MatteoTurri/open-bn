#!/bin/sh
# Prepares docker/toolchain/include/ to compile C++ with the Sony g++:
#   - libstdc++/libio/ginclude/cp-inc from the gcc tarball of the SRPM (with the Sony patches)
#   - fixinc/limits.h (glimits + include_next to the kit's glibc)
# Prerequisites: docker/sonygcc/ (extracted SRPM), docker/kit/rootfs/usr/include
# (GLIBC-_* and KERNEL_3 RPMs of disc 2 extracted, usr/src/linux symlink created).
# Runs in the open-bn-env container from the repo root (make headers), so the host needs no patch/tar.
set -e
[ -f /.dockerenv ] || { echo "error: run it with make headers (it runs in the container)" >&2; exit 2; }
command -v patch >/dev/null || { echo "error: no patch in open-bn-env, rebuild the image with make env" >&2; exit 2; }
ROOT=$(pwd)
SRC=$(mktemp -d)
trap 'rm -rf "$SRC"' EXIT

tar xzf docker/sonygcc/gcc-2.95.2.tar.gz -C "$SRC"
cd "$SRC/gcc-2.95.2"
# the same patches, in the same order, as docker/Dockerfile.sonygcc: they must all apply
for p in gcc-2.95.2-frankengcc-patches.patch gcc-2.95.2-single-float-const.patch gcc-ps2linux-1.0.0.patch; do
    out=$(patch -p1 -f < "$ROOT/docker/sonygcc/$p" 2>&1) || {
        echo "$out" | grep -E 'FAILED|malformed|can.t find|Reversed|garbage' | head -20 >&2
        echo "error: docker/sonygcc/$p does not apply" >&2
        exit 1
    }
done
cd "$ROOT"

I=docker/toolchain/include
rm -rf "$I" && mkdir -p "$I/fixinc"
cp -r "$SRC/gcc-2.95.2/libstdc++" "$I/"
cp -r "$SRC/gcc-2.95.2/libio" "$I/"
cp -r "$SRC/gcc-2.95.2/gcc/ginclude" "$I/"
cp -r "$SRC/gcc-2.95.2/gcc/cp/inc" "$I/cpinc"

{
    printf '#ifndef _GCC_LIMITS_H_\n#define _GCC_LIMITS_H_\n'
    cat "$SRC/gcc-2.95.2/gcc/glimits.h"
    printf '\n#if defined __GNUC__ && !defined _GCC_NEXT_LIMITS_DONE\n#define _GCC_NEXT_LIMITS_DONE\n#undef MB_LEN_MAX\n#include_next <limits.h>\n#endif\n#endif\n'
} > "$I/fixinc/limits.h"

echo "headers    C++ headers ready in $I"
