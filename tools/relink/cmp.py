#!/usr/bin/env python3
"""Section-by-section comparison between the original and a linked ELF.

Usage: python3 tools/relink/cmp.py [ORIG] LINKED [--funcs N] [--json OUT] [--mask-gp]
       (`make link-cmp` = cmp.py on build/relink/bn.elf + SHA1 against the original)

For each section of the original (by name): address and size of both sides, equal bytes
(over the common part, at the same offset inside the section), first differing byte (offset
and address). For the code sections (.text, .init, .fini) it reports which original function
holds the first difference (config/symbol_addrs.txt + config/static_funcs.txt), how many
functions have at least one differing byte, and how many differing bytes fall inside the
encrypted DNAS regions (config/dnas{1,2}_regions.txt): encrypted in the original, plain in our
.o files, so a difference there is expected unless the image was re-encrypted
(tools/relink/dnas_protect.py).
NOBITS sections (.sbss, .bss) are compared by address and size only.
Sections present only in the linked file are listed at the end.
--mask-gp: in the code sections, compare the $gp-based instructions (lw/sw/addiu/
lwc1/swc1 rt,off(gp)) without the offset: separates differences caused by the GOT order
(a cascade, as long as .dynsym/.got are not identical) from real ones.
Exit 0 if every section is identical (address, size, bytes), 1 otherwise, 2 if the comparison
breaks (exception: missing file, unreadable ELF), distinct from the verdict (`make link-cmp`).
Output: the full report on stdout; if something differs, one line per differing section
(at most 20) also on stderr, which `make link-cmp` shows while the report goes to
build/relink/cmp.txt.
"""
USAGE = 'usage: python3 tools/relink/cmp.py [ORIG] LINKED [--funcs N] [--json OUT] [--mask-gp]'
import bisect, json, re, sys
from elftools.elf.elffile import ELFFile

CODE = {'.text', '.init', '.fini'}


def load_funcs():
    rows = []
    for f in ('config/symbol_addrs.txt', 'config/static_funcs.txt'):
        for l in open(f):
            m = re.match(r'(\S+) = 0x([0-9A-Fa-f]+);.*type:func size:0x([0-9A-Fa-f]+)', l)
            if m and int(m.group(3), 16):
                rows.append((int(m.group(2), 16), int(m.group(3), 16), m.group(1)))
    rows.sort()
    return rows


def load_dnas():
    regs = []
    for f in ('config/dnas2_regions.txt', 'config/dnas1_regions.txt'):
        for l in open(f):
            m = re.match(r'^(0x[0-9a-f]+) (0x[0-9a-f]+)\s', l)
            if m:
                regs.append((int(m.group(1), 16), int(m.group(2), 16)))
    return sorted(regs)


GP_OPS = {0x23, 0x2b, 0x09, 0x31, 0x39, 0x37, 0x3f, 0x20, 0x24, 0x21, 0x25, 0x28, 0x29}


def mask_gp(d):
    out = bytearray(d)
    for i in range(0, len(d) - 3, 4):
        w = int.from_bytes(d[i:i + 4], 'little')
        if (w >> 26) in GP_OPS and ((w >> 21) & 31) == 28:
            out[i] = out[i + 1] = 0
    return bytes(out)


def func_at(funcs, a):
    i = bisect.bisect_right(funcs, (a, 1 << 40, '')) - 1
    if i >= 0 and funcs[i][0] <= a < funcs[i][0] + funcs[i][1]:
        return funcs[i]
    return None


