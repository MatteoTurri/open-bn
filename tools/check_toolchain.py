#!/usr/bin/env python3
"""Guard for the only compiler patch: the compiler is not patched.

A mismatch is always source or flags, never the compiler, with one
exception: the MIPS `exception_receiver` pattern, which the Sony SRPM lacks.
Rationale and requirements: docs/compiler.md (the patch header repeats them).

This makes the rule executable: the
allowed patches are pinned by list and by HASH, so adding a second compiler
patch, or changing the existing one, fails the check. A second patch would
need the same full justification as the first, not just a new entry here.

It also checks that every Dockerfile that rebuilds gcc applies the patch and
builds cc1/cc1plus with -msse2 -mfpmath=sse, and pins by sha256 the binaries
the gate and the link were verified with (docker/toolchain/, gitignored, built
by `make sony-gcc` and `make sony-binutils`): a rebuild that does not produce
the same compiler and assembler/linker fails here instead of in the gate.

Usage: python3 tools/check_toolchain.py
"""
import hashlib
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# our allowed compiler patches: name -> sha256. A single entry.
ALLOWED = {
    'mips-exception-receiver.patch':
        '15c47ced1ea21129ce1b8a1304380f5487d7fe56428377bda43dabe88f26168d',
}
# the toolchain binaries the gate and the link (SHA1 identical to the original)
# were verified with: path -> sha256. cc1/cc1plus built with -msse2 -mfpmath=sse.
BINARIES = {
    'docker/toolchain/sony/cc1':
        '8310e8b8d260f49a2f724bc311ce488a2c85012557dd85277a635ee51e02989e',
    'docker/toolchain/sony/cc1plus':
        'c23826fcee3f8ca058a4d6fd14f612ab4dedab0e1cee5ce3669004ca164f930a',
    'docker/toolchain/sony/cpp':
        '976864341074cd443f84f0b8d74b37e6860feef4a73b1f430d18cac6b2a0b8a5',
    'docker/toolchain/sony/gcc':
        '69175f2b11e9eff897eb3a32589d7069081573d8c008a3fe2f234852599c3172',
    'docker/toolchain/sony/g++':
        '9a85e2d1824ddffee48ebb6da9a55dd7c609aa24eb6907b6b1a8cfb78e1bf014',
    'docker/toolchain/sonybinutils/mipsEEel-linux-as':
        '03c7cfa969b35e11680a880de6c31eb3ff9cb738d95000ed5875e6532f247bb3',
    'docker/toolchain/sonybinutils/mipsEEel-linux-ld':
        '3c0c4b2baa63bd385e25a2385752913ecc92ea003abf93a995d60203fa87df42',
}
# compatibility patches for modern hosts; they do not touch codegen
NEUTRAL = {'obstack-2.95.2.h.patch'}


def main():
    d = os.path.join(ROOT, 'docker/patches')
    present = {f for f in os.listdir(d) if f.endswith('.patch')} - NEUTRAL
    bad = 0

    extra = present - set(ALLOWED)
    if extra:
        bad = 1
        for f in sorted(extra):
            print(f'toolchain  error: unratified compiler patch docker/patches/{f}'
                  ' (exception_receiver is the only allowed one, see docs/compiler.md)')

    for f, want in ALLOWED.items():
        p = os.path.join(d, f)
        if not os.path.exists(p):
            bad = 1
            print(f'toolchain  error: ratified patch {f} is missing')
            continue
        got = hashlib.sha256(open(p, 'rb').read()).hexdigest()
        if got != want:
            bad = 1
            print(f'toolchain  error: {f} modified: sha256 {got}, expected {want}')

    # the Dockerfile that rebuilds gcc must apply it, or the toolchain lacks it
    for dockerfile in ('docker/Dockerfile.sonygcc',):
        path = os.path.join(ROOT, dockerfile)
        if not os.path.exists(path):
            continue
        dk = open(path).read()
        for f in ALLOWED:
            if f not in dk:
                bad = 1
                print(f'toolchain  error: {f} is not applied by {dockerfile}')

    # precision of gcc's internal `double`s: IEEE 53 bit like the original.
    # With the 80-bit x87 of -m32, allocno_compare breaks ties the other way
    # (static_006D8938).
    for dockerfile in ('docker/Dockerfile.sonygcc',):
        path = os.path.join(ROOT, dockerfile)
        if not os.path.exists(path):
            continue
        for line in open(path):
            if line.startswith('RUN make -C') and 'CFLAGS=' in line \
                    and '-mfpmath=sse' not in line:
                bad = 1
                print(f'toolchain  error: {dockerfile} builds without'
                      f' -msse2 -mfpmath=sse: {line.strip()}')
    # the built binaries: the ones the gate and the link were verified with
    for rel, want in BINARIES.items():
        p = os.path.join(ROOT, rel)
        if not os.path.exists(p):
            bad = 1
            print(f'toolchain  error: {rel} missing (make sony-gcc / make sony-binutils)')
            continue
        got = hashlib.sha256(open(p, 'rb').read()).hexdigest()
        if got != want:
            bad = 1
            print(f'toolchain  error: {rel} differs from the verified build:'
                  f' sha256 {got}, expected {want}')
    if bad:
        return 1
    print(f'toolchain  ok (1 allowed compiler patch: exception_receiver;'
          f' {len(BINARIES)} binaries pinned)')
    return 0


if __name__ == '__main__':
    sys.exit(main())
