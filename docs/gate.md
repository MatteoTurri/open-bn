# The gate (`make check`)

`make check` proves that the sources rebuild the original `orig/bn-jpn.elf` piece by piece: every
function, every compiled byte of data and every exception table, plus the identity of every global
symbol and of every local address a function reaches. It compares object files with the original,
not a linked image; the linked image is checked by `make link && make link-cmp` (`docs/linking.md`).
Usage of each tool named here: `tools/README.md`.

## Steps

`make check` runs, in order, and stops at the first failure:

| step | tool | what fails it |
|---|---|---|
| format | `tools/format.py --check` | a file of `src/` not formatted by clang-format 23.1.5 (`docs/code-conventions.md`, section 7) |
| toolchain | `tools/check_toolchain.py` | a compiler patch other than the one allowed, or a gcc build without `-msse2 -mfpmath=sse` (`docs/compiler.md`) |
| assets | `tools/extract_assets.py` | `orig/bn-jpn.elf` missing, or a hash of `config/assets.txt` that does not match (`docs/original-bytes.md`) |
| functions | `tools/check.py` | a function that does not match, or a GOT identity outside the debt |
| data | `tools/datacheck.py` | a placed data block that differs |
| exceptions | `tools/ehcheck.py` | an `.eh_frame` / `.gcc_except_table` block that is not `ok` |

The format check, `check.py`, `datacheck.py` and `ehcheck.py` run in the `open-bn-env` container,
the other steps on the host. Before comparing, `tools/check.py` checks the SHA1 of `orig/bn-jpn.elf` against
`config/bn.yaml`.

## Units

`config/units.txt` is configuration only, one row per compilation unit:
`<source> | <cflags> | <asflags> | <skip, csv>`. It is edited with `tools/unitedit.py` (atomic, refuses
duplicates). Every unit is compiled with `-O2` plus its cflags (`.c` with the Sony gcc, `.cpp` with the
Sony g++), the `.s` goes through `tools/dnas_subst.py` (`docs/dnas.md`; a unit without DNAS markers comes
out unchanged), and gas assembles it with `-mips2 -m5900` plus the asflags. The objects stay in
`build/check/<unit>/out.o`; the link uses exactly these objects.

Two properties of the original build are reproduced at compile time:

- **Source paths.** gcc 2.95 writes the command-line argument as is into `__FILE__`, and the `assert`
  and `ERR_put_error` literals land in `.rodata`. The original build named its files with relative
  paths (`./src/func.c`, `../libtiff/tif_fax3.c`, `./crypto/asn1/../../crypto/asn1/x.c`,
  `objSimpleTextEdit.cpp`); they are read from the original `.rodata`. `config/unit_paths.txt` maps
  each such unit to its original path and `tools/srcpath.py` compiles it from the matching directory
  (a virtual tree in `build/vpath/` where the path has no real counterpart).
- **Build time.** The giflib units write `__DATE__`/`__TIME__` into `.rodata`.
  `config/unit_times.txt` holds the original build time of each, and the compiler runs under
  `tools/fixtime.py`, which rewrites the result of the time system calls (ptrace; Linux x86-64 or
  aarch64 kernel). If it cannot run, `-D__DATE__=... -D__TIME__=...` gives the same `.s`.

A scoped gate runs on a subset: `make check UNITS=<file>`, where the file lists unit rows. Flags and
skips are always re-read from `config/units.txt`. A scoped run writes `build/check-<name>.json` instead
of `build/check.json`, and the attribution of static functions sees only the scoped units, so the
full gate is the reference.

## The function universe

The universe is defined once, in `tools/universe.py`: **10,711 functions, 4,912,576 B**.

- **8,403 named functions** (3,682,256 B): every `type:func` row of `config/symbol_addrs.txt`, which is
  generated from `.dynsym`. The 32 size-0 aliases at the address of a real function (`scepfs*` over
  `scepfsd*`, `stat`, `lstat`) are not counted.
- **2,308 unnamed functions** (1,230,320 B): the binary has no `.symtab`, so the `static` functions
  have no name. `tools/statics.py` finds them in the gaps of `.text`: every function compiled by gcc
  2.95 with `-mabicalls` starts with the `.cpload` triple at an 8-aligned address, and ends at the
  next start minus the zero padding. Applied to the 8,403 named functions as if they were anonymous,
  the rule gives back every start and every size. They are named `static_<ADDR>` (8 upper-case hex
  digits) in `config/static_funcs.txt`, which is generated.
