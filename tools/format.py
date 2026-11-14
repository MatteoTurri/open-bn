#!/usr/bin/env python3
"""Format the sources of src/ with clang-format 23.1.4, the only version accepted.

    python3 tools/format.py [--check] [FILE ...]

Without FILE: every .c/.cpp/.h under src/. Without --check the files are rewritten in place (`make format`);
with --check nothing is written, the files that are not formatted are listed and the exit code is 1
(the first step of `make check`). Exit 2 if clang-format is missing or of another
version. CLANG_FORMAT=<path> picks the binary (default: clang-format in PATH; the container has it).
The style is .clang-format at the root; ref/ is never formatted (ref/.clang-format disables it).
"""
import os
import re
import subprocess
import sys

VERSION = '23.1.4'
CLANG_FORMAT = os.environ.get('CLANG_FORMAT', 'clang-format')


def sources():
    out = []
    for dp, dn, fn in os.walk('src'):
        dn.sort()
        out += [os.path.join(dp, f) for f in sorted(fn) if f.endswith(('.c', '.cpp', '.h'))]
    return out


def main():
    args = sys.argv[1:]
    check = '--check' in args
    files = [a for a in args if not a.startswith('--')] or sources()
    try:
        ver = subprocess.run([CLANG_FORMAT, '--version'], capture_output=True, text=True).stdout
    except OSError:
        ver = ''
    m = re.search(r'clang-format version (\S+)', ver)
    if not m or m.group(1) != VERSION:
        print(f'error: {CLANG_FORMAT} is not clang-format {VERSION} ({ver.strip() or "not found"}); '
              f'use `make format`, or install it on the host (tools/README.md) and set CLANG_FORMAT', file=sys.stderr)
        return 2
    if not check:
        return subprocess.run([CLANG_FORMAT, '-i'] + files).returncode
    r = subprocess.run([CLANG_FORMAT, '--dry-run', '--Werror'] + files, capture_output=True, text=True)
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
