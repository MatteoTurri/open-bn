# The compiler, and why the code looks the way it does

The sources are compiled by the compiler that built `bn`: gcc/g++ 2.95.2-3a of the PS2 Linux Kit,
rebuilt as a cross compiler. A difference between our object and the original is always a matter of
source or flags, never of the compiler. This page describes the toolchain, the flags, the rules of
g++ 2.95 that decide the order and the shape of the output, and the source forms that exist because
of them. How a match is verified: `docs/gate.md`. Rules for comments and names: `docs/code-conventions.md`.

## 1. Toolchain

**gcc 2.95.2-3a** (`make sony-gcc`, `docker/Dockerfile.sonygcc`): the gcc 2.95.2 tarball of the kit's
SRPM with the three Sony patches in the order of its spec file (`gcc-2.95.2-frankengcc-patches`,
`gcc-2.95.2-single-float-const`, `gcc-ps2linux-1.0.0`; `docker/sonygcc/`), configured for
`--target=mipsEEel-linux`. The Sony patches touch 42 files of `gcc/`, among them the MIPS backend,
`reload1.c`, `combine.c`, `final.c`, `varasm.c` and the C++ front end; `global.c`, `local-alloc.c`,
`cse.c`, `sched.c`, `jump.c`, `flow.c`, `integrate.c`, `expr.c`, `calls.c`, `stmt.c` and `function.c`
are vanilla. Read compiler behaviour in the patched tree, not in a vanilla tarball.

Host-only adjustments, which do not change what the compiler does: `varargs.h` -> `stdarg.h` and an
`obstack.h` fix for modern hosts; bison 1.28 to regenerate `cp/parse.c` (the Sony patch deletes it);
and `cc1`/`cc1plus` compiled with `-msse2 -mfpmath=sse`, so that the `double`s gcc uses internally
(for example the priorities of `global.c:allocno_compare`) are computed in 53-bit IEEE as in the
original compiler: with the 80-bit x87 default one tie in the register allocator breaks the other way.

**binutils 2.9EE** (`make sony-binutils`, version 2.9-ee-990721): gas, ld, objdump, nm, strip and the
rest, from the kit's sources and patch. gas assembles with `-mips2 -m5900`. Demangle with
`mipsEEel-linux-nm -C` or `objdump -C`: host `c++filt` does not know the old GNU mangling.

**Headers.** The kit's glibc and kernel headers (`make kit`, `docker/kit/rootfs`) and the C++ headers
of the period (libstdc++, libio, `ginclude`, from the patched gcc tree: `make headers`). In the
original build the libstdc++ headers were under `/usr/cross/mipsEEel-linux/include/g++-3/` (the path of
`bastring.cc` is in `.rodata`).

### The one compiler patch: `exception_receiver`

`docker/patches/mips-exception-receiver.patch` adds the `exception_receiver` pattern to the MIPS
backend; it is applied after the Sony patches. Without it every exception landing pad is entered with
the `$gp` of the frame that threw (in PIC code `$gp` is call-clobbered, and nothing on the unwinding
path restores it); with it the pad reloads `$gp` from its stack slot, as the original does.

Why this is the only admissible patch, and the requirements any other would have to meet:

1. the mechanism exists in vanilla gcc 2.95.2 and three targets implement it: the semantics are those
   of `pa/pa.md` ("load the PIC register from the stack slot", gated on `flag_pic`), the form that of
   `alpha/alpha.md`; the generic hook `receive_exception_label` (`except.c`) is target-conditional and
   MIPS defines neither `exception_receiver` nor `nonlocal_goto_receiver`;
2. every other possible emitter of that `lw $28` is excluded with its reason: `$28` is a fixed
   register, gas emits the reload only inside the `jal` macro, the binary uses `.eh_frame` tables and
   not setjmp/longjmp exceptions;
3. the test has no free parameter: the offset is `current_frame_info.args_size`, the same value
   `mips.c` prints in the `.cprestore` of the prologue;
4. it changes nothing else in the whole gate.

The patch is necessary, not sufficient: how many landing pads a function has is decided by the source
(the exception regions of its objects with destructors), so "four pads where the original has one"
means the body builds too many such objects. `tools/check_toolchain.py` pins the list of patches and
their SHA-256, and checks the SSE flags of the Dockerfile; a second patch would need the same five
points proved again, not a new line in the list.

## 2. Inline asm

Inline asm is not a way to steer code generation: no empty barriers, no `"+r"` pins, no pinned
registers, no instructions transcribed as `.word`. It is allowed to name a mangled symbol
(`extern T f() asm("_11Foo$bar")`), in vendored libraries that really contain it, and in three sites:

| site | what | why it is asm |
|---|---|---|
| `src/fw/ps2sync.h` | `PS2_DMA_SYNC()`, the `sync` before handing a GIF packet to DMA | a semantic instruction that no MIPS pattern of this gcc emits; it is in the original exactly in the functions that build a packet for DMA |
| `src/gfx/bt2ipu.cpp` | the block of `_ipu_store_block` that drains the IPU output FIFO | the FIFO is read only with 128-bit `lq`/`sq`, which this gcc cannot emit; the block is assembled under `.set mips3` (its `move`s are encoded `daddu`, the only such function in the binary), so it was asm in the original |
| `src/dnas/dnas_prot.h` | the four DNAS markers (7 `asm` statements) | they never reach an object: the build replaces them with the protection scaffold (`docs/dnas.md`) |

A macro of a kit header that expands to asm is ordinary use of the platform: `PS2_GIFTAG_CLEAR_TAG`
of `linux/ps2/ee.h` (an `sq $0`) in `_ps2_gs_set_finish` (`src/fw/fw_pk.c`), where the original re-reads and masks the words
after the store, which gcc would not do after a C assignment.

## 3. Flags

The gate compiles every unit with `-O2` plus the cflags of its row in `config/units.txt`. The flags
follow the original's modules (a contiguous block of `.text` built by one makefile rule), not single
files.

| tree | cflags | asflags |
|---|---|---|
| `ref/zlib-1.1.3`, `ref/libpng-1.0.3` | `-O3` (libpng also `-funroll-loops`) | |
| `ref/jpeg-6b`, `ref/gcc-2.95.2` (crtstuff) | `-O2` | |
| `ref/sqlite-2.7.2`, `ref/libmng-1.0.3` | `-O2 -g` | `-g2` |
| `ref/tiff-3.4beta037` | `-O1 -g` | `-g2` |
| `ref/giflib-4.1.0` | `-O0 -g` | `-g2` |
| `ref/sslc-2.1.0` | `-O0 -g` | `-g2` |
| `ref/rcom-1.1.0`, `src/rcom` | `-O0 -g` or `-O2`, by module | `-g2` |
| `ref/libps2dev-1.0` | `-O2 -DDEBUG -fno-common` | |
| `ref/small-fmt6` | `-O2`, `amx.c` with `-finline-functions` | |
| `src/` | `-O2`, with the exceptions below | |

In `src/`:

- **Exceptions** are on (the g++ default) only in two modules, and every other C++ unit has
  `-fno-exceptions`: the installer/TRY and DNAS2 system-update module (0x5B8BE0-0x5E4EF0, which is also
  the only place with `-pg`, visible as the `_mcount` call through the GOT in every prologue; the C file
  `src/dnas/dnas_ar.c` is compiled with `-fexceptions -pg -g`) and the image codecs (0x62AE70-0x644D80).
- **`-O3`** on most of `src/scene`, part of `gfx`, `ui`, `ime`, `xml`, and on `bmp.cpp`,
  `database.cpp`, `parseresult.cpp`. `-O3` is `-O2 -finline-functions`.
- **`-finline-functions`** alone on thirteen XML units and `gfx/gmo.cpp`.
- **`-g`** (with `-g2` to gas) on the image codecs, the installer, part of `dnas`, `cfd`, and the C
  units at `-O0` (`src/rcom`, part of `audio`, `input`, `memcard`).
- **`-DNDEBUG`** on most of `natives`, `app`, `fs` and `script`; **`-D__NO_STRING_INLINES`** (glibc's switch for
  `bits/string2.h`) on the C units that call the string functions out of line; **`-fno-common`** where
  uninitialized globals are in `.bss` of their unit and not common symbols.

What each flag is for, and how to tell it was there:

- A flag is part of a unit only with physical evidence in the binary. `-pg` shows as `_mcount` in the
  prologues. `-finline-functions` (or `-O3`) shows when the original expands a **global** function of
  the same unit that also exists out of line: no source form reproduces that (`inline` on the callee
  makes the out-of-line copy disappear). When every expanded callee is `static`, `static inline` is
  the form and the flag is not needed. Another sign of `-O3`: template helpers that do not exist as
  separate functions in the binary, while another instantiation of the same template has them.
