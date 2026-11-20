#!/usr/bin/env python3
"""The decompilation targets: the main executable `bn` and the modules it loads with dlopen.

The target is chosen with the environment variable BN_TARGET: unset or `bn` is the main
executable, with the paths it has always had; any name in config/modules.txt is a module,
with the same layout under a `modules/<name>` subdirectory of each root:

                    bn                          module <name>
    original        orig/bn-jpn.elf             orig/modules/<name>.so
    configuration   config/*.txt                config/modules/<name>/*.txt
    sources         src/, ref/                  src/modules/<name>/
    reference asm   expected/asm/               expected/modules/<name>/asm/
    build output    build/                      build/modules/<name>/

The tools take their paths from `target.get()` instead of hard-coding them, so a new module
needs a line in config/modules.txt and nothing else (docs/modules.md).

    python3 tools/target.py            the current target and its paths
    python3 tools/target.py --list     every target, with the original's state
    python3 tools/target.py --sh       the paths as shell assignments (tools/match.sh)
"""
import hashlib
import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REGISTRY = 'config/modules.txt'
MAIN = 'bn'


def modules():
    """{name: (archive member, sha1, size)} from config/modules.txt."""
    out = {}
    for n, raw in enumerate(open(os.path.join(ROOT, REGISTRY)), 1):
        line = raw.split('#', 1)[0].strip()
        if not line:
            continue
        f = [x.strip() for x in line.split('|')]
        if len(f) != 4:
            sys.exit(f'error: {REGISTRY}:{n}: 4 fields expected, found {len(f)}')
        out[f[0]] = (f[1], f[2].lower(), int(f[3]))
    return out


class Target:
    """Paths and identity of one target. All paths are relative to the repository root."""

    def __init__(self, name):
        self.name = name
        self.is_main = name == MAIN
        if self.is_main:
            self.elf = 'orig/bn-jpn.elf'
            self.sha1 = None            # config/bn.yaml (tools/check.py reads it there)
            cfg, exp, build, self.src = 'config', 'expected', 'build', 'src'
        else:
            mods = modules()
            if name not in mods:
                sys.exit(f'error: unknown target {name!r} (BN_TARGET): not bn and not in {REGISTRY}')
            self.member, self.sha1, self.size = mods[name]
            self.elf = f'orig/modules/{name}.so'
            cfg, exp, build = (f'{d}/modules/{name}' for d in ('config', 'expected', 'build'))
            self.src = f'src/modules/{name}'
        self.config = cfg
        self.symtab = f'{cfg}/symbol_addrs.txt'
        self.statics = f'{cfg}/static_funcs.txt'
        self.units = f'{cfg}/units.txt'
        self.expected = exp
        self.asm = f'{exp}/asm'
        self.index = f'{exp}/functions.txt'
        self.build = build
        self.objdir = f'{build}/check'                  # build/check/<unit>/out.o
        self.check_json = f'{build}/check.json'
        self.check_statics_json = f'{build}/check_statics.json'
        self.datacheck_json = f'{build}/datacheck.json'      # tools/datacheck.py
        self.ehcheck_json = f'{build}/ehcheck.json'          # tools/ehcheck.py
        self.got_copies = f'{build}/got_copies.json'
        self.skip_distance = f'{build}/skip_distance.json'
        self.implicit_json = f'{build}/implicit.json'
        # identity debts of the gate (tools/gotident.py)
        self.got_debt = f'{cfg}/got_debt.txt'
        self.gotlocal_debt = f'{cfg}/gotlocal_debt.txt'
        self.gotlo16_debt = f'{cfg}/gotlo16_debt.txt'

    def path(self, rel):
        return os.path.join(ROOT, rel)

    def sections(self):
        """{name: (addr, offset, size, type)} from the section headers of the original."""
        return elf_sections(self.path(self.elf))

    def text(self):
        """(addr, offset, size) of .text."""
        a, o, s, _ = self.sections()['.text']
        return a, o, s

    def gp(self):
        """The value of $gp: .got + 0x7ff0 (MIPS ABI; bn: 0x100D9910)."""
        return self.sections()['.got'][0] + 0x7FF0

    def check_original(self):
        """Exit with a message if the original is missing or (modules) has another sha1."""
        p = self.path(self.elf)
        if not os.path.exists(p):
            how = 'see Building in README.md' if self.is_main else 'run make modules'
            sys.exit(f'error: {self.elf} missing ({how})')
        if self.sha1:
            got = hashlib.sha1(open(p, 'rb').read()).hexdigest()
            if got != self.sha1:
                sys.exit(f'error: sha1 of {self.elf} is {got}, {REGISTRY} says {self.sha1}')


def elf_sections(path):
    """{name: (addr, offset, size, type)} of a 32-bit little-endian ELF (no pyelftools:
    this runs on the host too)."""
    d = open(path, 'rb').read()
    if d[:4] != b'\x7fELF' or d[4] != 1 or d[5] != 1:
        sys.exit(f'error: {path} is not a 32-bit little-endian ELF')
    shoff, = struct.unpack_from('<I', d, 0x20)
    shentsize, shnum, shstrndx = struct.unpack_from('<HHH', d, 0x2E)
    hdrs = [struct.unpack_from('<IIIIIIIIII', d, shoff + i * shentsize) for i in range(shnum)]
    stroff = hdrs[shstrndx][4]
    out = {}
    for nm, typ, _flags, addr, off, size, *_ in hdrs:
        end = d.index(b'\0', stroff + nm)
        out[d[stroff + nm:end].decode()] = (addr, off, size, typ)
    return out


_current = None


def get(name=None):
    """The target named `name`, or the current one (BN_TARGET, default bn)."""
    global _current
    if name is not None:
        return Target(name)
    if _current is None:
        _current = Target(os.environ.get('BN_TARGET') or MAIN)
    return _current


def main():
    if '--sh' in sys.argv[1:]:
        # for shell scripts (tools/match.sh): eval "$(python3 tools/target.py --sh)"
        t = get()
        for k in ('name', 'elf', 'symtab', 'statics', 'units', 'asm', 'build'):
            print(f"T_{k.upper()}='{getattr(t, k)}'")
        return
    if '--list' in sys.argv[1:]:
        for n in [MAIN] + sorted(modules()):
            t = Target(n)
            state = 'present' if os.path.exists(t.path(t.elf)) else 'missing'
            print(f'{n:10s} {t.elf:30s} {state}')
        return
    t = get()
    print(f'target {t.name}')
    for k in ('elf', 'symtab', 'statics', 'units', 'asm', 'index', 'build', 'src'):
        print(f'  {k:8s} {getattr(t, k)}')


if __name__ == '__main__':
    main()
