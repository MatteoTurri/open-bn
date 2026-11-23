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

## 8. The scripts (`src/bnx/`)

The Small sources of the scripts follow the same model: code a person can read, with the names and comments a
programmer of the time would have given it. Sections 2-4 apply (comments in English and ASCII, no process, the
markers of section 4); this section adds what is specific to Small. `include/core.inc` and `include/float.inc` are
the files of the Small toolkit and stay as they are.

### What the module fixes

- the names that are in the module: public functions and variables, `main`, the natives, the `#pragma library`
  names, the operators of `float.inc`; and `TAG`;
- the prototypes of the natives except the names of their parameters: tags, `&`, `[]`, `const`, default values,
  `...`;
- string literals, `#pragma` lines, the order of includes, of functions and of global and local declarations.

The shape of the code (the `{ new ...; }` blocks, `for (; ; )`, `goto`) is the code too: it changes only with the
module identical and the result still plausible, and never in the same commit as renames. `-d0` emits no line
numbers: adding or removing comment lines changes nothing in a module.

### Layout

No formatter knows Small (clang-format splits `Float:x` and `public x;`), so the layout is kept by hand: blocks with
the brace on its own line, 4 spaces per level, at most 120 columns. A longer statement continues on lines indented
8 more, filled up to 120 and broken after a comma or before a binary operator (`&&`, `||`, `|`, `+`), outer brackets
first; a string literal is never split (Small 2.1 does not join adjacent literals) and goes on a line of its own.

### Identifiers

Every other name is ours and is chosen for what the thing is or does, after reading the code that uses it:

- functions, parameters, locals, labels: `lower_snake_case`, functions with a verb (`wait_frames`, `set_focus`,
  `load_thumbnail`); a label names its target (`next_frame`, `done`, `retry`);
- non-public globals: always `g_` followed by `lower_snake_case`, also where the public names have no prefix (those
  are fixed by the pages and the other scripts); a local `static`: like a local;
- constants: `UPPER_SNAKE_CASE`, declared with `const` or an anonymous `enum` (a named enum is a tag);
- the scripts of the mailer (`mailer/`) are written in Hungarian notation, as their public names show (`g_iFontSize`,
  `g_txBg`, `g_flSpeed`, and `g_manager` where the other scripts have `manager`): there our globals are `g_` + type
  prefix + CamelCase (`i` cell, `fl` Float, `b` flag, `sz` string, `tx` texture, `obj` object; the prefix agrees with
  the tag and the use) and our functions are CamelCase like the public ones (`CreateTerminate`); a copy of a shared
  helper keeps the helper's name, and locals and parameters stay `lower_snake_case`. Another script whose public
  names are Hungarian follows the same rule;
- at most 31 characters (the significant length of a name in sc), and no name that hides another one (a local named
  like a global, a native or a helper of `include/`): the compiler warnings of a script stay the same. The names of
  the parameters of the natives count as global names here: sc warns when a script declares a global or a function
  with one of them;
- two helpers with the same code that cannot share a name (same file, or files a script includes together) take a
  suffix that says the difference, or a number where there is none (`get_screen_size2`), with a `/* Same as ... */`
  line. A script's own copy of a helper whose file it does not include takes the helper's name, the base name when
  the helper exists in numbered variants with the same code (`get_screen_size`). Every copy of a helper has the
  comment `/* Same as <helper> of <file>.inc. */` and nothing else unless it differs (`, with globals of its own`);
  the copies of a function of the scripts share one comment.

- the arguments of the messages: where a loop dispatches messages whose arguments mean different things, they are
  `a`, `b`, `c`, `d`, as in the prototypes of `RecvMessage` and `SendMessage`, and a comment at the dispatch says
  what they are for each message; a name that says what the argument is (`alpha`, `duration`) only where it means
  the same for every message the loop handles.

Names in `include/` are shared by all the scripts: a helper is renamed together with all its uses in the same
commit, and so is a script function that a helper calls back. The include files of the helpers are named after what
they contain (`wait.inc`, `dom.inc`, ...), not numbered; they keep their place and their content, since the grouping
of the helpers into files decides what the modules count for each library (`docs/scripts.md`).

### Constants

A number with a meaning gets a name when the name says more than the number: the bits of `PadState` (`PAD_LEFT`,
`PAD_CIRCLE`, ...), the indexes of `imgGetResource`, the states of a state machine. Constants shared by several
scripts are declared in `include/`; the ones of one script at its top. Sizes of buffers used once, colors,
coordinates and times stay as numbers. A character is written as a character literal (`';'`, `'0'`, not `59`, `48`);
a bit mask, a packed color and any value built from bytes in hexadecimal (`0x300`, `0x5F000000`); a measure in
1/64 pixel (the options named with 64: the 26.6 fixed point of FreeType) as pixels times 64 (`396 * 64`, folded
by the compiler). The ids of the sound effects, the states of `ScriptState`, the columns of a database and the
fields of an answer are constants.
A FourCC stays a `TAG("FOCS")` call: `TAG` is a function, and a constant in
its place changes the code. A constant never has a value or a tag other than the number it replaces.

### Comments

- File header: the module path and the options of sc stay, as the first line of the header; below it, 1-3 lines on
  what the script is (the page or part it drives, who loads it):
  ```
  /*
   * /opt0/bn/script/parts/button.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
   * A push button: a text label on a texture, which tells its manager when it is pressed.
   */
  ```
  The include files have the header of section 3.
- The interface of a script: the messages it receives and sends (`RecvMessage`/`SendMessage` with their FourCC and
  arguments) get a short comment at the loop that dispatches them where the arguments are not obvious; a public
  variable that the page or another script reads or writes gets a trailing comment with its meaning.
- A function comment where the name and the parameters are not enough, on the line above the definition.
- Natives: the parameters in `include/*.inc` are named after bn's implementation (`src/natives/`, `src/modules/`)
  where it is known, after the calls otherwise; a native whose behaviour is not obvious from its name gets one line.
- A Japanese string the code depends on (compared, used as a key) may get a one-line English gloss.

### Verification

`make scripts` (every module identical), the same compiler warnings, and no placeholder left in what was renamed: a
script renamed as a whole has none left (`func_<hex>`, `g_<hex>`, `static_<hex>`, `label_<hex>`, `var<n>`,
`arg<n>`, `p<n>`, and the numbered helper names `<file>_<k>`, `<file>_g<k>`, `<file>_sub<k>`); renaming a shared
helper changes only its name in the files that use it. Renames go in their own commits (section 5).
