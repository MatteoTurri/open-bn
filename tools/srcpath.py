#!/usr/bin/env python3
"""Compile a unit with the original path (config/unit_paths.txt).

gcc 2.95 puts the command-line argument as is into `__FILE__`, and the
literals of the `assert`s (and of ERR_put_error in SSL-C) end up in .rodata:
with a different path the unit's data block changes length and shifts
(docs/gate.md).

To reproduce it the unit is compiled from the right cwd with the original path:
  ./src/func.c                          cwd = ref/sqlite-2.7.2
  ./crypto/asn1/../../crypto/asn1/x.c   cwd = ref/sslc-2.1.0
  ../libtiff/tif_fax3.c, objSimpleTextEdit.cpp, ./digest/hmac/../../digest/hmac/hmac.c
                                        virtual tree in build/vpath/<k>: real directories for
                                        every component, relative links to the file and its
                                        siblings (works in the container too)
The rest of the command line (toolchain, -I, -B, output) becomes absolute.

As a module: command(src, cc_cmd_without_source) -> shell line to run; for the
units not in the table it changes nothing.
"""
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TABLE = os.path.join(ROOT, 'config', 'unit_paths.txt')
_MAP = None


def table():
    # the table is built locally and published whole: tools/check.py compiles in
    # parallel, and a thread seeing `_MAP` already assigned but half filled would
    # compile without the remapping
    global _MAP
    if _MAP is None:
        m = {}
        for line in open(TABLE):
            line = line.split('#', 1)[0].strip()
            if '|' in line:
                u, p = (x.strip() for x in line.split('|', 1))
                m[u] = p
        _MAP = m
    return _MAP


def _abs(cmd):
    """-I/-B/-o and the first token (the compiler) relative -> absolute against ROOT."""
    out = []
    for i, t in enumerate(cmd.split()):
        m = re.match(r'^(-I|-B|-include)(.+)$', t)
        if m and not m.group(2).startswith('/'):
            t = m.group(1) + os.path.normpath(os.path.join(ROOT, m.group(2))) + ('/' if m.group(2).endswith('/') else '')
        elif i == 0 and not t.startswith('/') and '/' in t:
            t = os.path.join(ROOT, t)
        elif out and out[-1] == '-o' and not t.startswith('/'):
            t = os.path.join(ROOT, t)
        out.append(t)
    return ' '.join(out)


def prepare(src):
    """-> (absolute cwd, source argument, extra -I) or None if the unit is not in the table."""
    p = table().get(src)
    if p is None:
        return None
    norm = os.path.normpath(p)
    srcdir = os.path.dirname(os.path.join(ROOT, src))
    if not norm.startswith('..') and src.endswith('/' + norm):
        # the original path is a suffix of ours: cwd = the root before it
        return os.path.join(ROOT, src[:-len(norm)]), p, ''
    # Otherwise a virtual tree in build/vpath/<key>: real directories for every
    # component of the original path and a link to the file only (the kernel resolves
    # `..` physically: a link to a directory would break `./digest/hmac/../../digest/...`).
    key = re.sub(r'[^A-Za-z0-9_]', '_', src)
    vroot = os.path.join(ROOT, 'build', 'vpath', key)
    parts = [x for x in p.split('/') if x not in ('', '.')]
    depth, lo = 0, 0
    for x in parts[:-1]:
        depth += -1 if x == '..' else 1
        lo = min(lo, depth)
    cwd = os.path.join(vroot, *(['w'] * -lo)) if lo else vroot
    os.makedirs(cwd, exist_ok=True)
    cur = cwd
    for x in parts[:-1]:
        cur = os.path.normpath(os.path.join(cur, x))
        os.makedirs(cur, exist_ok=True)
    # in the virtual folder: the file with the original name and all the files of the
    # real folder, so quoted includes find the same sibling headers with the same
    # precedence (an extra -I would change the order: incflat's hmac.h instead of the
    # one next to the source)
    links = {parts[-1]: os.path.join(ROOT, src)}
    for f in os.listdir(srcdir):
        if f not in links and os.path.isfile(os.path.join(srcdir, f)):
            links[f] = os.path.join(srcdir, f)
    for name, target in links.items():
        link = os.path.join(cur, name)
        rel = os.path.relpath(target, cur)
        if not os.path.islink(link) or os.readlink(link) != rel:
            if os.path.lexists(link):
                os.unlink(link)
            os.symlink(rel, link)
    return cwd, p, ''


TIMES = os.path.join(ROOT, 'config', 'unit_times.txt')
_TIMES = None


def times():
    """unit -> UTC epoch of the original build time (config/unit_times.txt)"""
    global _TIMES
    if _TIMES is None:
        import calendar
        import time as _t
        m = {}
        if os.path.exists(TIMES):
            for line in open(TIMES):
                line = line.split('#', 1)[0].strip()
                if '|' in line:
                    u, d = (x.strip() for x in line.split('|', 1))
                    m[u] = calendar.timegm(_t.strptime(' '.join(d.split()), '%b %d %Y %H:%M:%S'))
        _TIMES = m
    return _TIMES


def command(src, cc_cmd):
    """cc_cmd = the compiler line without the source at the end. For the units of
    config/unit_times.txt the compiler runs under tools/fixtime.py (original build
    time for __DATE__/__TIME__)."""
    prep = prepare(src)
    ep = times().get(src)
    if prep is None:
        cwd, arg, extra = None, src, ''
    else:
        cwd, arg, extra = prep
    cmd = _abs(cc_cmd) if cwd else cc_cmd
    if ep is not None:
        cmd = f'python3 {os.path.join(ROOT, "tools", "fixtime.py")} {ep} -- {cmd}'
    if cwd:
        return f'cd {cwd} && {cmd} {extra} {arg}'
    return f'{cmd} {arg}'


if __name__ == '__main__':
    # tools/match.sh: python3 tools/srcpath.py SRC "compiler line without the source"
    import sys
    print(command(sys.argv[1], sys.argv[2]))
