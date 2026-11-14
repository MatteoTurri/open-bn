#!/usr/bin/env python3
"""Non-compiled bytes for the link: blobs extracted from orig/bn-jpn.elf at build time.

Usage: python3 tools/relink/make_blobs.py   (repository root, in the open-bn-env container: it
       needs pyelftools and the Sony gas is a Linux i386 binary; `make link` calls it via `make link-in`)

Reads build/relink/order.json (written by link_order.py). For each blob:
  - mode 'object': build/relink/blobs/blob_<sec>_<addr>.{bin,s,o}, a .o with only
    the section and the original bytes (its line is already in link_order.txt). gas 2.9EE has
    no .incbin: the bytes go into a generated .inc (.word/.byte) pulled in with .include;
  - mode 'head'/'tail': the bytes share a section with a unit (the start or the
    end is not 16-aligned, while every gas 2.9EE .o starts at 16): the unit's .o is rebuilt
    from its gate out.s (already through dnas_subst) with the .include at the head (right
    after the first directive of that section) or at the tail, in
    build/relink/merged/<unit>.{s,o}.
The .s files are assembled with the .o's directory as cwd and relative .include paths: the
paths seen by as end up in the .mdebug of the link (bn.unstripped.elf; the cpt.offset of bn.elf
is copied from the original), and must be fixed, independent of the checkout directory.
The bytes never enter the repository: build/ is gitignored and the .bin files are rebuilt from
the user's ELF. The original's SHA1 (config/bn.yaml) is checked before extracting.
"""
import hashlib, json, os, re, subprocess, sys
from elftools.elf.elffile import ELFFile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from paths import OUT as HERE, ORIG, AS, BLOBS, MERGED  # noqa: E402
SECDIR = {'.data': '.data', '.rodata': '.rdata', '.text': '.text'}  # gcc 2.95 directives


def vread(elf, raw, a, n):
    for seg in elf.iter_segments():
        if seg['p_type'] == 'PT_LOAD' and seg['p_vaddr'] <= a and a + n <= seg['p_vaddr'] + seg['p_filesz']:
            off = seg['p_offset'] + a - seg['p_vaddr']
            return raw[off:off + n]
    sys.exit(f'error: 0x{a:x}+{n} outside the loaded segments')


def write_inc(path, data, labels=()):
    """The bytes as gas directives (little-endian .word in groups of 8, .byte where the
    offset is not 4-aligned), with the labels of the original's .dynsym that fall inside
    the blob (labels = [(offset, name, type, size, bind)], offset == len(data) for an end
    label). Generated file in build/, never tracked."""
    L = []
    cuts = sorted({0, len(data)} | {o for o, *_ in labels})
    lab = {}
    for o, n, t, sz, b in labels:
        lab.setdefault(o, []).append((n, t, sz, b))
    for k, a in enumerate(cuts):
        for n, t, sz, b in lab.get(a, []):
            L.append(f'\t.globl\t{n}' if b == 'STB_GLOBAL' else f'\t.weak\t{n}')
            L.append(f'\t.type\t{n},@{"function" if t == "STT_FUNC" else "object"}')
            L.append(f'\t.size\t{n},{sz}')
            L.append(f'{n}:')
        if k + 1 == len(cuts):
            break
        e = cuts[k + 1]
        i = a
        while i < e and i % 4:
            L.append(f'\t.byte\t{data[i]}'); i += 1
        while i + 4 <= e:
            j = min(e - (e - i) % 4, i + 32)
            w = [int.from_bytes(data[x:x + 4], 'little') for x in range(i, j, 4)]
            L.append('\t.word\t' + ','.join(f'0x{x:08x}' for x in w))
            i = j
        if i < e:
            L.append('\t.byte\t' + ','.join(str(x) for x in data[i:e]))
    open(path, 'w').write('\n'.join(L) + '\n')


def blob_labels(elf, blobs):
    """{blob index: [(offset, name, type, size, bind)]} from the original's .dynsym.
    A size-0 symbol at the end of an 'object' blob (ChloeUsotsuki, ChloeFork) is the
    end label of that blob, not the start of the next one."""
    out = {}
    ds = elf.get_section_by_name('.dynsym')
    for s in ds.iter_symbols():
        if not s.name or s['st_shndx'] == 'SHN_UNDEF':
            continue
        a = s['st_value']
        row = (s.name, s['st_info']['type'], s['st_size'], s['st_info']['bind'])
        k = next((i for i, b in enumerate(blobs) if b['end'] == a and not s['st_size']
                  and b['mode'] == 'object'), None)
        if k is None:
            k = next((i for i, b in enumerate(blobs) if b['start'] <= a < b['end']), None)
        if k is not None:
            out.setdefault(k, []).append((a - blobs[k]['start'],) + row)
    return out


