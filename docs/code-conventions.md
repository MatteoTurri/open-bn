# Code conventions

Conventions for the source in `src/`: comments, identifiers and formatting. The model is zeldaret/oot and
n64decomp/sm64: code readable by a human, comments in English, no decompilation-process commentary in the code.
Behaviour comments are calibrated: present where they help, absent on the obvious, short but clear. Constructs required by
the binary stay as they are.

Scope: `src/`. Our comments in `ref/` follow their own rules (953 `#line` in 129 files: moving a line is high risk).

## 1. What is never touched

- string and character literals (they live in `.rodata`, typos included);
- preprocessor directives: `#include`, `#pragma` (interface/implementation decide where vtables and inline copies
  go), `#line`, `#define`, `#if`;
- line breaking and indentation of the code are not done by hand: the formatter decides them (`make format`,
  section 7);
- the lines pinned to their line number are two sites, enclosed between `/* clang-format off */` and
  `/* clang-format on */`: `#line 3139` + `assert(movelen >= 0);` in `src/ui/simpletextedit.cpp`, and `#line 324` +
  `ERR_put_error(..., __LINE__)` in `src/rcom/sio_ioctl.c`. Moving a line after the `#line` changes the code of
  `AC_NewLine` and `SIO_socket_ioctl`; shifting every file of `src/` by a few lines leaves the code of every unit
  identical and changes only `.mdebug` of the `-g` units. The `#line` directives of `src/lib/libc_stat.c` and
  `libc_lstat.c` do not reach the object file (identical objects with the line moved): they only affect compiler
  messages;
- in the units compiled with `-g`, how the statements are split across lines matters: every new line is a line label
  in the `.s`, and at `-O0` the assembler does not move instructions across it (the delay-slot `nop`s change). In
  `ref/rcom-1.1.0`, breaking `bn_expand2(...); for (...)` onto two lines changed the `.text` of three `-O0 -g` units.
  `src/` has 95 `-g` units (14 at `-O0`): the current formatting leaves them identical; for any future change
  `make check` decides, and the remedy is a `clang-format off` region around the statement, never a different
  layout of the file.

## 2. What is removed: the process

Keep the *what*, remove the *how we know*. A comment that states a fact about the program together with its evidence is
reduced to the fact. Removed:

- dates, match status, evidence tags in brackets ("measured", "source", "hypothesis");
- addresses in the original binary (`@0x5e1f68`), GOT offsets, gp, `.text` sizes, `.text` order, TU boundaries;
- explanations of why a form matches (gcc, stack slots, registers, scheduling, combine, cse), comparisons with "the
  original", forms tried and discarded, MIPS mnemonics;
- the history of the file and of our corrected mistakes ("correction", "it used to be...", "merged on...");
- Orphan comments, describing code that is not in this file (after TU splits): they are deleted (never moved to
  another source file);
- paths of tools and documents (`tools/`, `docs/`, `expected/`, `units.txt`).

Process text is deleted, not moved elsewhere. A mixed comment (fact + process) keeps the fact, rewritten. Questions
that are still open go into `docs/open-questions.md`.

## 3. What stays: behaviour comments

- Only where needed: the *why*, or a meaning the code does not state by itself (record format, units, protocol, side
  effect, magic values, invariants).
- No comments on the obvious, no paraphrase of the line below.
- Short: one line is the norm; a paragraph only at the top of a file or of a really complex function.
- English, ASCII only, `/* ... */`. Existing `//` comments may stay.
- Citing another source file (`bbfile.cpp`, `bbfile.h`) in a behaviour comment is allowed; paths of tools and
  documents are not.
- Function summaries written from scratch are checked against the code (return values, conditions), not assumed.

### Form

- File header, mandatory, 1-4 lines; no file name, addresses, TU boundaries or flags:
  ```
  /*
   * <What this file contains, one sentence.>
   * <Optional: role in the program / key invariant.>
   */
  ```
- A function comment only if the name and signature are not enough, at the definition (`.c`/`.cpp`), never
  duplicated in the header. It sits on the line immediately above the signature (no blank lines or `extern` blocks in
  between: if declarations are in between, move the comment, not the code). Third person ("Returns", "Reads"),
  without repeating the function name and without a leading "Name:".
- For opaque names (`__*symbol*`, `_sce_*`, `static_*`) "name and signature are not enough" always holds: one line
  per function, or a symbol -> role list in the section comment.
- A class/struct comment goes where the class is declared. The header documents the class; the header of the
  `.cpp` says what that file defines. Never the same text in both.
- Field comments state meaning, unit or encoding. "Set by X", "read by X", "reset to N" only if the meaning is not
  known. Two structs describing the same memory are documented consistently.
- Field offsets, oot style: `/* 0x1C */ type name;` (upper-case hex), and `/* size = 0x6C */` after the `}` when the
  size is known. Existing spellings are converted with a script, not by hand.
- Trailing comments: only unit, meaning or value, never provenance.
- No comments explaining why a datum lives in a given section or why the same object has two access paths:
  `src/README.md` covers it.
- Commented-out code and `#if 0` blocks that are discarded attempts are deleted.
- TODOs are not allowed in the code: they go into `docs/open-questions.md`.
- No per-site "guessed name" marker: `src/README.md` says it once.

### FourCC

In comments a FourCC is written as the bytes are in memory (as it appears in files and dumps, and the form that
spells the name: "TYPE", "SMAL"), in double quotes: `/* "TYPE" */`. Single quotes are reserved for the C
multi-character literal, which on this little-endian target has the opposite order (`case 'LAMS':` = "SMAL" in
memory).

