#!/usr/bin/env python3
"""Download and extract the third-party files the link needs, into build/relink/dl/.

Usage: python3 tools/relink/fetch.py        (from the repo root; `make link` calls it)

They are third-party binaries: they live only in build/ (gitignored), never in the repo.
Every file is checked against a known sha1; a cached file with the right sha1 is not
downloaded again. Two sources:

  1. GCC-29_1.RPM of the PS2 Linux Kit (disc 2, /SCEI/RPMS/; the same disc has a
     namesake in /SRPMS/ which is the source, not that one). From the disc already
     downloaded by `make kit` (docker/kit/tmp/disc2.iso; if missing tools/fetch_kit.py
     downloads it). crtbegin.o, crtend.o and libgcc.a are extracted from it (rpm2cpio +
     cpio in the open-bn-env container, like fetch_kit.py) into
     build/relink/dl/gcc29/x/, and the members of libgcc.a into
     build/relink/dl/ar/libgcc/ (read only by missing_syms.py).
  2. PSBBN Definitive Patch v4.1.0 (archive.org, item psbbn-definitive-patch-v4.1,
     143 MB): PSBBN's libc and ld.so, libstdc++, libocm, and the reference libfreetype
     (needed only to know which symbols it resolves: the link uses a stub, make_libs.py).
     Extracted into build/relink/dl/dp41/ under the real file name (the tar has symlinks).

Why these and not the kit's: docs/linking.md.
"""
import hashlib, os, shutil, struct, subprocess, sys, tarfile, urllib.request

VERBOSE = bool(os.environ.get('VERBOSE')) or '-v' in sys.argv[1:]

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
from paths import DL, DP, GCC29, GCCLIB, LIBGCC_AR, KIT_DISC2  # noqa: E402

GCC_RPM = f'{DL}/gcc29/GCC-29_1.RPM'
GCC_RPM_ISO_PATH = '/SCEI/RPMS/GCC-29_1.RPM'
GCC_RPM_SHA1 = '327f42f209dbb1967e8b14591dca6d467143c9e4'
GCC_FILES = {  # files extracted from the RPM -> sha1
    'usr/lib/gcc-lib/mipsEEel-linux/2.95.2/crtbegin.o': 'a20c88e0d19e777f20aee0ae94a3a40d7e996cb6',
    'usr/lib/gcc-lib/mipsEEel-linux/2.95.2/crtend.o': 'b2e8c0dfd30f8bfcd592e38541e0369528ca7a17',
    'usr/lib/gcc-lib/mipsEEel-linux/2.95.2/libgcc.a': 'd39ec0cb438e5cca7244c3799afedf0ec884086d',
}

DP_URL = ('https://archive.org/download/psbbn-definitive-patch-v4.1/'
          'psbbn-definitive-patch-v4.1.0.tar.gz')
DP_TAR = f'{DL}/psbbn-definitive-patch-v4.1.0.tar.gz'
DP_TAR_SHA1 = '21501ac7e2b77807dddfecb83dd98e3f9eceb2cb'
DP_FILES = {  # tar member (the real file, not the symlink) -> sha1
    './__linux.1/lib/libc-2.2.2.so': '744284e54196c4a97948c7498689d0525ec4232b',
    './__linux.1/lib/ld-2.2.2.so': 'e2827e1661db147f3024cf80213ab17adf5fe390',
    './__linux.1/usr/lib/libstdc++-3-libc6.2-2-2.10.0.so': 'cb6dba42929cf2fadf8505961ae1b934e454bcb4',
    './__linux.4/bn/openmg/libocm.so.1.3': 'b594348548288815bfd054fb531b136ab55db067',
    './__linux.4/bn/usr/lib/libfreetype.so.6.1.0': '9be5fd94ac92f70a41393c7be831ceb13ec2f2e7',
}


def sha1(p):
    h = hashlib.sha1()
    with open(p, 'rb') as f:
        for b in iter(lambda: f.read(1 << 20), b''):
            h.update(b)
    return h.hexdigest()


def ok(p, want):
    return os.path.exists(p) and (want is None or sha1(p) == want)


# ---- 1. GCC-29_1.RPM -----------------------------------------------------------------

def iso_extract(iso, path, dest):
    """Extract the file `path` (absolute, ISO 9660 level 1 names) from the ISO."""
    from fetch_kit import iso_walk
    with open(iso, 'rb') as fh:
        fh.seek(16 * 2048)
        pvd = fh.read(2048)
        if pvd[1:6] != b'CD001':
            sys.exit(f'error: {iso} is not an ISO9660 image')
        root = pvd[156:190]
        lba, size = struct.unpack('<I', root[2:6])[0], struct.unpack('<I', root[10:14])[0]
        cur = ''
        for part in path.strip('/').split('/'):
            want = f'{cur}/{part}'
            hit = next((x for x in iso_walk(fh, lba, size, cur) if x[0] == want), None)
            if hit is None:
                sys.exit(f'error: {want} not found in {iso}')
            cur, lba, size = want, hit[1], hit[2]
        fh.seek(lba * 2048)
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        with open(dest, 'wb') as out:
            out.write(fh.read(size))


