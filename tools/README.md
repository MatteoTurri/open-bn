# tools/

All tools run from the repository root. Those marked [container] run inside the container
(`docker run --rm --platform linux/amd64 -v "$PWD:/work" open-bn-env python3 ...`)
or through the Makefile target shown.

## Gate (`make check`)

- **`check.py`** [container] `make check`: The decompilation gate: compile every unit and check the matches.
  - `Runs in the container: python3 tools/check.py [--verbose] [-jN]`
- **`datacheck.py`** [container] `make check`: Data gate: .rodata, .data, .ctors/.dtors (and the vtables) compared byte by byte.
  - `python3 tools/datacheck.py [--verbose] [--unit SRC]   (--unit writes build/datacheck-unit-<SRC>.json)`
- **`ehcheck.py`** [container] `make check`: Gate of .eh_frame and .gcc_except_table: the exception tables, byte by byte.
  - `python3 tools/ehcheck.py [--verbose] [--unit SRC]     (UNITS=file for the scoped gate)`
- **`dataregions.py`**: Data regions that no compiler produced (config/data_regions.txt).
  - `python3 tools/dataregions.py           # per-class summary + consistency + overlapping ok blocks`
- **`gotident.py`** [container] `make got-debt`: Identity of the GOT symbols of the green functions.
  - `python3 tools/gotident.py --write-debt [--add]    # regenerate config/got_debt.txt`
- **`got_callees.py`**: Resolve the GOT-direct call graph (%call16, `lw $t9, OFF($gp)`) of one or
  - `python3 tools/got_callees.py SYMBOL [SYMBOL...]`
- **`normalize_asm.py`**: Normalize an objdump listing for the matching diff.
- **`srcpath.py`**: Compile a unit with the original path (config/unit_paths.txt).
- **`fixtime.py`**: Run a command making it see a fixed time.
  - `python3 tools/fixtime.py EPOCH -- command ...     (EPOCH in seconds, read as UTC: TZ=UTC)`
- **`extract_assets.py`**: Extract the assets from the user's original ELF (never committed: docs/original-bytes.md).
  - `python3 tools/extract_assets.py           # extract (rewrites only the changed files)`
- **`check_toolchain.py`**: Guard for the only compiler patch: the compiler is not patched.
  - `Usage: python3 tools/check_toolchain.py`
- **`objdiff.py`**: Diff of the gate objects against a snapshot: what `make check` does not see but the link does.
  - `python3 tools/objdiff.py snapshot [UNITS_FILE]   # copy build/check/<unit>/out.o into build/objdiff/snapshot/`

## Universe, statics and DNAS

- **`universe.py`**: The one definition of the binary's function universe.
  - ``python3 tools/universe.py` prints the breakdown.`
- **`statics.py`**: The nameless functions of .text: the `static` functions (and the crt) missing from .dynsym.
  - `python3 tools/statics.py --validate   redo the measurement on the known functions`
- **`selfmod.py`**: The self-modifying DNAS functions (generated lists).
  - ``python3 tools/dnas2_decrypt.py` and `python3 tools/dnas1_decrypt.py` write the`
- **`dnas_ref.py`** [container] `make check`: Reference of the self-modifying DNAS functions (163: 89 named + 74 static).
  - `From the shell (in the container):  python3 tools/dnas_ref.py --verify   (pattern anomalies)`
- **`dnas_subst.py`**: Build step that substitutes the DNAS scaffold (docs/dnas.md).
  - `Usage: python3 tools/dnas_subst.py IN.s OUT.s   (exits 1 if a substitution fails;`
- **`dnas1_crypt.py`**: The DNAS1 self-modifying transformation (bn-jpn.elf).
  - `python3 tools/dnas1_crypt.py                    # self-test`
- **`dnas1_decrypt.py`**: DNAS1: survey + full static decryption, with exclusion lists.
  - `Usage: python3 tools/dnas1_decrypt.py [--verify]`
- **`dnas2_crypt.py`**: Reconstruction of the self-modifying DNAS2 transform (bn-jpn.elf).
  - `Usage: python3 tools/dnas2_crypt.py ADDR NWORDS CODE   (decrypts and dumps)`
- **`dnas2_decrypt.py`**: DNAS2: survey + full static decryption, with exclusion lists.
  - `Usage: python3 tools/dnas2_decrypt.py`

## One function and diagnostics

- **`match`**: Host wrapper: runs tools/match.sh inside the open-bn-env container.
- **`match.sh`** [container] `tools/match`: match.sh <symbol> <source> [extra compiler flags]
- **`compare_asm.py`**: Compare two normalized listings: orig (linked) vs ours (.o with RELOC).
- **`datacmp.py`**: datacmp.py UNIT SEC [--ctx N] [--words] [--all] [--addr HEX]
- **`split.py`** [container] `make check`: Per-function split of the original binary (replaces splat for this target).
  - `Runs in the container: python3 tools/split.py`
