# open-bn

The source code of `bn`, the main program of the PlayStation BB Navigator
(PSBBN), the PS2 Linux menu, browser and installer that shipped with the
PlayStation 2 network adaptor and HDD in Japan.

The code comes from a matching decompilation: the C and C++ sources, built with
the original Sony toolchain, reproduce the shipped executable byte for byte. That
identical build is the starting point for fixing and changing the program.

The repository contains no original binaries or assets. You need your own copy
of the executable (see below); sound banks, encrypted blobs and other data that
no compiler produced are extracted from it at build time.

## The target

- `bn` is a dynamically linked C++ executable for PS2 Linux (MIPS R5900), not a
  native PS2 program. It loads further modules from `bn/modules/` (music,
  movies, photos, mail, ...), which are not part of this repository.
- Compiler: Sony gcc 2.95.2 (`mipsEEel-linux`, 2.95.2-3a); assembler and linker:
  Sony binutils 2.9EE. Both are rebuilt from the PS2 Linux Kit sources.
- Linked libraries (SSLeay, SQLite, zlib, libpng, libjpeg, libtiff, libmng,
  giflib, the Small script interpreter) are built from period upstream sources
  in `ref/`.
- Reference executable: `orig/bn-jpn.elf`, SHA1
  `0bb9908a1d423edc77d116514003866898e5cf6b`. A few fields of the linked file are
  still copied from it; see [`docs/original-bytes.md`](docs/original-bytes.md).

## Building

Requirements, on Linux (tested on Arch and Ubuntu 22.04/24.04, which Linux Mint
21/22 are based on):

- `git`, `make`, `python3`, `curl` and Docker with the buildx plugin. On
  Ubuntu/Mint: `sudo apt install git make python3 curl docker.io docker-buildx`.
- Your user in the `docker` group (`sudo usermod -aG docker $USER`, then log out
  and back in).
- About 6 GB of disk. The first build needs network access: github.com for the
  clone, archive.org for the kit packages and the link inputs, plus ftp.gnu.org,
  Docker Hub and the Debian/Ubuntu package mirrors while the images build.

Clone the repository and run everything below from its root:

```sh
git clone https://github.com/MatteoTurri/open-bn.git
cd open-bn
```

Get the executable (it is the `bn` of the Japanese language pack distributed
with the PSBBN Definitive Patch):

```sh
mkdir -p orig && cd orig
curl -LO 'https://archive.org/download/psbbn-definitive-patch-v4.1/language-pak-jpn-v1.1.0.tar.gz'
tar xzf language-pak-jpn-v1.1.0.tar.gz './__linux.4/bn/bn' && mv __linux.4/bn/bn bn-jpn.elf
rm -r language-pak-jpn-v1.1.0.tar.gz __linux.4
sha1sum bn-jpn.elf    # 0bb9908a1d423edc77d116514003866898e5cf6b
cd ..
```

Set up the toolchain and build:

```sh
make bootstrap      # Docker image, Sony gcc and binutils, PS2 Linux Kit headers
make check          # compile every unit and compare it with the original
make link           # link the executable: build/relink/bn.elf
make link-cmp       # compare it with the original (prints the SHA1)
```

`make bootstrap` takes a few minutes: it builds the toolchain into
`docker/toolchain/` and downloads about 1 GB of PS2 Linux Kit packages from
archive.org. The first `make check` also writes the reference disassembly
(`expected/asm/`); after that a full check takes under a minute on 16 cores.
After a pull that changes `docker/patches/`, rebuild the compiler with
`make sony-gcc`.

## Repository layout

| Path | Contents |
|---|---|
| `src/` | the Sony code of `bn`, one folder per subsystem (see [`src/README.md`](src/README.md)) |
| `ref/` | third-party libraries, as upstream sources plus the changes the binary requires |
| `config/units.txt` | the list of compilation units with their compiler flags |
| `config/` | other inputs of the build and of the checks (symbols, asset list, data regions) |
| `docker/` | Dockerfiles and patches that rebuild the Sony toolchain |
| `tools/` | the checks (`tools/check.py`, `tools/datacheck.py`), the linker pipeline (`tools/relink/`), `tools/match` and a few diagnostics |
| `docs/` | how the gate, the link, DNAS and the compiler work, open questions, code conventions |
| `orig/`, `assets/`, `expected/`, `build/` | your executable, extracted data, reference disassembly, build output (all gitignored) |

## Changing the code

`make check` compiles every unit and compares each function and each block of
data with the original; `make link-cmp` compares the whole linked executable.
For code you have not touched they work as a regression test: every function
you did not change must still match. Many details that look arbitrary
(declaration order, types, temporaries, an unused variable) are what makes the
compiler produce the same bytes, so keep them unless you mean to change the
function. To compare a single function with the original:

```sh
tools/match <symbol> src/<folder>/<file>.cpp
```

How the code is organized and how to read it: [`src/README.md`](src/README.md).
How the checks, the link and the compiler work: [`docs/`](docs/).

## License

The tools, build files and documentation written for this project, and the changes and additions
made to `src/` here, are under the MIT license ([`LICENSE`](LICENSE)). The sources in `src/` reconstruct a
program by Sony Computer Entertainment, over which no rights are claimed or granted; the libraries in `ref/`
and the toolchain sources in `docker/` keep their own licenses.
