#!/usr/bin/env python3
"""Gate of the scripts: every src/bnx/<path>.sma rebuilds its original module byte for byte.

Usage: python3 tools/scriptcheck.py [-v] [PATH ...]

The scripts of bn are Small (Pawn) programs, /opt0/bn/script/<path>.bnx, each an encrypted AMX
module (docs/scripts.md). config/scripts.txt lists them with the sha1 and size of the module
their .bnx carries. This tool compiles every row with the Small 2.1.0 compiler
(docker/toolchain/small/sc, `make small`) and checks the module it writes against those two
values; the originals are not needed. The include files the scripts share are in
src/bnx/include. It fails on a module that differs, a source that does not compile, a row
without its source and a source without its row.

    python3 tools/scriptcheck.py              every script (in the container: `make check`)
    python3 tools/scriptcheck.py game/game    only the named ones (src/bnx/game/game.sma too)
    python3 tools/scriptcheck.py -v           one line per script

The modules, and the files the compiler leaves next to them, stay in build/scripts/.
"""
import hashlib
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REGISTRY = 'config/scripts.txt'
SOURCES = 'src/bnx'
INCLUDE = 'src/bnx/include'
OUT = 'build/scripts'
SC = 'docker/toolchain/small/sc'
FLAGS = ['-d0', '-t0', '-\\', '-C+', '-P+', '-;+']
TIMEOUT = 60


def registry():
    """path -> (sha1 of the .bnx, sha1 of the module, size of the module)."""
    rows = {}
    with open(REGISTRY) as f:
        for n, line in enumerate(f, 1):
            line = line.split('#', 1)[0].strip()
            if not line:
                continue
            parts = [p.strip() for p in line.split('|')]
            if len(parts) != 4 or parts[0] in rows or not parts[3].isdigit():
                sys.exit(f'error: {REGISTRY}:{n}: malformed or repeated row')
            rows[parts[0]] = (parts[1], parts[2], int(parts[3]))
    return rows


def sources():
    out = set()
    for dp, _, fs in os.walk(SOURCES):
        for f in fs:
            if f.endswith('.sma'):
                out.add(os.path.relpath(os.path.join(dp, f), SOURCES)[:-4])
    return out


def row_of(arg):
    """The registry path of an argument, given as a row or as its source."""
    p = os.path.normpath(arg)
    if p.startswith(SOURCES + os.sep):
        p = p[len(SOURCES) + 1:]
    return p[:-4] if p.endswith('.sma') else p


def build(path):
    """(module bytes or None, compiler output)."""
    src = os.path.abspath(os.path.join(SOURCES, path + '.sma'))
    dst = os.path.abspath(os.path.join(OUT, path + '.amx'))
    cwd = os.path.dirname(dst)
    os.makedirs(cwd, exist_ok=True)
    if os.path.exists(dst):
        os.remove(dst)
    # The compiler writes its assembler file in the current directory (so build/, not src/),
    # and rebuilds its own path from PWD: it crashes without it.
    try:
        r = subprocess.run([os.path.abspath(SC)] + FLAGS + ['-i' + os.path.abspath(INCLUDE), src,
                            '-o' + dst], cwd=cwd,
                           env=dict(os.environ, PWD=cwd), capture_output=True, text=True,
                           errors='replace', timeout=TIMEOUT)
    except subprocess.TimeoutExpired:
        return None, f'(the compiler ran for more than {TIMEOUT} s)'
    # exit status: 0 done, 1 warnings only, 2 errors
    if r.returncode not in (0, 1) or not os.path.exists(dst):
        return None, r.stdout + r.stderr
    with open(dst, 'rb') as f:
        return f.read(), ''


def main():
    os.chdir(ROOT)
    opts = [a for a in sys.argv[1:] if a.startswith('-')]
    if any(o != '-v' for o in opts):
        sys.exit('Usage: python3 tools/scriptcheck.py [-v] [PATH ...]')
    verbose = bool(opts)
    args = [row_of(a) for a in sys.argv[1:] if not a.startswith('-')]
    if not os.path.exists(SC):
        sys.exit(f'error: {SC} missing: build it with make small')
    rows = registry()
    have = sources()
    unknown = [a for a in args if a not in rows]
    if unknown:
        sys.exit(f'error: not in {REGISTRY}: {" ".join(unknown)}')
    todo = args or sorted(rows)
    problems = [f'no source {SOURCES}/{p}.sma' for p in todo if p not in have]
    if not args:
        # (a full run also checks that every source has its row)
        problems += [f'not in {REGISTRY}: {SOURCES}/{p}.sma' for p in sorted(have - set(rows))]
    todo = [p for p in todo if p in have]
    ok = 0
    for path in todo:
        data, log = build(path)
        _, sha1, size = rows[path]
        if data is None:
            problems.append(f'does not compile: {SOURCES}/{path}.sma')
            print(log.rstrip())
            continue
        if len(data) != size or hashlib.sha1(data).hexdigest() != sha1:
            problems.append(f'differs: {path} ({len(data)} vs {size} bytes)')
            continue
        ok += 1
        if verbose:
            print(f'match      {path}')
    for p in problems:
        print(f'FAIL       {p}')
    print(f'scripts    {ok}/{len(todo)} match')
    sys.exit(1 if problems else 0)


if __name__ == '__main__':
    main()