- **163 DNAS self-modifying functions** (114,764 B) are part of both sets and compared whole with the
  decrypted image (`docs/dnas.md`).

Outside the universe: 39,152 B of `.text` that no compiler produced: `_start` of crt1.o (96 B), three
DNAS data tables in `.text` (34,512 B) and the linker's lazy-binding stubs (4,512 B).

## Function comparison

Both sides are disassembled and normalized (`tools/normalize_asm.py`): mnemonic and operands only,
branch targets relative, GOT/`$gp` offsets and relocations masked, and the local-page call form
(`lw t9,%got(page)` + `addiu t9,t9,%lo(f)`, emitted by gas for a callee defined in the same object)
collapsed into the CALL16 form. Two functions match when the normalized rows are identical. The
original side is disassembled once and cached in `build/cache/`, keyed by the hash of the normalizer.

### Identity of the static functions

A local function of an object has no name in the binary, so the gate decides which original address
it is. `tools/check.py` collects candidates for every local and then assigns them globally, once, in
this order:

1. **code anchors**: the named functions of the same object have known addresses, and the linker
   lays out a `.text` section whole, so a local sits at `anchor + (its offset - anchor's offset)`;
2. **data anchors**: a named data object of the same object (an `AMX_NATIVE_INFO` table, a vtable)
   whose words point at local functions gives their addresses directly;
3. **content**: a body identical to an unclaimed static of the universe.

An address is taken by one local only. Several identical locals of one unit with the same candidates
("twins") are assigned in object order to the nearest candidates. When a small body is identical to
many statics, the bytes are verified but which one we wrote is not: the gate prints the functions
attributed by content with 20 or more candidates. Locals that match no function of the original are
reported as "out of alignment" (`build/check_statics.json`).

### Orphan linkonce copies

An inline function or template instance without an effective `#pragma interface` goes into its own
`.gnu.linkonce.t.<name>` section. The linker keeps the first such section of each name and places it
at the end of `.text`; if the name also has a strong definition elsewhere, the symbol goes to the strong
definition and the kept section stays in the image as code without a name. Those are statics of the
universe too (the six after `crtstuff_end` are copies of `mat4_identity`, `apply`, `multiply`,
`mat4_translate`, `mat4_scale`, `mat4_quat` emitted by `camera.o` and `object.o`). The gate offers such
copies as candidates for the statics of the linkonce region only; `CHECK_ORPHAN_LINKONCE=0` turns this
off. Background: `docs/compiler.md`, emission order.

### Skips

The skip column of `config/units.txt` lists functions a unit compiles but that are not judged
(measured only). What remains there are template copies (`basic_string::replace`, `_S_chunk_alloc`,
`_Rb_tree::_M_copy`, ...) that the unit emits but the linker discards: the copy in the binary comes
from another unit, and this unit's copy differs because of its flags (with `-fno-exceptions` the
range checks of `bastring.cc` are `assert`s, which `-DNDEBUG` removes; `-O3` inlines `_S_oom_malloc`
into `_S_chunk_alloc`). Data blocks use the form
`data:<section>`.

In a full run every function of the universe must have an outcome; one that disappears (a function
that lost its C linkage still matching as a static, its named symbol gone) is an error. In C units gcc
2.95 does not stop on an implicit declaration, so the gate compiles with
`-Wimplicit-function-declaration` (no effect on the code) and reports every one.

## GOT identity (`tools/gotident.py`)

The row comparison masks GOT offsets, so it cannot see which global a function calls or reads: `free`
for `R_free`, `strncmp` for `strncasecmp`, `int` for `bool` in a callee's mangling, a call to the wrong
static of the same unit all give the same instructions. Three further comparisons run on every green
copy of a function (a template has one copy per unit):

1. **Global symbols.** The ordered sequence of the global names referenced through the GOT. On the
   original side the name comes from the position: on MIPS o32 global GOT entry `local_gotno + k` is
   `.dynsym` entry `gotsym + k`. On our side, the CALL16/GOT16 relocations to non-local symbols.
