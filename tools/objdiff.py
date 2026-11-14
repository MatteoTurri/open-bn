#!/usr/bin/env python3
"""Diff of the gate objects against a snapshot: what `make check` does not see but the link does.

    python3 tools/objdiff.py snapshot [UNITS_FILE]   # copy build/check/<unit>/out.o into build/objdiff/snapshot/
    python3 tools/objdiff.py compare  [UNITS_FILE]   # after `make check` (full or UNITS=): compare with the snapshot

Never compiles: it reads the objects the gate produced (srcpath, dnas_subst, assets, same cwd).
Per unit it compares sections (set, order, type/flags/align/size/content), relocations, global
symbols (set, order, attributes) and local symbols, ignoring `.mdebug` and the numeric suffixes of
local symbols (`$L123`, `$LC4`, `buf.3`). Appearing/disappearing vtables (`_vt$`), typeinfo (`__tf`,
`__ti`) and `.gnu.linkonce.*` copies are named. One line per differing unit, exit 1 if any;
VERBOSE=1 prints the details. Pure Python ELF32 reader, runs on the host.
A unit whose object the last gate did not rebuild (failed compile: the gate leaves the old out.o) is
reported as stale. OBJDIFF_SNAP=<dir> uses another snapshot directory. `snapshot` without
UNITS_FILE starts a new snapshot; with it, only those units are replaced.
"""
import hashlib
import json
import os
import re
import shutil
import struct
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'relink'))
from paths import unit_obj  # noqa: E402  (same naming rule as tools/check.py)

SNAP = os.environ.get('OBJDIFF_SNAP', 'build/objdiff/snapshot')
MANIFEST = f'{SNAP}/manifest.json'
VERBOSE = os.environ.get('VERBOSE', '') not in ('', '0')
DETAIL_CAP = 12
UNIT_CAP = 20      # differing units printed without VERBOSE

SHT_SYMTAB, SHT_STRTAB, SHT_RELA, SHT_NOBITS, SHT_REL = 2, 3, 4, 8, 9
STT_SECTION, STT_FILE = 3, 4
STB_LOCAL = 0
SHN_UNDEF, SHN_ABS, SHN_COMMON = 0, 0xfff1, 0xfff2


def ignored_section(name):
    return name.startswith('.mdebug') or name.startswith('.rel.mdebug')


def read_units(path):
    units = []
    for raw in open(path):
        raw = raw.strip()
        if raw and not raw.startswith('#'):
            units.append(raw.split('|')[0].strip())
    return units


def snap_name(unit):
    return os.path.basename(os.path.dirname(unit_obj(unit))) + '.o'


def sha1(path):
    return hashlib.sha1(open(path, 'rb').read()).hexdigest()


def norm_local(name):
    """Drop the numeric suffix of a compiler/assembler-generated local name."""
    if re.match(r'[$.]', name):
        return re.sub(r'\d+$', '#', name)
    return re.sub(r'\.\d+$', '.#', name)