def unit_flags():
    out = {}
    for l in open('config/units.txt'):
        if l.strip() and not l.startswith('#'):
            f = [x.strip() for x in l.split('|')]
            out[f[0]] = f[2] if len(f) > 2 else ''
    return out


def sh(cmd):
    r = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    if r.returncode:
        sys.exit(f'error: {cmd}\n{r.stderr}')


def main():
    want = re.search(r'sha1: ([0-9a-f]{40})', open('config/bn.yaml').read()).group(1)
    raw = open(ORIG, 'rb').read()
    if hashlib.sha1(raw).hexdigest() != want:
        sys.exit('error: SHA1 of orig/bn-jpn.elf differs from config/bn.yaml')
    elf = ELFFile(open(ORIG, 'rb'))
    o = json.load(open(f'{HERE}/order.json'))
    flags = unit_flags()
    os.makedirs(BLOBS, exist_ok=True)
    os.makedirs(MERGED, exist_ok=True)
    merges = {}
    labels = blob_labels(elf, o['blobs'])
    for k, b in enumerate(o['blobs']):
        n = b['end'] - b['start']
        data = vread(elf, raw, b['start'], n)
        stem = b['file'][:-2]
        open(stem + '.bin', 'wb').write(data)
        lab = labels.get(k, ())
        if b['mode'] != 'object':
            # the unit hosting the blob may already define those names (sound.c: bbsebd,
            # bbsehdo as local .comm = data debt, in the original they are the blob's data):
            # they are not redefined, and the report says so
            us = open('build/check/' + re.sub(r'[^A-Za-z0-9_]', '_', b['unit']) + '/out.s').read()
            dup = [x for x in lab if re.search(r'(^|\n)(%s:|\s*\.(l?comm)\s+%s,)' % (re.escape(x[1]), re.escape(x[1])), us)]
            for x in dup:
                print(f'   note: {x[1]} already defined by {b["unit"]}: blob label omitted')
            lab = [x for x in lab if x not in dup]
        write_inc(stem + '.inc', data, lab)
        if b['mode'] == 'object':
            sec = b['sec']
            al = 2 if sec == '.eh_frame' else 4
            hdr = {'.text': '\t.text', '.data': '\t.data', '.rodata': '\t.rdata',
                   '.eh_frame': '\t.section\t.eh_frame,"aw",@progbits'}[sec]
            open(stem + '.s', 'w').write(
                f'# blob {b["cls"]} [{b["name"]}] 0x{b["start"]:08x}..0x{b["end"]:08x} (bytes from the user\'s ELF)\n'
                f'\t.abicalls\n{hdr}\n\t.align\t{al}\n\t.include\t"{os.path.basename(stem)}.inc"\n')
            sh(f'cd {BLOBS} && {os.path.abspath(AS)} -mips2 -m5900 -o {os.path.basename(b["file"])} '
               f'{os.path.basename(stem)}.s')
        else:
            merges.setdefault(b['unit'], []).append(b)
    for u, bl in merges.items():
        base = re.sub(r'[^A-Za-z0-9_]', '_', u)
        lines = open(f'build/check/{base}/out.s').read().split('\n')
        for b in bl:
            # path relative to the merged directory (cwd of as, below): the paths seen by as end
            # up in the .mdebug (bn.unstripped.elf) and must not depend on the checkout
            rel = os.path.relpath(b['file'][:-2] + '.inc', MERGED)
            inc = f'\t.include\t"{rel}"\t# blob {b["name"]} 0x{b["start"]:08x}'
            d = SECDIR[b['sec']]
            if b['mode'] == 'head':
                i = next(k for k, l in enumerate(lines) if l.strip() == d or l.strip().startswith(d + '\t'))
                lines.insert(i + 1, inc)
            else:
                lines += [f'\t{d}', inc]
        out = f'{MERGED}/{base}'
        open(out + '.s', 'w').write('\n'.join(lines))
        sh(f'cd {MERGED} && {os.path.abspath(AS)} -mips2 -m5900 {flags.get(u, "")} -o {base}.o {base}.s')
    # check: every blob has its section with the right size
    bad = 0
    for b in o['blobs']:
        p = b['file'] if b['mode'] == 'object' else o['merged'][b['unit']]
        s = ELFFile(open(p, 'rb')).get_section_by_name(b['sec'])
        need = b['end'] - b['start']
        okk = s is not None and (s['sh_size'] >= need)
        bad += not okk
        print(f"{'ok' if okk else 'XX'} {b['mode']:8s} {b['sec']:10s} 0x{b['start']:08x} +{need:7d} B -> {p} "
              f"({s['sh_size'] if s else '-'} B)")
        if not okk and bad <= 20:
            print(f"error: blob {b['sec']} 0x{b['start']:08x} +{need} B: section in {p} missing or smaller "
                  f"({s['sh_size'] if s else '-'} B)", file=sys.stderr)
    if bad > 20:
        print(f'... and {bad - 20} more, see build/relink/make_blobs.log', file=sys.stderr)
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
