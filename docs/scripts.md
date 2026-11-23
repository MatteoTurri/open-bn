# The scripts of bn

The pages of the interface are XML documents driven by scripts in the Small language
(`src/script/` is the host that runs them, `src/natives/` the functions they call). The
scripts ship compiled: `/opt0/bn/script/<path>.bnx`, each an AMX module (the bytecode of the
Small abstract machine) inside DNAS2 encryption. `src/bnx/<path>.sma` is the source of
each one, and the Small compiler turns it back into the original module byte for byte.

## Where the originals come from

The 227 `.bnx` are in the full archive of the PSBBN Definitive Patch v4.1.0
(`./__linux.4/bn/script/`), the archive `make link` and `make modules` already download; no
language pack carries them. `config/scripts.txt` lists them, one row per script: the path
under `/opt0/bn/script/`, the sha1 of the `.bnx`, and the sha1 and size of the module it
carries (what bn's own verifier gives back from it). The gate needs only these two values, not
the originals: the repository has no decryption of its own for the scripts, and none of their
bytes.

## The compiler

Small 2.1.0 of ITB CompuPhase (2002), rebuilt from source by `make small` into
`docker/toolchain/small/sc` (`docker/small/README.md`: where the sources come from, the build,
the one patch, without which six of the scripts do not rebuild). Every script of `/opt0/bn`
carries AMX version 6, which 2.1.0 writes and 2.2.0 does not reproduce (it sets the nochecks
flag); 2.4 writes another file format.

The options are the same for every script: `sc -d0 -t0 -\ -C+ -P+ -;+`. `-d0`: the scripts
have no `line` opcodes, which 2.1.0 already emits at `-d1`. `-C+`: compact encoding. `-P+`:
strings packed by default. `-;+`: semicolons required. `-\`: `\` as the escape character. The
stack size is in each source (`#pragma dynamic`), because `-S` takes no less than 65 cells.
`-i` names the include files the scripts share (below).

## The gate

`tools/scriptcheck.py` compiles every row of `config/scripts.txt` into
`build/scripts/<path>.amx` and compares it with the sha1 and size of the original module. It
fails on a module that differs, a source that does not compile, a row without its source and a
source without its row. It is a step of `make check` (full runs, not `UNITS=`); `make scripts`
runs it alone (`ARGS=game/game` for one script, `ARGS=-v` for one line each).

## The sources

They were recovered from the modules (`tools/amxcorpus.py` of the bnx-decrypt tools), so only
what a module keeps has its original name: the public functions and variables, the natives and
the operators of `float.inc`. Every other name (functions, globals, parameters, locals, labels,
constants, the parameters of the natives and the include files of the helpers) was chosen
after reading the code, following section 8 of `docs/code-conventions.md`; renaming one leaves
the module unchanged, since `-d0` keeps no symbol that is not public.

`TAG` keeps the name the scripts of a Hudson channel give to the same function.

Each source starts with the path of its module and the options; the `-iinclude` there is
relative to `src/bnx/` (the gate passes the full path). The scripts share include files, in
`src/bnx/include/`:

- `core.inc` and `float.inc`: the include files of the Small toolkit of 2003 (version 2.2.0),
  unchanged; `core.inc` is by ITB CompuPhase, `float.inc` by Artran Inc. and Greg Garner
  (`smallkit-license.txt`: the license of the toolkit). The stock operators of `float.inc` are
  compiled where a script includes it, after its first helper functions.
- `bn.inc`: the natives of bn, one prototype for all the scripts. Where bn's own
  implementation of a native is known (`src/natives/`), the parameters that are floats and the
  ones that take an address come from it; elsewhere from the calls of the scripts. `printf`,
  `sprintf`, `SystemCommand`, `SetOptionI` and `SetOptionF` are variadic, as their calls show.
  It also declares the constants of their arguments (pad bits, animation curves, system
  parameters, disc types, blend modes, resource textures, ...).
- `<library>.inc` (`mailer.inc`, `edclient.inc`, `movie.inc`, `photo.inc`, ...): the natives of
  a library, under its `#pragma library`, in the order of the FuncTable of
  `/opt0/bn/modules/<library>.so` (`player.so` comes with the language pack,
  `docs/modules.md`). The mailer's natives are all variadic; `mailer.inc` also has the
  constants its scripts share (folders, message states, the screens and modes the pages pass to
  each other), `player.inc` the numbers of the player commands.
- `stdmenu.inc`, `infomenu.inc`, `scroll.inc`, `devimage.inc`, `moviedb.inc`, `musicdb.inc`,
  `gamedb.inc`, `photodb.inc`, `topmenu.inc`, `edpage.inc`, `browser.inc`: only constants, the
  interface between the scripts that include them (the results of the option and information
  menus, the scroll modes of the items on a curve and the messages they follow, the images of
  `device_image`, the movie database, the albums of the music pages, the game database, the
  album modes and the photo database, the scripts of the top menu, the pages of the edclient
  and the fields and requests they send to its server, the file browser). A declaration
  compiles to no code, so these files can go anywhere before the first use.
- `wait.inc`, `tag.inc`, `imgsync.inc`, `button.inc`, `text.inc`, `wndvar.inc`, `sound.inc`,
  `dialog.inc`, `xmlsync.inc`, `imgdec.inc`, `sysconf.inc`, `misc.inc`, `dom.inc`,
  `string.inc`, `disc.inc`, `focus.inc`, `pfs.inc`, `layout.inc`, `background.inc`,
  `bgmode.inc`, `fileop.inc`, `validate.inc`, `card.inc`, `date.inc`, `title.inc`: helper
  functions the scripts share (the wait by frames, `TAG`, image and XML loaders, DOM lookups,
  focus and message helpers, ...). A script compiles the ones it calls,
  as one block per file where it includes it; a function the scripts compile with different
  code, or only after their own functions, is a copy of the script.
- `mailsync.inc`, `mailline.inc`, `playerctl.inc`, `topisp.inc`, `edsreq.inc`, `edsid.inc`,
  `edsid2.inc`, `photodate.inc`, `photousb.inc`: helpers that call the natives of a library.

A global that two include files use, or that a script uses before the file that has it, is
declared under `#if !defined` wherever it is needed first. The scripts include `<core>`, `<bn>` and `<float>` in the same order,
and the files whose functions they only need declared (not compiled) at their end.

The library files are what the tables of the modules ask for. A module keeps, for each
library, how many calls the compiler read to its natives, also in the functions it then leaves
out: the helpers of a library file a script includes and does not use. Every script that loads
the mailer library, for instance, includes `mailsync.inc` (six helpers that wait for the
mailer, twelve calls) and misses exactly the calls of the helpers it does not compile. A script
whose last `#pragma library` stays open (`utility/dbconvert`, `system/download`,
`photo/photo_grid` and three of `movie/`) declares that library itself (`download`,
`moviedata` and `dbconvert` have no file of their own in `include/` for this reason) and counts under it
every native call of its source, read again in the second pass: these include whole helper
files they do not use (at their end) until the count is reached. That grouping of the helpers
into files is the one reconstruction the bytes do not settle alone.

Eight scripts (the focus and browser parts, `utility/menu/clock`) declare their natives
themselves and include nothing: with the shared prototypes their calls would not compile to
the same code. Three more (`edclient/agreement`, `photo/photo`, `photo/usb_process`) include
the natives but keep their own copies of the helpers, which the shared files give with other
code.

The strings keep the bytes of the original; the Japanese text is UTF-8. The floats are written
as decimals that the compiler turns back into the same bits.

An `assert` (four scripts) has no code: without run-time checks (`-d0`) the compiler drops it,
but a native called there still takes its place in the table of natives
(`assert IsValidObject(obj)`), and a user-defined operator used there is still compiled.