def ar_members(path):
    """Members of an ar archive (GNU/SysV format, the one of the kit's libgcc.a): [(name, bytes)]
    in order, without the symbol table (`/`) and the long names table (`//`). Done in Python and
    not with `ar` 2.9EE because fetch.py runs on the host, where the Sony binutils (Linux
    binaries) may not run (Mac). Exits if the format is not the expected one."""
    data = open(path, 'rb').read()
    if data[:8] != b'!<arch>\n':
        sys.exit(f'error: {path}: not an ar archive')
    out, longnames, p = [], b'', 8
    while p < len(data):
        hdr = data[p:p + 60]
        if len(hdr) < 60 or hdr[58:60] != b'`\n':
            sys.exit(f'error: {path}: broken ar header at {p}')
        name, size = hdr[:16].decode('ascii').rstrip(), int(hdr[48:58].decode('ascii'))
        body = data[p + 60:p + 60 + size]
        if len(body) != size:
            sys.exit(f'error: {path}: truncated member at {p}')
        if name == '//':
            longnames, name = body, None
        elif name.startswith('/') and name[1:].isdigit():
            off = int(name[1:])
            name = longnames[off:longnames.index(b'/\n', off)].decode('ascii')
        elif name in ('/', '/SYM64/'):
            name = None
        elif name.startswith('#1/'):
            sys.exit(f'error: {path}: BSD member names not supported')
        elif name.endswith('/'):
            name = name[:-1]
        if name is not None:
            if not name or '/' in name or name in ('.', '..'):
                sys.exit(f'error: {path}: unexpected member name {name!r}')
            out.append((name, body))
        p += 60 + size + (size & 1)
    return out


def fetch_gcc():
    if not all(ok(f'{GCC29}/{p}', w) for p, w in GCC_FILES.items()):
        if not ok(GCC_RPM, GCC_RPM_SHA1):
            if not os.path.exists(KIT_DISC2):
                print(f'fetch: {KIT_DISC2} missing, downloading it with tools/fetch_kit.py (disc only)')
                import fetch_kit
                off, csize, usize = fetch_kit.zip_member(fetch_kit.ITEM_URL, fetch_kit.DISC)
                fetch_kit.download_iso(fetch_kit.ITEM_URL, off, csize, usize, KIT_DISC2)
            iso_extract(KIT_DISC2, GCC_RPM_ISO_PATH, GCC_RPM)
            if sha1(GCC_RPM) != GCC_RPM_SHA1:
                sys.exit(f'error: unexpected sha1 of {GCC_RPM}')
        os.makedirs(GCC29, exist_ok=True)
        # rpm2cpio/cpio in the container (the host may lack them), with the host's user
        # so the files are not owned by root
        script = (f'set -e; cd /work/{GCC29}; rpm2cpio /work/{GCC_RPM} | cpio -idmu --quiet '
                  + ' '.join(GCC_FILES))
        subprocess.run(['docker', 'run', '--rm', '--platform', 'linux/amd64', '--network', 'none',
                        '--user', f'{os.getuid()}:{os.getgid()}', '-v', f'{os.getcwd()}:/work',
                        'open-bn-env', 'sh', '-c', script], check=True)
        for p, w in GCC_FILES.items():
            if not ok(f'{GCC29}/{p}', w):
                sys.exit(f'error: {p} not extracted from {GCC_RPM} or unexpected sha1')
    if VERBOSE:
        print(f'ok  crtbegin.o crtend.o libgcc.a  -> {GCCLIB}')
    # members of libgcc.a: missing_syms.py reads them to know which symbols it defines.
    # Complete if it has exactly the members of `ar t`; otherwise extract into a temporary
    # folder and rename it, so an interrupted extraction never looks complete
    lib = f'{GCCLIB}/libgcc.a'
    arch = ar_members(lib)
    members = sorted(set(n for n, _ in arch))
    if not os.path.isdir(LIBGCC_AR) or sorted(os.listdir(LIBGCC_AR)) != members:
        tmp = LIBGCC_AR + '.tmp'
        shutil.rmtree(tmp, ignore_errors=True)
        os.makedirs(tmp)
        for n, data in arch:              # like `ar x`: on a repeated name the last member wins
            with open(os.path.join(tmp, n), 'wb') as f:
                f.write(data)
        if sorted(os.listdir(tmp)) != members:
            sys.exit(f'error: incomplete extraction of {lib}')
        shutil.rmtree(LIBGCC_AR, ignore_errors=True)
        os.rename(tmp, LIBGCC_AR)
    if VERBOSE:
        print(f'ok  {len(os.listdir(LIBGCC_AR))} members of libgcc.a -> {LIBGCC_AR}')


# ---- 2. Definitive Patch v4.1.0 --------------------------------------------------------

def fetch_dp():
    dst = {m: f'{DP}/{os.path.basename(m)}' for m in DP_FILES}
    if all(ok(dst[m], w) for m, w in DP_FILES.items()):
        if VERBOSE:
            print(f'ok  {len(DP_FILES)} Definitive Patch files -> {DP} (cached)')
        return
    if not ok(DP_TAR, DP_TAR_SHA1):
        os.makedirs(DL, exist_ok=True)
        print(f'fetch: downloading {DP_URL} (143 MB)')
        with urllib.request.urlopen(DP_URL, timeout=60) as r, open(DP_TAR + '.part', 'wb') as f:
            for b in iter(lambda: r.read(1 << 20), b''):
                f.write(b)
        os.rename(DP_TAR + '.part', DP_TAR)
        if sha1(DP_TAR) != DP_TAR_SHA1:
            sys.exit(f'error: unexpected sha1 of {DP_TAR}')
    os.makedirs(DP, exist_ok=True)
    with tarfile.open(DP_TAR, 'r:gz') as t:
        for m in DP_FILES:
            ti = t.getmember(m)
            if not ti.isfile():
                sys.exit(f'error: {m} in the tar is not a regular file')
            with t.extractfile(ti) as src, open(dst[m], 'wb') as out:
                out.write(src.read())
    for m, w in DP_FILES.items():
        if sha1(dst[m]) != w:
            sys.exit(f'error: unexpected sha1 of {dst[m]}')
    if VERBOSE:
        print(f'ok  {len(DP_FILES)} Definitive Patch files -> {DP}')



def main():
    fetch_gcc()
    fetch_dp()


if __name__ == '__main__':
    main()
