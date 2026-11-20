#!/usr/bin/env python3
"""Extract the original modules of bn (config/modules.txt) into orig/modules/<name>.so.

The modules come from the same archive the link already uses, the PSBBN Definitive Patch
v4.1.0 (archive.org, item psbbn-definitive-patch-v4.1, 143 MB, cached in build/relink/dl/ by
tools/relink/fetch.py). Every file is checked against the sha1 of config/modules.txt; a file
already in place with the right sha1 is left alone. Like orig/bn-jpn.elf, the modules are
third-party binaries: orig/ is gitignored and they never enter the repository.

    python3 tools/fetch_modules.py            every module of config/modules.txt
    python3 tools/fetch_modules.py top        only the named ones (`make modules`)
"""
import hashlib
import os
import sys
import tarfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, 'relink'))
import target  # noqa: E402
import fetch   # noqa: E402  (tools/relink/fetch.py: the archive and its sha1)


def sha1(path):
    return hashlib.sha1(open(path, 'rb').read()).hexdigest()


def main():
    os.chdir(target.ROOT)
    mods = target.modules()
    want = sys.argv[1:] or sorted(mods)
    unknown = [n for n in want if n not in mods]
    if unknown:
        sys.exit(f'error: not in {target.REGISTRY}: {" ".join(unknown)}')
    todo = [n for n in want
            if not (os.path.exists(target.get(n).elf) and sha1(target.get(n).elf) == mods[n][1])]
    if todo:
        fetch.download_dp_tar()
        os.makedirs('orig/modules', exist_ok=True)
        with tarfile.open(fetch.DP_TAR, 'r:gz') as t:
            for n in todo:
                ti = t.getmember(mods[n][0])
                if not ti.isfile():
                    sys.exit(f'error: {mods[n][0]} in the tar is not a regular file')
                dst = target.get(n).elf
                with t.extractfile(ti) as src, open(dst + '.part', 'wb') as out:
                    out.write(src.read())
                if sha1(dst + '.part') != mods[n][1]:
                    os.remove(dst + '.part')
                    sys.exit(f'error: unexpected sha1 of {mods[n][0]}')
                os.replace(dst + '.part', dst)
    print(f'modules: {len(want)} in orig/modules/ ({len(todo)} extracted, '
          f'{len(want) - len(todo)} already present)')


if __name__ == '__main__':
    main()