- `-fvtable-thunks` is a no-op: this g++ uses vtable thunks by default.
- `-g` at the compiler is not neutral. With `-g` gcc puts stabs between the end label of a function and
  the `.align` that follows; without it gas moves the label onto the aligned address, and the FDE
  `pc_range` of a function whose length is not a multiple of 8 grows by 4. In a few units (`memlib.c`)
  `-g` changes the code itself at `-O2`, and in three RCOM units (`berder.c`, `bn_modexp_word.c`,
  `bn_modexp_mont.c`) gcc's `-g` changes `.text`, so they have only the assembler's `-g2`.
- `-g2` to gas sets `mips_optimize = 1`: gas does not fill delay slots by swapping branches, and the
  `nop` stays where gcc did not fill the slot.
- In the units compiled with `-g`, every line break between statements is a line label in the `.s`,
  and at `-O0` the assembler does not move instructions across it: how statements are split over
  lines changes the code (`docs/code-conventions.md`, section 1).

## 4. Emission order

g++ 2.95 decides per function whether to write it at once or to defer it, and the order of `.text`
follows from that (`cp/decl.c:finish_function`, `toplev.c:rest_of_compilation`, `cp/decl2.c:finish_file`).

**Immediate or deferred.** A function that can be inlined is deferred and put on the `saved_inlines`
list, in order of definition; any other function is written at its definition. At `-O2` only `inline`
functions are candidates; with `-finline-functions` (`-O3`) every function under the size threshold is.
Artificial functions (`__static_initialization_and_destruction_0`, `__tf*`, implicit constructors and
destructors) are always treated as `-finline-functions`.

**The threshold** (`integrate.c:function_cannot_inline_p`). With `inline`: `max_insns + 8 * nargs`;
without: `8 * (8 + nargs)` RTL insns. Two tests: first `get_max_uid() > 3 * max_insns` (every numbered
rtx counts: notes, labels, barriers, deleted insns), then the count of real insns from the first
non-parameter insn. Hence:

- source shapes with the same machine code can fall on either side: an early `return` instead of a
  nested `if`, a `switch` (more RTL than the equivalent `if`/`else`) instead of an `if`, an accessor
  call instead of a direct field access (the inlined body, with its copy of `this` and its return
  label, counts in the caller even if nothing of it survives);
- a stale front-end state: `current_function_return_value` is set by every constructor and by a
  top-level `return variable;`, and is cleared only by a user label or a `case`. When it is clear at
  the end of a normal function, `finish_function` adds two jumps, two barriers and a label, so the size
  of a function depends on what precedes it in the file.

**End of the unit** (`finish_file`), in rounds: template instances that are too large come out at once;
vtables; `__static_initialization_and_destruction_0`, written right away; the artificial functions in
use; then `saved_inlines` in order, writing what is public and not comdat (`#pragma implementation`
members) or referenced out of line, round after round. `_GLOBAL_$I$`/`$D$` always come last. A deferred
`static` function that is never called out of line is never written.

So a C++ unit at `-O3` reads: [too large, in definition order] [static initialization] [deferred, in
definition order, by rounds] [`_GLOBAL_$I$`]. The order of the definitions in the source is the order
of `.text`, and it is deliberate everywhere.

**String constants** are written to `.rodata` when the RTL of a function is generated, and the RTL of
an inline function is generated at its definition. Moving an inline function moves its literals in
`.rodata`, whether it is called or not.

**Comdat copies.** An inline function or a template instance without an effective `#pragma interface`
goes to `.gnu.linkonce.t.<name>` as a weak symbol, at its place in `saved_inlines`. The linker keeps the
first copy per name in link order, at the end of `.text`, in per-object blocks in the order of each
`.s`: the neighbours of a copy say which unit emitted it, and the call form inside it (local page
against CALL16) tells between two candidate units. A kept copy whose name has a strong definition
elsewhere stays in the image without a name (`docs/gate.md`, orphan linkonce copies).

**A call in the original does not mean "callee defined after the caller".** The inliner copies the
calls of a saved body as they were: if the caller expands a function W, W's calls stay calls, also to
functions defined after W.

## 5. Headers and declarations

Every class, struct and union has one declaration, in a header next to the file that implements it.
What decides the shape of those headers:

- **Declarations emit no code by themselves**: a full class with more members than a unit uses, STL
  members, nested types with inline constructors leave the `.text` of a unit that uses only part of it
  unchanged. Unused inline members are not written, and including `<map>` or `<vector>` instantiates
  nothing. Two effects of inline bodies do reach the object, though.