2. **Data through the local GOT.** The section class (`.text`, `.rodata`, `.data`, `.bss`) of every
   object reached through the local GOT, and its name when the original object has one. This catches
   a `.bss` table where the original has initialized `.data` (a missing explicit `= 0` puts a static
   in `.bss`), a buffer where the original has a literal, a datum where the original passes a function.
3. **LO16 targets.** The absolute target of every local GOT + `%lo` pair: code must reach the same
   function at the same offset, and data must sit at one constant offset per (unit, section), which
   the green copies of the unit vote on.

Each dimension has a debt file with a ratchet: `config/got_debt.txt`, `config/gotlocal_debt.txt`,
`config/gotlo16_debt.txt`. A dirty copy outside the debt, or a debt row whose distance grows or whose
fingerprint changes, fails the gate. `make got-debt` only removes rows (adding needs `ARGS=--add`).
All three files are empty. Limits (pointers inside `.data` tables, jump-table entries, static versus
global linkage of a datum): docstring of `tools/gotident.py`; the identical link SHA1 covers them for
the current binary.

A diagnosis `[TU]` means the name is right but the body is in the wrong translation unit: gas never
emits CALL16 to a strong global of the same object, so a CALL16 in the original towards a function our
object defines says that, in the original, it lived in another object.

## Data (`tools/datacheck.py`)

The data gate compares `.rodata`, `.data`, `.ctors` and `.dtors` byte by byte: 1,083,856 B in the
original (249,860 + 833,624 + 268 + 104). Of these, **67,120 B** were not produced by a compiler from
our sources and are listed with their class in `config/data_regions.txt` (only ranges, never bytes):
the two encrypted Chloe stages, the ATOK IME tables, two crt1.o objects and 48 bytes of linker padding.
The other **1,016,736 B** are compiled data, all verified. Asset bytes inside a compiled object (audio
banks, VU microcode, DNAS keys) are included from `assets/` by the source and compared like any other
data (`docs/original-bytes.md`).

Each section of each object is placed in the original by:

- **name anchors**: objects of our `.o` that exist in `.dynsym` (globals, `__vt_*`);
- **code anchors**: in a matched function, a `%got(.rodata+A)` / `%lo` pair sits at the same row as
  the original's `lw rX,OFF(gp)` / `addiu`, which gives the original address of offset A;
- then link order and content, for blocks without anchors.

A word with an `R_MIPS_32` relocation is compared as `base(symbol) + addend`, or masked when the base is
unknown. Block states: `ok`, `fail` (placed and different: fails unless in skip), `conflict` (anchors
disagree: the objects are laid out in another order), `unanchored`, `out-of-section`, `order-differs`,
`bss` (sizes only).

## Exception tables (`tools/ehcheck.py`)

gcc 2.95.2 uses the old exception scheme: per object one CIE with augmentation `"eh"` pointing to its
`__EXCEPTION_TABLE__` and one FDE per function in `.eh_frame` (21,664 B), and per object a list of
(start, end, handler) triples closed by `-1, -1` in `.gcc_except_table` (2,724 B). Blocks are placed by
the `pc_begin` of their FDEs translated through the matched functions. A block that is not `ok`
(different bytes, a CIE boundary in another place, permuted FDEs) fails the gate. Original FDEs that no
unit produces are counted per unit, not judged. Which units are compiled with exceptions, and why `-g`
matters for `pc_range`: `docs/compiler.md`.

## Outputs

`build/check.json` (per function: outcome, identity, size), `build/check_statics.json`,
`build/datacheck.json`, `build/ehcheck.json`.

## Working on one function

- `tools/match <symbol> <source> [flags]` compiles one unit exactly as the gate does (original path,
  build time, DNAS substitution, assets) and compares one function, named or `static_<ADDR>`. `MATCH
  n/n` and exit 0 mean identical; a match whose GOT identity differs outside the debt exits 1.
  Flags on the command line replace those of `config/units.txt`.
- `expected/asm/<ADDR>_<name>.s` is the reference listing of every function (`make split`; for the DNAS
  functions it is the decrypted, scaffolded reference).
- `tools/gotident.py SYM` prints both sides of the three identity comparisons.
- `tools/datacmp.py UNIT SEC` shows the bytes of a data block that does not match.
- `tools/layoutcheck.py` tells whether each object, laid at a base, reproduces the original addresses
  (`docs/linking.md`).

A change to `src/` is accepted when `make format`, `make check` and `make link && make link-cmp`
(identical SHA1) all pass.
