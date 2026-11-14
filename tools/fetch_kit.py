#!/usr/bin/env python3
"""Fetches the pieces of the PS2 Linux Kit (disc 2) the decomp needs and builds
docker/kit/rootfs -- the automatic recipe of the kit bootstrap.

Source: archive.org item `ps2linux-pal` = a single zip (~1.19GB) holding
PS2Linux-disc1.iso and PS2Linux-disc2.iso deflated. The server-side unzip
endpoint of archive.org truncates at ~1GiB, so:

  1. read the central directory of the zip (last 64KB, HTTP range) and find
     the PS2Linux-disc2.iso member (offset + compressed size);
  2. download only that member with a Range and inflate it while streaming to
     docker/kit/tmp/disc2.iso (cache: if it is already there, skip the download);
  3. walk the ISO9660 filesystem (minimal parser on a seekable file, no
     dependencies) and extract, anywhere in the tree, the files matching the globs
     (default: GLIBC-_* and KERNEL_3*, the glibc and kernel header RPMs);
  4. unpack the RPMs into docker/kit/rootfs with rpm2cpio+cpio inside the
     open-bn-env container as the host user (needs `make env` first) and create the
     usr/src/linux -> linux-2.2.1_ps2 symlink that setup_cxx_headers.sh expects.

Usage: python3 tools/fetch_kit.py [--rpm-glob GLOB ...] [--purge-iso]
Then:  make headers   (C++ headers of the period)
"""
import argparse
import fnmatch
import os
import shutil
import struct
import subprocess
import sys
import urllib.request
import zlib

ITEM_URL = 'https://archive.org/download/ps2linux-pal/ps2linux-pal.zip'
DISC = 'PS2Linux-disc2.iso'
KIT = 'docker/kit'
ISO = f'{KIT}/tmp/disc2.iso'
ROOTFS = f'{KIT}/rootfs'
DEFAULT_GLOBS = ['GLIBC-_*', 'KERNEL_3*']  # ISO 8.3 names (level 1)
CHUNK = 1 << 20
VERBOSE = os.environ.get('VERBOSE', '') not in ('', '0')


def vprint(*a, **kw):
    if VERBOSE:
        print(*a, **kw)


def progress(msg, end=False):
    """One line rewritten in place on a terminal, one line per step otherwise."""
    if sys.stdout.isatty():
        print('\r' + msg, end='\n' if end else '', flush=True)
    elif not end:
        print(msg, flush=True)


def http(url, first=None, last=None):
    req = urllib.request.Request(url)
    if first is not None:
        req.add_header('Range', f'bytes={first}-{last}')
    return urllib.request.urlopen(req)


def zip_member(url, name):
    """(data_offset, csize, usize) of the member `name` via the central directory."""
    size = int(http(url, 0, 0).headers['Content-Range'].split('/')[1])
    tail = http(url, max(0, size - 65536), size - 1).read()
    i = tail.rfind(b'PK\x05\x06')
    if i < 0:
        sys.exit('error: zip EOCD not found (zip64?)')
    _, _, _, _, n, cdsize, cdoff, _ = struct.unpack('<IHHHHIIH', tail[i:i + 22])
    cd = tail[cdoff - (size - len(tail)):]
    j = 0
    for _ in range(n):
        (sig, _, _, _, method, _, _, _, csize, usize, nlen, elen, clen,
         _, _, _, off) = struct.unpack('<IHHHHHHIIIHHHHHII', cd[j:j + 46])
        assert sig == 0x02014b50
        fname = cd[j + 46:j + 46 + nlen].decode()
        j += 46 + nlen + elen + clen
        if fname == name:
            assert method == 8, 'expected deflate'
            lh = http(url, off, off + 29).read()  # local header: 30B + name + extra
            lnlen, lelen = struct.unpack('<HH', lh[26:30])
            return off + 30 + lnlen + lelen, csize, usize
    sys.exit(f'error: member {name!r} not in the zip')


def download_iso(url, data_off, csize, usize, dest):
    if os.path.exists(dest) and os.path.getsize(dest) == usize:
        vprint(f'{dest} already cached, download skipped')
        return
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    resp = http(url, data_off, data_off + csize - 1)
    dec = zlib.decompressobj(-15)  # raw deflate
    read = 0
    with open(dest + '.part', 'wb') as out:
        while True:
            raw = resp.read(CHUNK)
            if not raw:
                break
            read += len(raw)
            out.write(dec.decompress(raw))
            if read % (50 << 20) < CHUNK:
                progress(f'kit        downloading {DISC}: {read >> 20}/{csize >> 20} MB')
        out.write(dec.flush())
    progress(f'kit        downloading {DISC}: {csize >> 20}/{csize >> 20} MB', end=True)
    got = os.path.getsize(dest + '.part')
    if got != usize:
        sys.exit(f'error: incomplete ISO: {got} != {usize} bytes')
    os.rename(dest + '.part', dest)