- **Their literals are emitted** (section 4): an inline function of a header that holds a string
  writes it into the `.rodata` of every unit that includes the header. `<vector>`/`<map>` bring
  `"bad_alloc"` (from `<new>`); the inline `operator<<` of `gfx/vec.h` writes `" "` and `"\n"`. A header
  with literal-bearing inline code can be included only by units whose original `.rodata` holds those
  literals: `vec.h` keeps its `operator<<` under `#ifdef _IOSTREAM_H`, and the units that have the
  literals include `<iostream.h>` before it.
- **Their template callees are instantiated**: the body of an inline function is expanded when its
  class is complete, so even if it is never called every includer emits linkonce copies of the
  templates it calls; the linker discards them, but they count in `local_gotno`
  (`docs/linking.md`). Example: the inline constructor of `SceneWindow::CreateArg`
  (`script/scenewindow_createarg.h`, included by `scenewindow.h`) instantiates `map` copies in every
  unit that includes `scenewindow.h`.
- **Inline in the class or declared only.** A member with a body in the class is expanded by its
  callers; a declared one is called. The canonical form is `#pragma interface` in the header and
  `#pragma implementation` in the `.cpp`: bodies in the class, callers expand them, and the
  implementing unit writes a global copy of each one in class order, where the class definition ends.
  `#pragma implementation` applies to every class of the header, so an interface header holds only the
  classes its `.cpp` implements. Where the out-of-line copies must come in another order, the copies
  are written in the `.cpp` under a guard (`VECMATH_IMPLEMENTATION` in `gfx/vec.h`).
- **What `#pragma interface` does to the other units**: they still expand inline bodies, but never write
  an out-of-line copy of one (they call the implementing unit's global copy instead of a linkonce
  copy), and never write the vtable or typeinfo of the header's classes.
- **Where the class definition sits in the implementing unit** matters: the typeinfo function and the
  in-class bodies enter the deferred queue when the class is complete, so the `#include` goes where the
  definition was, not necessarily at the top.
- **Vtables and typeinfo** without `#pragma interface` go to the unit that defines the first virtual
  function that is neither inline nor pure. The slot order of each vtable is that of the original
  (the `__vt_*` objects are in `.dynsym`).
- **Exceptions**: in a unit with exceptions, whether a class has a destructor decides whether its local
  objects get cleanup regions, so adding or removing one changes code, `.eh_frame` and
  `.gcc_except_table`; the third pass of the gate sees the tables.
- **Position of an include**: inline functions of a header can change the state that a later function
  sees (section 4). `src/app/bnexec.cpp` includes `thread/thposixthread.h` near the end of the file,
  right before the native table; compiled before `n_err10030`, its inline constructor changes the
  register allocation of that function (`docs/open-questions.md`).

### Prototypes declared by hand

Every function has one prototype, in the header of the file that defines it, or the system header.
The exceptions:

| file | functions | reason |
|---|---|---|
| `src/lib/resuid.h` | `getresuid`, `setresuid` | glibc exports them but no kit header declares them |
| `src/chloe/chloe.h` | `sysmips` | left unprototyped: the `MIPS_TASKFLAGS` call passes three arguments, `<sys/sysmips.h>` declares four `int`s |
| `src/gfx/pltm.c` | `PLTM_SetFontPath` | `__inline__` forward declaration, called before its `__inline__` definition |
| `src/gfx/pltm.c` | `_sceCccIsValidUTF16`, `_sceCccDecodeUTF16` | global `__inline__` helpers: the place of the first declaration decides where they are emitted |
| `src/gfx/freetype2.h` | `FT_*` | no FreeType 2 header in the kit or in `ref/` |
| `src/image/outputdatafiltergif.cpp` | `EGifOpen` | absent from giflib 4.1.0's `gif_lib.h` |
| `src/script/script.cpp` | `amx_NumNatives`, `amx_NumLibraries` | defined in `amx.c`, absent from this revision of `amx.h` |
| `src/lib/pure_virtual.c` | `__write`, `__terminate` | upstream copy (`libgcc2.c`) |
| `ref/sslc-2.1.0` | `fcrypt_body`, `passwd_read_pw_string`, `fd_*`, the `*_block_x86` functions, `bn_add_words`/`bn_expand2` | upstream SSLeay |
| `src/dnas/dnas_prot.h` | the three shims, `extern void f();` | called with the arguments of the scaffold (`docs/dnas.md`) |

## 6. Line numbers, paths and dates in the binary