def parse(path):
    data = open(path, 'rb').read()
    if data[:4] != b'\x7fELF' or data[4] != 1 or data[5] != 1:
        raise ValueError(f'{path}: not an ELF32 little-endian object')
    shoff, = struct.unpack_from('<I', data, 0x20)
    shentsize, shnum, shstrndx = struct.unpack_from('<HHH', data, 0x2e)
    raw = [struct.unpack_from('<10I', data, shoff + i * shentsize) for i in range(shnum)]
    stroff = raw[shstrndx][4]
    e_flags, = struct.unpack_from('<I', data, 0x24)

    def cstr(off):
        return data[off:data.index(b'\0', off)].decode('latin-1')

    secs = []
    for name, typ, flags, _addr, off, size, link, info, align, entsize in raw:
        body = None if typ == SHT_NOBITS else data[off:off + size]
        secs.append(dict(name=cstr(stroff + name), type=typ, flags=flags, size=size, link=link,
                         info=info, align=align, entsize=entsize, body=body))

    # symbols
    syms, symkeys = [], []
    symtab = next((s for s in secs if s['type'] == SHT_SYMTAB), None)
    if symtab:
        sstr = raw[symtab['link']][4]
        for i in range(symtab['size'] // 16):
            st_name, value, size, info, other, shndx = struct.unpack_from('<IIIBBH', symtab['body'], i * 16)
            bind, typ = info >> 4, info & 15
            if shndx == SHN_UNDEF:
                sec = 'UND'
            elif shndx == SHN_ABS:
                sec = 'ABS'
            elif shndx == SHN_COMMON:
                sec = 'COM'
            elif shndx < len(secs):
                sec = secs[shndx]['name']
            else:
                sec = f'#{shndx}'
            name = cstr(sstr + st_name)
            if typ == STT_SECTION:
                key = 'sec:' + sec
            elif bind == STB_LOCAL:
                key = norm_local(name)
            else:
                key = name
            symkeys.append(key)
            syms.append(dict(name=name, key=key, bind=bind, type=typ, sec=sec, value=value, size=size,
                             other=other))

    names = [cstr(stroff + r[0]) for r in raw]
    dup = {n for n in names if n and names.count(n) > 1}
    if dup:
        raise ValueError(f'{path}: duplicate section names {sorted(dup)}')
    order, sections, relocs = [], {}, {}
    for s in secs:
        n = s['name']
        if not n or ignored_section(n) or s['type'] in (SHT_SYMTAB, SHT_STRTAB):
            continue
        if s['type'] in (SHT_REL, SHT_RELA):
            target = secs[s['info']]['name'] if s['info'] < len(secs) else '?'
            if ignored_section(target):
                continue
            ent = 8 if s['type'] == SHT_REL else 12
            rows = []
            for i in range(s['size'] // ent):
                off, rinfo = struct.unpack_from('<II', s['body'], i * ent)
                add = struct.unpack_from('<i', s['body'], i * ent + 8)[0] if ent == 12 else 0
                si = rinfo >> 8
                rows.append((off, rinfo & 0xff, symkeys[si] if si < len(symkeys) else f'#{si}', add))
            order.append(n)
            relocs[n] = rows
            continue
        order.append(n)
        sections[n] = s

    globs = [s for s in syms if s['bind'] != STB_LOCAL and not ignored_section(s['sec'])]
    locs = [s for s in syms if s['bind'] == STB_LOCAL and s['type'] not in (STT_SECTION, STT_FILE)
            and not ignored_section(s['sec'])]
    return dict(order=order, sections=sections, relocs=relocs, globals=globs, locals=locs, e_flags=e_flags)


def kind(name, sec):
    if '_vt$' in name or name.startswith('__vt_'):
        return 'vtable'
    if name.startswith(('__tf', '__ti')):
        return 'typeinfo'
    if sec.startswith('.gnu.linkonce'):
        return 'linkonce'
    return 'sym'


def diff_list(a, b):
    """+/- of two name lists, summarised by kind; returns (summary items, detail lines)."""
    sa, sb = set(a), set(b)
    add, rem = [x for x in b if x not in sa], [x for x in a if x not in sb]
    return add, rem


def compare(old, new):
    """-> (list of short items, list of detail lines). Empty = identical."""
    items, det = [], []
    if old['e_flags'] != new['e_flags']:
        items.append('e_flags')
        det.append(f'  e_flags 0x{old["e_flags"]:x} -> 0x{new["e_flags"]:x}')

    # sections: set and order
    so, sn = [n for n in old['order'] if not n.startswith('.rel')], [n for n in new['order'] if not n.startswith('.rel')]
    add, rem = diff_list(so, sn)
    lk_add = [n for n in add if n.startswith('.gnu.linkonce')]
    lk_rem = [n for n in rem if n.startswith('.gnu.linkonce')]
    if lk_add or lk_rem:
        items.append(f'linkonce +{len(lk_add)}/-{len(lk_rem)}')
    other_add = [n for n in add if n not in lk_add]
    other_rem = [n for n in rem if n not in lk_rem]
    if other_add or other_rem:
        items.append('sections ' + ' '.join(['+' + n for n in other_add] + ['-' + n for n in other_rem]))
    det += [f'  section +{n}' for n in add] + [f'  section -{n}' for n in rem]
    common_o = [n for n in so if n in set(sn)]
    common_n = [n for n in sn if n in set(so)]
    if common_o != common_n:
        lk = lambda l: [n for n in l if n.startswith('.gnu.linkonce')]  # noqa: E731
        nl = lambda l: [n for n in l if not n.startswith('.gnu.linkonce')]  # noqa: E731
        items.append('linkonce order' if nl(common_o) == nl(common_n) and lk(common_o) != lk(common_n)
                     else 'section order')
        i = next(i for i in range(len(common_o)) if common_o[i] != common_n[i])
        det.append(f'  section order: first difference at #{i}: {common_o[i]} -> {common_n[i]}')

    # section headers and contents
    changed, hdr = [], []
    for n in common_n:
        a, b = old['sections'][n], new['sections'][n]
        what = []
        if a['size'] != b['size']:
            what.append(f'size {a["size"]}->{b["size"]}')
        elif a['body'] != b['body']:
            first = next(i for i in range(len(a['body'])) if a['body'][i] != b['body'][i])
            what.append(f'content from +0x{first:x}')
        if what:
            changed.append(n)
        h = [f'{f} {a[f]}->{b[f]}' for f in ('type', 'flags', 'align', 'entsize') if a[f] != b[f]]
        if h:
            hdr.append(n)
        if what or h:
            det.append(f'  {n}: ' + ', '.join(what + h))
    if changed:
        items.append('content ' + ','.join(changed[:6]) + (' ...' if len(changed) > 6 else ''))
    if hdr:
        items.append('section attrs ' + ','.join(hdr[:6]) + (' ...' if len(hdr) > 6 else ''))

    # relocations: sections that appear or vanish (a pointer 0 -> &sym in .data changes only this)
    both = set(old['sections']) & set(new['sections'])     # relocs of added/removed sections: told above
    radd = [n for n in new['relocs'] if n not in old['relocs'] and n[4:] in both]
    rrem = [n for n in old['relocs'] if n not in new['relocs'] and n[4:] in both]
    if radd or rrem:
        items.append('reloc sections ' + ' '.join(['+' + n for n in radd] + ['-' + n for n in rrem]))
        det += [f'  {n}: +{len(new["relocs"][n])} entries' for n in radd]
        det += [f'  {n}: -{len(old["relocs"][n])} entries' for n in rrem]
    rch = [n for n in old['relocs'] if n in new['relocs'] and old['relocs'][n] != new['relocs'][n]]
    if rch:
        items.append('relocs ' + ','.join(rch[:4]) + (' ...' if len(rch) > 4 else ''))
        for n in rch:
            a, b = old['relocs'][n], new['relocs'][n]
            i = next((i for i in range(min(len(a), len(b))) if a[i] != b[i]), min(len(a), len(b)))
            det.append(f'  {n}: {len(a)}->{len(b)} entries, first difference at #{i}: '
                       f'{a[i] if i < len(a) else "-"} -> {b[i] if i < len(b) else "-"}')

    # global symbols: set (by kind), order, attributes
    go = [s['name'] for s in old['globals']]
    gn = [s['name'] for s in new['globals']]
    add, rem = diff_list(go, gn)
    secof = {s['name']: s['sec'] for s in old['globals'] + new['globals']}
    if add or rem:
        by = {}
        for sign, lst in (('+', add), ('-', rem)):
            for n in lst:
                k = kind(n, secof[n])
                by[(sign, k)] = by.get((sign, k), 0) + 1
        items.append('globals ' + ' '.join(f'{s}{k} {c}' for (s, k), c in sorted(by.items(), key=lambda x: (x[0][1], x[0][0]))))
        det += [f'  global +{n} [{secof[n]}]' for n in add] + [f'  global -{n} [{secof[n]}]' for n in rem]
    cg_o = [n for n in go if n in set(gn)]
    cg_n = [n for n in gn if n in set(go)]
    if cg_o != cg_n:
        items.append('global order')
        i = next(i for i in range(len(cg_o)) if cg_o[i] != cg_n[i])
        det.append(f'  global order: first difference at #{i}: {cg_o[i]} -> {cg_n[i]}')
    oa = {s['name']: s for s in old['globals']}
    attr = []
    for s in new['globals']:
        a = oa.get(s['name'])
        if a:
            d = [f'{f} {a[f]}->{s[f]}' for f in ('sec', 'value', 'size', 'bind', 'type', 'other') if a[f] != s[f]]
            if d:
                attr.append(s['name'])
                det.append(f'  global {s["name"]}: ' + ', '.join(d))
    if attr:
        items.append(f'global attrs {len(attr)}')

    # local symbols (numeric suffixes dropped)
    lt = lambda s: (s['key'], s['sec'], s['value'], s['size'], s['type'])  # noqa: E731
    lo, ln = [lt(s) for s in old['locals']], [lt(s) for s in new['locals']]
    if lo != ln:
        ko, kn = [x[0] for x in lo], [x[0] for x in ln]
        add, rem = diff_list(ko, kn)
        lsec = {s['key']: s['sec'] for s in old['locals'] + new['locals']}
        named = [n for n in add + rem if kind(n, lsec[n]) != 'sym']
        if add or rem:
            items.append(f'locals +{len(add)}/-{len(rem)}' + (f' ({",".join(named[:3])})' if named else ''))
            det += [f'  local +{n} [{lsec[n]}]' for n in add] + [f'  local -{n} [{lsec[n]}]' for n in rem]
        else:
            items.append('locals moved' if len(lo) == len(ln) else f'locals {len(lo)}->{len(ln)}')
            i = next((i for i in range(min(len(lo), len(ln))) if lo[i] != ln[i]), 0)
            det.append(f'  local first difference at #{i}: {lo[i] if i < len(lo) else "-"} -> {ln[i] if i < len(ln) else "-"}')
    return items, det


def units_of(argv):
    return read_units(argv[0] if argv else os.path.join(ROOT, 'config/units.txt'))


def git_head():
    try:
        r = subprocess.run(['git', 'rev-parse', '--short', 'HEAD'], capture_output=True, text=True)
        d = subprocess.run(['git', 'status', '--porcelain', '--', 'src', 'ref', 'config'],
                           capture_output=True, text=True)
        return r.stdout.strip() + ('-dirty' if d.stdout.strip() else '')
    except OSError:
        return '?'


def cmd_snapshot(argv):
    """Without UNITS_FILE the snapshot is rebuilt from scratch; with it, those units are replaced."""
    units = units_of(argv)
    man = {}
    if argv:
        try:
            man = json.load(open(MANIFEST))
        except (OSError, ValueError):
            man = {}
    elif os.path.isdir(SNAP):
        shutil.rmtree(SNAP)
    os.makedirs(SNAP, exist_ok=True)
    entries = man.setdefault('units', {})
    head = git_head()
    missing = []
    for u in units:
        src = unit_obj(u)
        if not os.path.exists(src):
            missing.append(u)
            continue
        dst = f'{SNAP}/{snap_name(u)}'
        shutil.copyfile(src, dst)
        entries[u] = {'object': snap_name(u), 'sha1': sha1(dst), 'mtime_ns': os.stat(src).st_mtime_ns,
                      'commit': head}
    man.pop('commit', None)
    with open(MANIFEST, 'w') as f:
        json.dump(man, f, indent=1, sort_keys=True)
    for u in missing[:UNIT_CAP]:
        print(f'objdiff    warning: {u}: no object in build/check (run make check first)')
    if len(missing) > UNIT_CAP:
        print(f'objdiff    ... and {len(missing) - UNIT_CAP} more without an object')
    print(f'objdiff    snapshot of {len(units) - len(missing)} units at {head} in {SNAP}')
    return 1 if missing else 0


def cmd_compare(argv):
    units = units_of(argv)
    try:
        entries = json.load(open(MANIFEST))['units']
    except (OSError, ValueError, KeyError):
        sys.exit(f'objdiff    error: no snapshot ({MANIFEST}); run `python3 tools/objdiff.py snapshot` first')
    lines, bad, same, stale = [], 0, 0, []
    for u in units:
        cur = unit_obj(u)
        e = entries.get(u)
        if e is None:
            lines.append([f'{u}  not in snapshot'])
            bad += 1
            continue
        if not os.path.exists(cur):
            lines.append([f'{u}  no object in build/check'])
            bad += 1
            continue
        asm = os.path.join(os.path.dirname(cur), 'out.s')
        if not os.path.exists(asm) or os.stat(asm).st_mtime_ns > os.stat(cur).st_mtime_ns:
            # the gate does not delete out.o: a failed compile/DNAS/as step leaves the old object
            lines.append([f'{u}  stale object (the last make check did not build it)'])
            bad += 1
            continue
        if os.stat(cur).st_mtime_ns == e.get('mtime_ns'):
            stale.append(u)
        old = f'{SNAP}/{e["object"]}'
        if sha1(cur) == e['sha1']:
            same += 1
            continue
        try:
            items, det = compare(parse(old), parse(cur))
        except (ValueError, IndexError, struct.error) as ex:
            lines.append([f'{u}  unreadable object: {ex}'])
            bad += 1
            continue
        if not items:
            same += 1     # differs only in ignored parts (.mdebug, local label numbers)
            continue
        bad += 1
        out = [f'{u}  ' + '; '.join(items)]
        if VERBOSE:
            out += det[:DETAIL_CAP]
            if len(det) > DETAIL_CAP:
                out.append(f'  ... and {len(det) - DETAIL_CAP} more')
        lines.append(out)
    for out in (lines if VERBOSE else lines[:UNIT_CAP]):
        print('\n'.join(out))
    if len(lines) > UNIT_CAP and not VERBOSE:
        print(f'objdiff    ... and {len(lines) - UNIT_CAP} more (VERBOSE=1 for all)')
    if stale:
        print(f'objdiff    warning: {len(stale)} units not rebuilt since the snapshot, compared as they were'
              + (f': {", ".join(stale[:3])}{" ..." if len(stale) > 3 else ""}' if VERBOSE or len(stale) <= 3 else ''))
    if bad:
        print(f'objdiff    {bad} of {len(units)} units differ from the snapshot')
        return 1
    print(f'objdiff    {same} units identical to the snapshot')
    return 0


def main():
    sys.argv[2:] = [os.path.abspath(a) for a in sys.argv[2:]]
    os.chdir(ROOT)
    if len(sys.argv) < 2 or sys.argv[1] not in ('snapshot', 'compare') or len(sys.argv) > 3:
        sys.exit('usage: python3 tools/objdiff.py snapshot|compare [UNITS_FILE]')
    sys.exit((cmd_snapshot if sys.argv[1] == 'snapshot' else cmd_compare)(sys.argv[2:]))


if __name__ == '__main__':
    main()
