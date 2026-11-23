# The modules of bn

`bn` loads part of its features at run time from shared objects in `/opt0/bn/modules/`:
it `dlopen`s `<name>.so`, calls its `Init` and registers its `FuncTable`, a table of
Small (Pawn) natives that the XML pages of the interface call. Its input method also
`dlopen`s the two libraries of the ATOK engine (`src/ime/atok.c`). Each module is a
decompilation target of its own, built with the same toolchain as `bn` (g++ 2.95.2 Sony,
MIPS r5900, `-mabicalls`: the ELF flags are `0x20920003` for `bn` and for every module).

## Where the originals come from

The modules are in the full archive of the PSBBN Definitive Patch v4.1.0 (archive.org, item
`psbbn-definitive-patch-v4.1`), the same archive `make link` downloads for the shared
libraries, except `player.so`: the language packs of the same item carry it, each translated
(the English one differs in 85 bytes of text), and the Japanese one, the same in every version
of the Japanese pack, is the original; it comes from the pack `orig/bn-jpn.elf` comes from.
`make modules` (`tools/fetch_modules.py`) extracts them into `orig/modules/<name>.so` and
checks each sha1 against `config/modules.txt`, the registry of the modules. They are the
files as shipped: Sony's, and JustSystems' for the ATOK engine.

| module | size | notes |
|---|---:|---|
| `top` | 66,892 | the Top Menu: Small natives (`topIsp*` read `/opt0/bn/sysconf/ispch`) |
| `photo` | 23,380 | |
| `moviedata` | 58,736 | |
| `netconf` | 83,396 | |
| `download` | 150,696 | |
| `ispconf` | 153,884 | no encrypted code; `.data` holds about 64 KB of encrypted DNAS authentication data |
| `edclient` | 160,408 | encrypted DNAS2 regions (`__sce_dnas2_symbol7*`, `sceDNAS2KakinEncrypt`/`Decrypt`), used to encrypt the saved ID |
| `movie` | 246,712 | |
| `dbconvert` | 550,024 | |
| `mailer` | 771,440 | |
| `player` | 2,423,304 | the music player: one variadic native, `playerExecCommand` |
| `atokcodeconv` | 216,744 | ATOK engine (JustSystems): code conversion; no `FuncTable` |
| `atok12engine` | 2,514,788 | ATOK engine (JustSystems): the conversion engine; no `FuncTable` |

## Layout: one `modules/<name>` folder per root

The tools take their paths from `tools/target.py`; the target is chosen with
`BN_TARGET` (unset or `bn`: the main executable, with the paths it always had):

| | bn | module `<name>` |
|---|---|---|
| original | `orig/bn-jpn.elf` | `orig/modules/<name>.so` |
| symbols, statics, units | `config/*.txt` | `config/modules/<name>/*.txt` |
| sources | `src/`, `ref/` | `src/modules/<name>/`, `src/modules/libgcc/`, `ref/` |
| reference asm | `expected/asm/` | `expected/modules/<name>/asm/` |
| build output | `build/` | `build/modules/<name>/` |

A new module needs its line in `config/modules.txt` and nothing else.

## Setting up a module

```sh
make modules                        # orig/modules/*.so, sha1-checked
BN_TARGET=top make symtab           # config/modules/top/symbol_addrs.txt + static_funcs.txt
BN_TARGET=top make split            # expected/modules/top/asm/, one file per function
python3 tools/target.py --list      # the targets and whether their original is present
```

`tools/symtab.py` writes `symbol_addrs.txt` from `.dynsym`, but with the size the code
gives (the function ends where the next symbol, `.cpload` prologue or linker stub starts,
minus the padding), because some `.dynsym` sizes are wrong. Run on `bn`
(`python3 tools/symtab.py bn --check`) it gives back the committed
`config/symbol_addrs.txt` row for row, including the 9 sizes that were fixed by hand.
The unnamed functions come from `tools/statics.py` as for `bn`; its check, treating the
named functions as if they were unnamed, finds them all again (`top`: 135/135 starts and
135/135 sizes).

