#!/usr/bin/env python3
"""Format the sources of src/ with clang-format 23.1.5, the only version accepted.

    python3 tools/format.py [--check] [FILE ...]

Without FILE: every .c/.cpp/.h under src/. Without --check the files are rewritten in place (`make format`);
with --check nothing is written, the files that are not formatted are listed and the exit code is 1
(the first step of `make check`). Exit 2 if no clang-format of that version is found.
The binary is the first of CLANG_FORMAT=<path>, .venv/bin/clang-format (`make venv`) and clang-format in PATH
(the container has it) that has the pinned version. --version prints the pinned version.
The style is .clang-format at the root; ref/ is never formatted (ref/.clang-format disables it).
"""
import os
import re
import subprocess
import sys

# Keep in sync with docker/Dockerfile. `make venv` reads it from here.
VERSION = '23.1.5'
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def version_of(exe):
    try:
        out = subprocess.run([exe, '--version'], capture_output=True, text=True).stdout
    except OSError:
        return None
    m = re.search(r'clang-format version (\S+)', out)
    return m.group(1) if m else None


def find_clang_format():
    """First candidate with the pinned version, or (None, [(candidate, version found)])."""
    env = os.environ.get('CLANG_FORMAT')
    candidates = [env] if env else [os.path.join(ROOT, '.venv', 'bin', 'clang-format'), 'clang-format']
    seen = []
    for exe in candidates:
        ver = version_of(exe)
        if ver == VERSION:
            return exe, seen
        seen.append((exe, ver or 'not found'))
    return None, seen


def sources():
    out = []
    for dp, dn, fn in os.walk('src'):
        dn.sort()
        out += [os.path.join(dp, f) for f in sorted(fn) if f.endswith(('.c', '.cpp', '.h'))]
    return out


def main():
    args = sys.argv[1:]
    if '--version' in args:
        print(VERSION)
        return 0
    check = '--check' in args
    files = [a for a in args if not a.startswith('--')] or sources()
    exe, seen = find_clang_format()
    if not exe:
        found = ', '.join(f'{e}: {v}' for e, v in seen)
        print(f'error: no clang-format {VERSION} ({found}); run `make venv` (host) or `make env` (container)',
              file=sys.stderr)
        return 2
    if not check:
        return subprocess.run([exe, '-i'] + files).returncode
    r = subprocess.run([exe, '--dry-run', '--Werror'] + files, capture_output=True, text=True)
    bad = sorted(set(re.findall(r'^(\S+?):\d+:\d+: error:', r.stderr, re.M)))
    for f in bad:
        print(f'not formatted: {f}')
    if r.returncode and not bad:
        print(r.stderr, file=sys.stderr)
    if bad or r.returncode:
        print(f'format     {len(bad)} files not formatted: run `make format`')
        return 1
    print(f'format     {len(files)} files formatted (clang-format {VERSION})')
    return 0


if __name__ == '__main__':
    sys.exit(main())
