# Bytes taken from the original

The image that `make link` produces (`build/relink/bn.elf`) is built from our sources, **except for
what is listed here**: parts copied at build time from `orig/bn-jpn.elf`, the ELF the user supplies.
They exist to reproduce the original byte for byte (the SHA1); they are not reconstructions. Whoever
develops `bn` from here (changes it, moves addresses, ports it to another toolchain) must know which
of them to drop or regenerate, because in that context they mean nothing or become wrong.

Rule for a new entry: the step that copies from the original says so in its docstring and in its log,
and gets a line here.

## Policy: copyrighted bytes stay out of the repository

The repository holds only what we write (decompiled sources, tools, configuration, documentation)
and third-party material that its license lets us redistribute (the vendored libraries in `ref/`, the
GPL toolchain sources). Sony's bytes never enter it, as in n64decomp/sm64 and zeldaret/oot: they are
extracted from the user's ELF into gitignored directories and included again at build time.

- An **asset** is a run of bytes that no compiler produced from our sources: audio, microcode, keys,
  high-entropy tables, encrypted code. Strings, constant tables, jump tables, vtables and the
  `AMX_NATIVE_INFO` tables are not assets: the compiler emits them from the source, so they are source.
- Never transcribe an asset into a tracked file: no C array with the bytes, no `.word`/`.byte` of
  original bytes, no test fixture with them. Only addresses, sizes, classes and hashes are tracked.
- `orig/`, `assets/`, `build/`, `expected/`, `*.elf` and `*.bin` are gitignored. Before committing a
  large or binary file, check that it holds no byte of the original.
- ## 1. Fields with no meaning: drop them when developing `bn`

They do not describe the program: they reproduce uninitialized memory of Sony's linker, or
information that the strip removed.

| what | where | why it is copied | when developing `bn` |
|---|---|---|---|
| `relvaddr` of the 6,583 `.compact_rel` entries (19 bits each) | `tools/relink/compact_rel.py`, called by `tools/relink/link.sh` | ld 2.9EE never assigns it: the file gets whatever was on the stack of Sony's linker | remove `compact_rel.py` from `link.sh`: the `.compact_rel` written by ld is valid |
| `cpt.offset` of the `.compact_rel` header (4 B) | `tools/relink/compact_rel.py` | it is the file position of `.compact_rel` before the strip, so it depends on the size of the `.mdebug` (7,529,808 B in the original), which the strip removed and the binary does not keep | as above |

## 2. DNAS protection: data of Sony's protection tool

| what | where | why it is copied | when developing `bn` |
|---|---|---|---|
| 21 descriptor words for each of the 166 self-modifying regions and 2 before each of the 31 hidden strings (cipher codes, ids, encrypted lengths, filler) | `tools/relink/dnas_protect.py` (after the strip, in `link.sh`) | Sony's protection tool chose them after the link; our build writes the placeholder `0xd5c0d5c0` (`tools/dnas_subst.py`) | if the code or the address of a region changes, the copied descriptors become wrong (lengths, excluded words): generate new ones, or remove the protection (`src/dnas/dnas_prot.h`, `tools/dnas_subst.py`, `tools/relink/dnas_protect.py`) |
| tables of the words excluded from encryption (in `.text`) | blobs of `tools/relink/make_blobs.py` (item 3) | as above | as above: they depend on the relocations inside the regions |

The encryption itself is ours: `dnas_protect.py` encrypts our bytes with the parameters it reads from
the linked binary. Details: `docs/dnas.md`.

## 3. Bytes that no compiler produced

They enter in two ways.

**Assets inside a compiled object** (`config/assets.txt`, 143 rows, 668,132 B). The object is
declared in our source and its initializer is an `#include` of a file of `assets/`:

```c
static const struct DnasPubKey dnas2_hdr_pubkey = {{
#include "../assets/dnas/dnas2m60_verify_hdr_pubkey.inc"
}};
```