def sections(path):
    e = ELFFile(open(path, 'rb'))
    out = {}
    for s in e.iter_sections():
        if s.name:
            out[s.name] = dict(addr=s['sh_addr'], size=s['sh_size'], type=s['sh_type'],
                               data=None if s['sh_type'] == 'SHT_NOBITS' else s.data())
    return e, out


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    opt = {a.split('=')[0]: (a.split('=', 1)[1] if '=' in a else True) for a in sys.argv[1:] if a.startswith('--')}
    if len(args) == 1:
        args = ['orig/bn-jpn.elf'] + args
    if len(args) != 2:
        print(USAGE, file=sys.stderr)
        sys.exit(2)
    po, pl = args
    nf = int(opt.get('--funcs', 10))
    eo, so = sections(po)
    el, sl = sections(pl)
    funcs = load_funcs()
    dnas = load_dnas()
    rows, extra, detail = [], [], []
    allsame = True
    for name, o in so.items():
        l = sl.get(name)
        r = dict(sec=name, oaddr=o['addr'], osize=o['size'])
        if l is None:
            r.update(state='MISSING')
            rows.append(r); allsame = False
            continue
        r.update(laddr=l['addr'], lsize=l['size'])
        if o['data'] is None or l['data'] is None:
            same = o['addr'] == l['addr'] and o['size'] == l['size'] and o['type'] == l['type']
            r.update(state='ok' if same else 'different', eq=None)
            allsame &= same
            rows.append(r)
            continue
        a, b = o['data'], l['data']
        if name in CODE and opt.get('--mask-gp'):
            a, b = mask_gp(a), mask_gp(b)
        n = min(len(a), len(b))
        diffs = [i for i in range(n) if a[i] != b[i]] if a[:n] != b[:n] else []
        r['eq'] = n - len(diffs)
        r['first'] = diffs[0] if diffs else (n if len(a) != len(b) else None)
        same = not diffs and len(a) == len(b) and o['addr'] == l['addr']
        r['state'] = 'ok' if same else 'different'
        allsame &= same
        if r['first'] is not None and o['addr']:
            fa = o['addr'] + r['first']
            r['first_addr'] = fa
            if name in CODE:
                f = func_at(funcs, fa)
                r['first_func'] = f'{f[2]}+0x{fa - f[0]:x}' if f else '(outside the functions of the universe)'
        if name in CODE and diffs:
            # functions with differences; differing bytes inside the encrypted DNAS regions
            fd = {}
            in_dnas = 0
            j = 0
            for i in diffs:
                ad = o['addr'] + i
                while j < len(dnas) and dnas[j][1] <= ad:
                    j += 1
                if j < len(dnas) and dnas[j][0] <= ad < dnas[j][1]:
                    in_dnas += 1
                f = func_at(funcs, ad)
                k = f[2] if f else '(outside)'
                fd[k] = fd.get(k, 0) + 1
            r['diff_bytes'] = len(diffs)
            r['diff_dnas'] = in_dnas
            r['diff_funcs'] = len(fd)
            detail.append((name, sorted(fd.items(), key=lambda x: -x[1])[:nf]))
        elif diffs:
            r['diff_bytes'] = len(diffs)
        rows.append(r)
    for name, l in sl.items():
        if name not in so:
            extra.append((name, l['addr'], l['size']))

    brief = []

    def h(x):
        return '-' if x is None else f'0x{x:08x}'
    print(f'orig:    {po}\nlinked:  {pl}\n')
    print(f'{"section":20s} {"orig addr":>10s} {"link addr":>10s} {"orig size":>9s} {"link size":>9s} '
          f'{"equal":>9s} {"%":>6s}  first difference')
    for r in rows:
        if r['state'] == 'MISSING':
            print(f'{r["sec"]:20s} {h(r["oaddr"]):>10s} {"MISSING":>10s} {r["osize"]:9d}')
            continue
        eq = r.get('eq')
        pct = '' if eq is None or not r['osize'] else f'{100.0 * eq / r["osize"]:6.2f}'
        fd = ''
        if r.get('first') is not None:
            fd = f'+0x{r["first"]:x}'
            if r.get('first_addr'):
                fd += f' (0x{r["first_addr"]:08x})'
            if r.get('first_func'):
                fd += ' ' + r['first_func']
        if r.get('diff_bytes'):
            fd += f'  [{r["diff_bytes"]} B differ'
            if 'diff_funcs' in r:
                fd += f', {r["diff_funcs"]} functions, {r["diff_dnas"]} B in encrypted DNAS regions'
            fd += ']'
        mark = '' if r['state'] == 'ok' else ' *'
        print(f'{r["sec"]:20s} {h(r["oaddr"]):>10s} {h(r.get("laddr")):>10s} {r["osize"]:9d} '
              f'{r.get("lsize", 0):9d} {"" if eq is None else eq:>9} {pct:>6s}  {fd}{mark}')
        if r['state'] != 'ok':
            brief.append(f'{r["sec"]}: differs (orig {h(r["oaddr"])} +{r["osize"]}, '
                         f'linked {h(r.get("laddr"))} +{r.get("lsize", 0)}) {fd}'.rstrip())
    if extra:
        print('\nonly in the linked file:')
        for n, a, s in extra:
            print(f'  {n:20s} 0x{a:08x} {s}')
            brief.append(f'{n}: only in the linked file (0x{a:08x} +{s})')
    for name, fl in detail:
        print(f'\n{name}: functions with the most differing bytes (first {nf})')
        for k, v in fl:
            print(f'  {v:7d}  {k}')
    if opt.get('--json'):
        json.dump(dict(rows=[{k: v for k, v in r.items()} for r in rows], extra=extra),
                  open(opt['--json'], 'w'), indent=1)
    print('\nRESULT:', 'identical' if allsame else 'different')
    for b in brief[:20]:
        print(b, file=sys.stderr)
    if len(brief) > 20:
        print(f'... and {len(brief) - 20} more, see the full report', file=sys.stderr)
    sys.exit(0 if allsame else 1)


if __name__ == '__main__':
    try:
        main()
    except SystemExit:
        raise
    except BaseException:
        import traceback
        traceback.print_exc()
        sys.exit(2)
