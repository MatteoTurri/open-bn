# Open questions

What is not known, or was decided without proof from the binary. The code rebuilds the original byte
for byte (`make check`, `make link-cmp`); the points below are about the source behind those bytes. To-do
items for the code go here, not in comments (`docs/code-conventions.md`).

## Constructs accepted without a proof of their exact form

The rule for the source is that someone at Sony could plausibly have written it, for a reason. Nine
functions reach their bytes only with a construct whose effect is visible in the binary but whose exact
form the binary cannot prove. Each one has a plausible reason, and each is open to a better form:

| function | file | construct | reason |
|---|---|---|---|
| `ExtJacket` | `src/install/tryinstaller.cpp` | `JACKET_CHECK(e)`, a `goto out` on a negative result | the only call whose negative error is propagated as is |
| `GetUntil` | `src/xml/xml_utf8stream.cpp` | `XML_TRACE` of the marker, empty in the release build | a debug trace |
| `__sce_dnas2_symbol65037` | `src/dnas/dnas2read.c` | `DNAS2_TRACE` of the record length, and a second read of `h->len` (the second read is in the binary) | a debug trace |
| `amxFormatSpec` (`static_00517270`) | `src/natives/natives_str.cpp` | `BN_TRACE`/`BN_TRACE_FMT` of `src/natives/bntrace.h` | debug traces of the Small natives |
| `n_domGetData` (`static_004F24E8`) | `src/natives/natives_dom.cpp` | `BN_TRACE` | as above |
| `getproperty` | `src/fs/thfileoperation.cpp` | `THFO_ASSERT`, a release assert that only type-checks its expression | checks on names read from memory cards |
| `fo_svfolder_copy` | `src/fs/thfileoperation.cpp` | `THFO_ASSERT` | as above |
| `data_load` | `src/fs/thloadicon.cpp` | `thScopeGuard` (`src/thread/thscopeguard.h`), a local guard object | a scope guard |
| `decode` | `src/image/thimagedecoder.cpp` | `thScopeGuard` | as above |

## A position that only the result decides

`src/app/bnexec.cpp` includes `thread/thposixthread.h` near the end of the file, right before the
native table. With the include at the top, the inline constructor of `thPosixThread` is compiled before
`n_err10030`, and that function swaps `s7` and `s8`. Nothing in the original says where the include was,
and the dependence of `n_err10030` on what was compiled before it is not explained. Hypothesis: gcc's
cse hashes depend on pointer values, so the allocation could even depend on the host that runs the
compiler. A change to the headers that `bnexec.cpp` includes can break this function.

## Layout choices the binary does not decide

- Objects that are never referenced but occupy their place: `s_cdName[16]` of
  `src/app/bncdsearch.cpp` (16 bytes of `.bss` that the code never touches) and `g_appTabLock` of
  `src/app/bnapp.cpp`. Name and type are guesses.
- The objects merged with `ld -r` before the link (`config/relink_partial_r.txt`): the original's
  `.fini` proves that some group was, not which one (the Chloe files are used).
- The debug print of `src/app/biff.cpp` behind `if (BIFF_DEBUG)`: the binary has the instance of
  `operator<<` it needs, but no caller of it; the form of the switch is a guess.
- Single-file flags that were not checked against the physical-evidence rule of `docs/compiler.md`
  (section 3): `-fno-strength-reduce` of `src/gfx/bt2ipu.cpp`, `-fno-builtin` of `src/ime/textimsub.c`,
  `src/lib/scepfsd.c` and `src/natives/natives_img.cpp`, `-mmemcpy` of `src/ime/skbmain.c`.

## Names

- Many locals, parameters, globals and fields still have names from the decompilation. Every name should
  say what the thing is or does; the names of functions we chose (statics without a name in the binary,
  helpers) need checking against their body, callers and tables. Names that come from the binary
  (`.dynsym`, the native tables, mangled names) stay.
- 51 distinct `unk_XX` fields remain, spread over most folders of `src/`: their meaning is not known.
- DNAS2 data whose role is not known: `dnas2_dec_a_keys` and `dnas2_dec_b_keys` (`dnas2m11_slots.c`,
  `dnas2m12_slots.c`: keys of the two slot decoders, with no other visible difference between them),
  `dnas2_blk_marker` (`dnas2m12_blocks.c`: 32 bytes compared with the block at `+0xFC80`) and
  `dnas2_keyslots` (`dnas2keyload.c`).

## Chloe

`docs/dnas.md` describes what is known. Open:

- The two encrypted blobs (stage 1, 1,152 B; stage 2, 42,848 B) are copied from the original. The goal
  is to decompile them and have the build encrypt them, as for the DNAS regions.
- About 28 KB in the middle of stage 2 have an entropy of 7.95 bits per byte: compressed, or encrypted a
  second time; not decoded.
- `/dev/akmem` is known (`docs/dnas.md`), but the PSBBN kernel's own version of the driver is not
  public: the layout of the parameter block stage 1 builds, with the `"PS2REBOO"` signature, is
  inferred from the code that builds it only.
- Hypothesis for the whole: when the user launches PS2 content, `bn` loads the program from the disk and,
  through `/dev/akmem` and a reboot, installs a small native boot in physical memory that restarts the EE
  outside Linux and runs that program with the downloads. The program read by `ChloeBanana` from `/dev/hda` is probably
  the boot of the system partition, and the other chain (`"DVD Player"`) the boot of the native DVD
  player. Neither is proved.

## VU microcode

The two VU1 programs that `gm_graph::Init` loads (`gm_vu1_prog0`/`1`, 31,648 B) are assets included from
`assets/`. They were produced by the VU assembler (`dvp-as`), not by gcc. Open: whether to write them as
VU source and assemble them in the build, which needs a VU assembler in the toolchain, or keep them as
assets. Likewise the ATOK IME tables (hex-digit strings in `.data`) are treated as an asset; that choice
can be revisited.