def iso_walk(fh, lba, size, path=''):
    """Yields (path, lba, size, is_dir) for the records of an ISO9660 directory."""
    fh.seek(lba * 2048)
    data = fh.read(size)
    i = 0
    while i < len(data):
        ln = data[i]
        if ln == 0:  # padding up to the next sector
            i = (i // 2048 + 1) * 2048
            continue
        rec = data[i:i + ln]
        r_lba = struct.unpack('<I', rec[2:6])[0]
        r_size = struct.unpack('<I', rec[10:14])[0]
        flags = rec[25]
        nlen = rec[32]
        name = rec[33:33 + nlen].decode('ascii', 'replace').split(';')[0]
        i += ln
        if name in ('\x00', '\x01', '', '.', '..'):
            continue
        yield f'{path}/{name}', r_lba, r_size, bool(flags & 2)


def extract_globs(iso_path, globs, outdir):
    """Looks for the globs in the whole ISO tree and extracts them into outdir."""
    found = []
    with open(iso_path, 'rb') as fh:
        fh.seek(16 * 2048)
        pvd = fh.read(2048)
        if pvd[1:6] != b'CD001':
            sys.exit('error: not an ISO9660 image')
        root = pvd[156:190]
        stack = [('', struct.unpack('<I', root[2:6])[0],
                  struct.unpack('<I', root[10:14])[0])]
        while stack:
            path, lba, size = stack.pop()
            for p, r_lba, r_size, is_dir in iso_walk(fh, lba, size, path):
                if is_dir:
                    stack.append((p, r_lba, r_size))
                elif any(fnmatch.fnmatch(os.path.basename(p), g) for g in globs):
                    dest = os.path.join(outdir, os.path.basename(p))
                    fh_pos = fh.tell()
                    fh.seek(r_lba * 2048)
                    with open(dest, 'wb') as out:
                        left = r_size
                        while left:
                            buf = fh.read(min(CHUNK, left))
                            out.write(buf)
                            left -= len(buf)
                    fh.seek(fh_pos)
                    vprint(f'  extracted {p} ({r_size} bytes)')
                    found.append(os.path.basename(p))
    return found


def build_rootfs(rpms):
    """rpm2cpio+cpio inside open-bn-env (the image has them; the host may not), as the
    host user so that the rootfs is not root-owned (cpio run by a user ignores the
    ownership in the archive). The rootfs is rebuilt from scratch every time."""
    if os.path.lexists(ROOTFS):
        try:
            shutil.rmtree(ROOTFS)
        except PermissionError:
            # a rootfs unpacked as root by an older version of this script
            subprocess.run(['docker', 'run', '--rm', '--platform', 'linux/amd64',
                            '-v', f'{os.getcwd()}:/work', 'open-bn-env',
                            'rm', '-rf', '/work/' + ROOTFS], check=True)
    os.makedirs(ROOTFS)
    script = ('set -e; cd /work/' + ROOTFS + '; '
              + '; '.join(f'rpm2cpio "/work/{KIT}/{r}" | cpio -idmu --quiet' for r in rpms)
              + '; cd usr/src 2>/dev/null && for d in linux-*; do '
                '[ -e linux ] || ln -s "$d" linux; done; true')
    subprocess.run(['docker', 'run', '--rm', '--platform', 'linux/amd64',
                    '--user', f'{os.getuid()}:{os.getgid()}', '-e', 'HOME=/tmp',
                    '-v', f'{os.getcwd()}:/work',
                    'open-bn-env', 'sh', '-c', script], check=True)


def main():
    ap = argparse.ArgumentParser(description='Fetch the PS2 Linux Kit pieces (disc 2) and build docker/kit/rootfs.')
    ap.add_argument('--rpm-glob', action='append', default=None, metavar='GLOB',
                    help=f'file pattern to extract (default: {DEFAULT_GLOBS})')
    ap.add_argument('--purge-iso', action='store_true',
                    help='delete the cached ISO at the end')
    args = ap.parse_args()
    globs = args.rpm_glob or DEFAULT_GLOBS

    vprint(f'central directory of {ITEM_URL}')
    off, csize, usize = zip_member(ITEM_URL, DISC)
    vprint(f'{DISC}: {csize >> 20}MB compressed, {usize >> 20}MB ISO; download+inflate to {ISO}')
    download_iso(ITEM_URL, off, csize, usize, ISO)
    rpms = extract_globs(ISO, globs, KIT)
    if not rpms:
        sys.exit(f'error: no file matches {globs}')
    build_rootfs(rpms)
    if args.purge_iso:
        os.remove(ISO)
    print(f'kit        {len(rpms)} RPMs unpacked in {ROOTFS}; next: make headers')


if __name__ == '__main__':
    main()