## Working on a module

```sh
BN_TARGET=top make check                                   # functions, data, exception tables
BN_TARGET=top make link && BN_TARGET=top make link-cmp     # build/modules/top/top.so, SHA1
BN_TARGET=top tools/match static_5FFE4440 ref/gcc-2.95.2/crtstuff.c
```

The gate is `tools/check.py` with the module's paths: units in
`config/modules/top/units.txt`, objects in `build/modules/top/check/`, results in
`build/modules/top/check.json`. Functions that have no unit yet are counted as work left
and do not fail the gate (for bn, whose universe is complete, they do). The identity
checks (GOT symbols, local GOT data, LO16 targets) run as for bn, with the module's own
`$gp` (`.got` + 0x7ff0, 0x600370F0 for `top`) and debt files in `config/modules/<name>/`.
Two things of bn are off for modules: the DNAS references and the orphan linkonce copies,
which depend on bn's linkonce region after `crtstuff_end`.

The data and exception-table gates (`tools/datacheck.py`, `tools/ehcheck.py`) run on the
module's paths too (`build/modules/<name>/datacheck.json`, `ehcheck.json`);
`config/data_regions.txt` is bn's only. A shared object differs from bn in one point: a word
of `.data` with a dynamic `R_MIPS_REL32` against a symbol of the global GOT holds only the
addend in the file (the loader adds the symbol), so it is compared as the `.dynsym` value plus
the addend, or by symbol name and addend when the symbol is undefined (`pthread_create` in
`gthr-posix`). Each block is checked at its own address: the order of the `.gnu.linkonce.d`
copies of one unit is seen only by the link.

The link (`tools/relink/link_module.sh`) is the `g++ -shared` line with ld 2.9EE: the kit's
`crti.o`, `crtbegin.o`, the gate's objects in `units.txt` order, `crtend.o`, `crtn.o`, and
`-soname <name>.so`. As for bn, the two crtstuff units are replaced by the kit's
`crtbegin.o`/`crtend.o` (our `crtstuff.c` also holds `call_gmon_start`, the `.text` of
`crti.o`). No library is linked: the modules have no `DT_NEEDED`. ld's default script for
`-shared` gives the base `0x5ffe0000` and the whole layout. After the strip, the `relvaddr`
fields and the `cpt.offset` of `.compact_rel` are copied from the original with
`tools/relink/compact_rel.py`, for the same reason as for bn (`docs/original-bytes.md`).

## State

Only `top` is decompiled. The other twelve modules are still to be decompiled: `photo`,
`moviedata`, `netconf`, `download`, `ispconf`, `edclient`, `movie`, `dbconvert`, `mailer`,
`player` and the ATOK engine (`atokcodeconv`, `atok12engine`). The scripts already name the
natives of each library after its module (`src/bnx/include/<library>.inc`, `docs/scripts.md`).

| module | named | unnamed | universe | matched | skipped | anomalies |
|---|---:|---:|---:|---:|---:|---|
| `top` | 135 | 42 (12,816 B) | 177 functions, 36,244 B | 177 (36,244 B) | 0 | 496 B of linker lazy-binding stubs at the end of `.text` |

`top`: data 41 blocks equal (`.rodata` 836, `.data` 560, `.ctors` 12, `.dtors` 12 B) plus the
`.bss`/`.sbss` sizes, `.eh_frame` 6 and `.gcc_except_table` 2 blocks equal, and the linked
`top.so` has the original SHA1 `f3ffc51f320652ea666ecc34e5b45285401c2fff`.

`top.so` has 37,056 B of `.text`, 836 B of `.rodata`, 560 B of `.data`, `.eh_frame` and
`.gcc_except_table`, base address `0x5ffe0000`. It has no `NEEDED` entry: of its 37
undefined symbols, 11 come from `bn` itself (the BB `*CPP` file functions, `amx_GetAddr`,
`amx_GetString`, `amx_SetString`, `NaviMenu::Enable`) and 26 from the libraries `bn` has
already loaded (libc, libpthread, libstdc++).

The units of `top` (`config/modules/top/units.txt`), in the order of the module's `.text`:

| unit | what | functions |
|---|---|---:|
| `ref/gcc-2.95.2/crtstuff.c` | crtbegin, shared with bn | 5 |
| `src/modules/top/top.cpp` | the Sony code: `Init`, `Exit`, the natives `topInit`, `topIspGetCount`, `topIspSync`, `topIspOpen`, `topIspClose`, `topIspGetValue` (`n_<name>` in the source) (they read the ISP settings in `/opt0/bn/sysconf/ispch` into a `vector<string>` and a `map<string,string>`) and the STL instances of those containers | 14 + 17 |
| `src/modules/libgcc/rt_opdel.cc` | libgcc `opdel.o` (`cp/new2.cc`, `-DL_op_delete`) | 1 |
| `src/modules/libgcc/libgcc_eh.c` | libgcc `_eh.o` (`libgcc2.c`, `-DL_eh`) | 27 |
| `ref/gcc-2.95.2/cp/exception.cc` | libgcc `exception.o` | 25 |
| `ref/gcc-2.95.2/frame.c` | libgcc `frame.o` | 18 |
| `ref/gcc-2.95.2/cp/tinfo.cc`, `tinfo2.cc` | libgcc `tinfo.o`, `tinfo2.o` | 27 + 41 |
| `ref/gcc-2.95.2/crtstuff_end.c` | crtend, shared with bn | 2 |

The gcc runtime is upstream gcc 2.95.2 source, unchanged, with the flags of the libgcc
build (`gcc/Makefile.in` LIBGCC2_CFLAGS, `gcc/cp/Make-lang.in`): `-O2 -g -G 0 -DIN_LIBGCC2
-D__GCC_FLOAT_NOT_NEEDED`, `-fexceptions` for `_eh` and `exception`. Two findings:
- the libgcc of `top.so` was built with POSIX threads (`--enable-threads=posix`:
  `-DHAVE_GTHR_DEFAULT`, `gthr-posix.h`, weak `pthread_*`), and with the gcc patch that
  reloads `$gp` at exception landing pads; the kit's `libgcc.a` has neither, so it is not
  the library the module was linked with and only some members can serve as an oracle;
- the assembler flags matter: `-g -g1` for the C members (what the driver passes to gas for
  `-g ... -g1`), `-g2` for the C++ ones.

The Sony unit uses the flags of bn's natives units (`-fno-exceptions -DNDEBUG`, each one
measured). It includes `app/stringutil.h`, as bn's natives units include `app/strutil.h`
(which includes it), although it does not use `StringUtil`: the header is visible only in the
order of the STL static data members in `.data`. g++ 2.95 emits them in the order their
instantiation is requested; the inline constructors of `StringUtil` request `nilRep` and
`Rep::clone` before the first function of the file requests the allocator's `_S_free_list`
and `_S_node_allocator_lock` (through `vector<string>::clear`). Without the header the
order is the one of bn (`_S_free_list` first), with it the one of `top.so` (`nilRep` first,
the lock last). `app/strutil.h` itself gives the same order but also emits its unused
`static inline` helpers.

### `what__C9exception` and the compiler

`what__C9exception` (104 B, `exception.cc`) keeps, in the original, the null check of
`typeid (*this)` (a call to `__throw_bad_typeid`). gcc 2.95.2 decides that check by reading an
uninitialized local (`nonnull` in `cp/rtti.c`, `build_x_typeid`): the Sony `cc1plus` reads 0 and
keeps it, ours read a pointer and dropped it. The kit's `libgcc.a`, built by the real compiler, has
the same function as `top.so` (25/25 rows). The toolchain carries the upstream fix that initializes
`nonnull` to 0 (`docker/patches/cp-rtti-nonnull.patch`, `docs/compiler.md`), and with it the
function matches the original.