`tools/extract_assets.py` reads each row (name, address, size, SHA1 prefix, file), takes the bytes from
`orig/bn-jpn.elf`, checks the hash and writes `assets/<file>` as the body of a C initializer. It runs in
`make check` (before the gate), and in `tools/match`. These bytes are compared
by the data gate like any other data, and placed by the symbol, not by address.

| content | rows | bytes | source |
|---|---:|---:|---|
| audio banks and sequences (CSL `SSsq`, `SShd`, ADPCM `bbsebd`) | 12 | 611,222 | `src/audio/sound.c` |
| VU1 microcode, two programs loaded by `gm_graph::Init` (produced by `dvp-as`, not gcc) | 2 | 31,648 | `src/gfx/gm_graph.cpp` |
| DNAS keys, RSA public keys, DES keys and IVs, high-entropy tables, root CAs | 123 | 25,198 | `src/dnas/` |
| keys and IVs of the CFD module | 6 | 64 | `src/cfd/cfd2.c` |

**Whole blobs outside every unit** (`tools/relink/make_blobs.py`, 6 blobs, 101,552 B). They sit between
objects in the original, so they are cut from `orig/bn-jpn.elf` into `build/relink/blobs/` (never into
the repository) and linked as objects in the place `tools/relink/link_order.py` computes from the
original addresses. gas 2.9EE has no `.incbin`: the bytes go through a generated `.inc` of
`.word`/`.byte`. Their labels (name, type, size, binding) come from the original `.dynsym`.

| content | section | bytes | class in `config/data_regions.txt` |
|---|---|---:|---|
| Chloe stage 1 and stage 2, encrypted code (`docs/dnas.md`, "Chloe") | `.data` | 44,000 | `encrypted` |
| ATOK IME conversion tables (`atok12n_key`/`_roma`, `ps2ime_key`/`_roma`, serialized as hex strings) | `.data` | 23,040 | `asset` |
| DNAS tables in `.text` (`_sce_dnas_pwrap_info_tbl`, `__sce_dnas2_symbol11001`, `__sce_dnas2_symbol12001`) | `.text` | 34,512 | outside the data gate (`.text`) |

They are placed at the original addresses: if the layout changes they must be tied to symbols.

Not taken from the original although `config/data_regions.txt` lists them: the two crt1.o objects
(`_IO_stdin_used`, `__data_start`; 32 B, the kit's crt1.o is linked as is) and 48 B of linker padding
before the 64-aligned `.data` of `sound.o`.

## 4. Third-party libraries and their substitutes

| what | where | why | when developing `bn` |
|---|---|---|---|
| stub of `libfreetype.so.6`: names, types and sizes of its symbols from the original `.dynsym`; `_gp_disp` and `_procedure_table_size` of the real library, read from the original's GOT entries, because ld copies them into the GOT of `bn` | `tools/relink/make_libs.py` | the FreeType 2 build Sony linked against has not been found (the one of the Definitive Patch is another build) | link against a real libfreetype; the stub is only an interface and is not needed at run time (the system library is loaded) |

The other libraries of the link (PSBBN's libc, libstdc++ and libocm, the kit's libpthread, libm, libdl,
libcrypt) are real third-party files downloaded at build time and checked by SHA1; they live only in
`build/relink/` (`docs/linking.md`).

## 5. Layout: addresses of the original (configuration, not bytes)

| what | where | when developing `bn` |
|---|---|---|
| start address of every output section | `config/bn.x` | remove them (ld's default script) as soon as the code changes size |
| order of the objects and position of the blobs | `tools/relink/link_order.py` (from the addresses of `config/symbol_addrs.txt`) | the order stays valid; the blob positions must be tied to symbols |
| date and time of the original build for `__DATE__`/`__TIME__` of the giflib units | `config/unit_times.txt` (`tools/fixtime.py`, in the gate and therefore in the linked objects) | remove the rows: a rebuilt `bn` carries its own date |