## 4. Markers (the only ones allowed)

- `/* NOTE: bug: <one line> */` for a defect of the original program that is reproduced (like oot's `//! @bug`). It
  goes on every site with the same defect verified in the code, except several sites in the same function a few lines
  apart: one marker, on the first. Every new `NOTE: bug:` is checked against the code and its context (a global in
  `.bss` is zero-initialized, fields filled in by `mktime`, ...).
- Fixed phrases, trailing the line of the construct, one per line (no marker above a pair of declarations), on the
  site of the construct (the use of a macro, the declaration), not on the macro definition:
  - `/* unused, but part of the layout */`: a variable, buffer or field that is really never read (a temporary that
    is read is covered by `src/README.md`);
  - `/* redundant as written */`: a dead store or branch, a redundant test, a construct with no visible semantic
    reason;
  - `/* the line number is part of the binary */`: a `#line`.
- The pair `/* clang-format off */` ... `/* clang-format on */`, only for the regions listed in section 7.
- Where a construct has a name that says its purpose (disabled trace, release assert), a normal comment is enough.
- The marker never names the mechanism (stack, registers, scheduling, gcc, match, original).
- Order of declarations/definitions/includes, divergent local declarations, types and signedness, temporaries, shape
  of branches: no marker. They are covered once and for all by the sentence in `src/README.md`:
  > This code reproduces the original binary byte for byte: the order of declarations, definitions and includes,
  > local re-declarations, types and temporaries are deliberate. Run `make check` and `make link-cmp` after any change.

## 5. Identifiers

Renamable with no effect on the binary: parameters, locals, labels, fields, file-scope `static`s, macros not used
with `#`/`##`, typedefs (except one that names an anonymous struct in C++).
Never: names in `.dynsym` (all of them, imports included: `mipsEEel-linux-nm -D orig/bn-jpn.elf`; `config/symbol_addrs.txt` lists only the defined ones); locals inside `assert(...)`; default values in C++ declarations (they end up in
`__PRETTY_FUNCTION__`); in C++, any class/struct/union/enum/namespace name used in a global symbol (also as a
template argument) or polymorphic. When in doubt, do not rename. A name declared in a header is renamed together
with all its uses in the same commit. Renames go in separate commits, with the name map.

## 6. Verification

1. Code-line invariant: with comments removed and whitespace normalized, the sequence of code lines of every touched
   file is identical to HEAD's; for the files of section 1 the line numbers too.
2. Comment-only check: no process words (match, matching, original, orig, LCS, codegen, gcc,
   GOT, stack slot, register alloc, scheduling, measured, ...), no non-ASCII characters, no apostrophe accents
   (`e' `), no `0x` with 5+ digits or `@0x`, no MIPS mnemonic with operands, no bracketed evidence tags (check mark,
   `[~`), no tool/doc paths.
3. Fixed-phrase markers only from the closed list of section 4.
4. Gate: `make check` with `build/check.json` equal to the baseline address by address (not only in the totals), and
   `make link && make link-cmp` with an identical SHA1.

Non-blocking review signals: comment density of a file > 20%, a comment block inside a function > 8 lines, a file
header > 6 lines. The review question: "does every comment say something the code does not? is there process?".

## 7. Formatter

`src/` is formatted with clang-format **23.1.5** (`.clang-format` at the root): `make format` rewrites, the
first step of `make check` checks. `tools/format.py` refuses any other version (21 and 22
give a different result on 28-30 files). On the host: `make venv` (`tools/README.md`).
`ref/` is not formatted (`ref/.clang-format`: `DisableFormat: true`): upstream libraries stay comparable with their
tarballs, sslc is pinned to its lines (234 `#line`, `__LINE__` in the `XXXerr` macros), rcom is compiled
with `-g` (section 1).

Rules:
- Style: the one the code already had (BSD KNF): 8-column tabs, +4 continuation, brace on its own line only for
  functions, `} else {`, `T *p`, 120 columns, one statement per line. The formatter does not touch the text of
  comments nor the position of trailing comments, the include order, macro bodies, literals.
- `AlignAfterOpenBracket: DontAlign` is deliberate: continuation lines are indented, not aligned to the parenthesis.
  It also turns off the column layout of brace lists: a multi-line table with a trailing comma is laid out one
  element per line.
- Multi-line file-scope tables: with a comma after the last element (C89 and g++ 2.95 accept it; the objects do not
  change). Tables the formatter blows up (more than three times the lines) or repacks losing their rows (matrices,
  DES tables) are enclosed between `/* clang-format off */` and `/* clang-format on */` (47 tables in 22 files);
  one-element-per-line lists are left to the formatter.
- Never a comma after an asset `#include` inside an initializer (the `.inc` file supplies the comma).
- The comment of a table that includes an asset goes above the declaration, not after `= {` on the same line: there
  clang-format is not idempotent (it breaks the line on every pass: `audio/sound.c`, `dnas/dnas2prot_5.c`).
- The only other `clang-format off/on` regions allowed: the two line-pinned sites (section 1), the `asm` block of
  `gfx/bt2ipu.cpp` (its instruction strings would be realigned), the three K&R definitions
  (`_sce_dnas_pwrap_main`, `pwrap_info_lookup`, `ssl_rcv`: clang-format misparses them).
- Every formatting run is a source change: `make check` and `make link && make link-cmp` as always.