- Two source lines are pinned to their line number, between `/* clang-format off */` markers:
  `#line 3139` before `assert(movelen >= 0)` in `src/ui/simpletextedit.cpp` (`AC_NewLine`), and
  `#line 324` before an `ERR_put_error(..., __LINE__)` in `src/rcom/sio_ioctl.c`. The vendored
  libraries carry their own `#line` directives (SSL-C's error macros use `__LINE__`), and `ref/` is not
  reformatted.
- `__FILE__` is the command-line path of the original build, reproduced by `config/unit_paths.txt`
  (`docs/gate.md`). The original name of `src/ui/simpletextedit.cpp` was `objSimpleTextEdit.cpp`.
- `__DATE__`/`__TIME__` of the giflib units are those of Sony's build (`config/unit_times.txt`).
- Default arguments of C++ declarations end up in `__PRETTY_FUNCTION__` strings, and the locals named
  inside an `assert` in its text: they are part of `.rodata`.

## 7. Deliberate source shapes

These forms are in the source because the binary requires them; each one is plain C or C++ that a
programmer could have written.

- `static int x = 0;` and `static int x;` differ: the explicit zero goes to `.data`, the other to
  `.bss`. A `.data` object that is all zeros in the original is an explicit `= 0`.
- `goto loop;` with a label instead of `for (;;)` where the original keeps the exit test at the top:
  gcc rotates a `for` loop and moves the test to the bottom.
- Parentheses that group an address computation (`sig + ((state << 4) + 8)`) where the original reuses
  the shifted value; a named temporary would take its own register.
- `if (n == 0 || n > max)` with `max` a variable: with a constant, gcc folds the two tests into one
  range check.
- An early `return` or a `switch` in place of an equivalent `if`, or an accessor call in place of a
  field access, where the inliner threshold of section 4 decides immediate or deferred emission.
- `2.0f * x * y` in place of a named `x2`: `fold` turns `x * 2` into `x + x`, which changes the insn
  count, not the code.
- An explicit `return;` at the end of a constructor, and `return new T(...)` instead of a temporary,
  where the original's exception region ends there.
- `inline` on a member defined in the `.cpp` where the original writes it after the typeinfo function
  (deferred), and the order of definitions everywhere (section 4).
- Two copies of a body (a C helper `gm_*(gm_packet *pk, ...)` and the C++ member) where the original
  has a compiled copy, an expanded copy and the helper's literals early in `.rodata`.
- Implicit special members (no user-declared copy constructor or destructor) where the original
  synthesizes them in the using unit.
- Variables, buffers and fields that are never read but occupy their place in the layout, marked
  `/* unused, but part of the layout */`.
- Debug and assertion macros that compile to nothing or to a release check (`THFO_ASSERT`, `XML_TRACE`,
  `DNAS2_TRACE`, `BN_TRACE` of `src/natives/bntrace.h`, `JACKET_CHECK`) and the scope guard
  `thScopeGuard`: the reason for each is in `docs/open-questions.md`.

## 8. Forms kept because the plain one changes the code

| where | form | the plain form |
|---|---|---|
| RCOM op interpreter (`src/rcom/op*.c`, `ref/rcom-1.1.0/op_load.c`) | registers read through `OP_REG` (`ref/rcom-1.1.0/op_ctx.h`) | indexing the register array changes all of them |
| `BBgetMCInfo` (`src/fs/bbfile_dir.cpp`) | the cache entry addressed as `(char *)g_mcInfoCache + offset` | `&g_mcInfoCache[port][slot]` and a flat index both change it |
| `myPad` (`src/input/mypad.cpp`) | `p = &m_bufStat; p[4]...` in `pad_button_read` and `_InitButtonValues` | a member array index changes both |
| `gm_graph::GetMatrix`, `write_matrices` | pointer arithmetic on the matrix stack | `&fMatrixStack[mode]`, `fMatrixStack[i + 4]` reorder the address computation |
| `fwGetXYOffset_SingleBuffer` | a pointer to `gs_env.originX` | plain fields (its multi-buffer twin uses them) |
| `fwGsBlk*` | word copies of the kick-base environments | copying the typed packets uses 64-bit moves |
| `n_imgDecStart`, `n_imgDecStringStart` | the first read of the script is an explicit `amx_GetUserData` into a local | the inline accessor takes one more saved register |
| `ref/rcom-1.1.0` `kit_integer_items.c`, `bsafe_ai_cbc_iv8.c`, `bn_me_word.c` | offsets into layout-parallel structures and allocations | |

Byte parsing (icon and `.ico` files, the decrypted DNAS records, the Chloe header, VIF/DMA packet
building, GMO chunks, BER buffers) stays as pointer work because that is what it is.