- **`unitedit.py`**: Targeted edit of one row of config/units.txt, without rewriting the file.
  - `python3 tools/unitedit.py src/foo.cpp --skip-del SYM1,SYM2      # matched: remove them`

## Link (`make link`, `make link-cmp`; docs/linking.md)

- **`layoutcheck.py`** [container] `make link`: layoutcheck.py -- is the .text of each of our .o files ready for the link? (measurement, not a gate)
  - `python3 tools/layoutcheck.py                  # all units: summary, build/layoutcheck.json`
- **`relink/fetch.py`**: Download and extract the third-party files the link needs, into build/relink/dl/.
  - `Usage: python3 tools/relink/fetch.py        (from the repo root; `make link` calls it)`
- **`relink/make_libs.py`**: Prepare build/relink/libs/: the shared libraries of the link.
  - `Usage: python3 tools/relink/make_libs.py [--kit-libc]   (repository root, after fetch.py)`
- **`relink/link_order.py`**: Link order of the .o files: from the original's addresses to link_order.txt.
  - `Usage: python3 tools/relink/link_order.py   (repository root; called by `make link`)`
- **`relink/make_blobs.py`**: Non-compiled bytes for the link: blobs extracted from orig/bn-jpn.elf at build time.
  - `Usage: python3 tools/relink/make_blobs.py   (repository root, in the open-bn-env container: it`
- **`relink/missing_syms.py`**: Symbols our .o files use that no .o / library of the link defines.
  - `Usage: python3 tools/relink/missing_syms.py   (repository root, after link_order.py,`
- **`relink/partial_r.py`**: Final list of objects for ld, with the `ld -r` group checked.
  - `Usage: python3 tools/relink/partial_r.py   (called by tools/relink/link.sh, before ld)`
- **`relink/link.sh`** [container] `make link`: Link bn with ld 2.9EE, strip, DNAS protection and the post-link .compact_rel step.
- **`relink/compact_rel.py`**: Post-link .compact_rel step: copy the entries' `relvaddr` and the header's `cpt.offset` from the original.
  - `Usage: python3 tools/relink/compact_rel.py IN.elf OUT.elf [--orig orig/bn-jpn.elf]`
- **`relink/dnas_protect.py`**: Post-link step of the DNAS protection: descriptors and re-encryption of the regions.
  - `Usage: python3 tools/relink/dnas_protect.py IN.elf OUT.elf`
- **`relink/cmp.py`**: Section-by-section comparison between the original and a linked ELF.
  - `Usage: python3 tools/relink/cmp.py [ORIG] LINKED [--funcs N] [--json OUT] [--mask-gp]`
- **`relink/link_cmp.sh`** [container] `make link-cmp`: Body of `make link-cmp` (docs/linking.md): per-section comparison (tools/relink/cmp.py,
- **`relink/paths.py`**: Paths and constants shared by the link pipeline (docs/linking.md).
  - `them with `eval "$(python3 tools/relink/paths.py --sh)"` (quoted shell assignments).`

## Formatter (`make format`)

- **`format.py`** [container] `make format`: Format the sources of src/ with clang-format 23.1.5, the only version accepted.
  - `python3 tools/format.py [--check] [--version] [FILE ...]`
- **`hooks/pre-commit`**: Optional pre-commit hook: refuses a commit whose staged src/ files are not formatted.

The container has the pinned clang-format (docker/Dockerfile), so `make format` needs nothing else.
On the host (editor format-on-save, the hook), `make venv` installs it into `.venv` (gitignored), like a
lockfile-pinned dev dependency; `tools/format.py` picks the first of `CLANG_FORMAT`, `.venv/bin/clang-format`
and `clang-format` in PATH that has the pinned version, so the system clang-format does not matter:

```sh
make venv                                             # once, and again after a version bump
python3 tools/format.py --check                       # what make check does first, on the host
.venv/bin/clang-format                                # the binary to give the editor
```

To bump the version: `VERSION` in `tools/format.py` and the pin in `docker/Dockerfile`, then `make venv`,
`make env`, `make format`.

Another version of clang-format formats differently: `tools/format.py` refuses it (exit 2).

## Generators of tracked sources

- **`gen_cloud_noise.py`**: Generate src/scene/cloud_noise.inc: the initializer of `s_noise` (src/scene/cloud.cpp).
  - `python3 tools/gen_cloud_noise.py            # rewrite src/scene/cloud_noise.inc`
- **`gen_sqlite.sh`**: Regenerates the generated source files of SQLite 2.7.2 in ref/sqlite-2.7.2/gen/

## Environment

- **`fetch_kit.py`**: Fetches the pieces of the PS2 Linux Kit (disc 2) the decomp needs and builds
  - `Usage: python3 tools/fetch_kit.py [--rpm-glob GLOB ...] [--purge-iso]`
- **`setup_cxx_headers.sh`**: Prepares docker/toolchain/include/ to compile C++ with the Sony g++:
