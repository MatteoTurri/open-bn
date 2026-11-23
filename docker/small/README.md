# Small 2.1.0 compiler

The Small compiler of ITB CompuPhase, release 2.1.0 (2002), the compiler of the bn scripts
(`/opt0/bn/script/**/*.bnx`, docs/scripts.md). It writes AMX file format 6 with AMX version 6,
the version field of every script of `/opt0/bn`; 2.2.0 already sets the nochecks flag and 2.4
writes format 7. `make small` builds it (`Dockerfile.small`) into `docker/toolchain/small/sc`.
License: `small-2.1.0/license.txt`.

## Where it comes from

CompuPhase no longer publishes the 2.x releases. The sources in `small-2.1.0/compiler/` are
the copy kept in the nox-wizard project (github.com/nox-wizard/nxwtoolset, `compiler/`, CVS
revisions of 2003-04-26). Another copy of 2.1.0, in the OpenLegends project
(github.com/Kronuz/OpenLegends, `QuestDesigner/libs/compiler/`), differs from it only in
portability details (MSVC names such as `_unlink`, `_stricmp`, `_strdup`, the include of
`sclinux.h`, casts), so the code generation is CompuPhase's. `license.txt` is from the
OpenLegends copy. `small-2.1.0/linux/getch.c` and `getch.h` are from the Linux directory of
the Small 2.2.0 kit (`smallkit.tgz`).

Changes from CompuPhase's release in the nox-wizard copy: `#include <sclinux.h>` commented out
in `sc1.c`, `sc2.c`, `sc6.c`; a comment reworded in `sc6.c`; the CVS `$Id$` lines.

## The build

`gcc -m32 -std=gnu89 -O1 -w -fcommon -DLINUX -Dstricmp=strcasecmp -D_MAX_PATH=4096
-Icompiler -Ilinux ... -lm`, linked statically. `_MAX_PATH`: with the default 255 bytes
`setconfig()` overflows its buffer on long paths. `-m32`: the compiler assumes 32-bit cells
and pointers. None of these affects the code the compiler generates. `tools/check_toolchain.py`
pins the patch below and the sha256 of the `sc` the scripts were verified with.

On Linux the compiler rebuilds its own path from the `PWD` environment variable and crashes
when it is not set (a NULL entry in its scan of the environment): run it with `PWD` set.
Its `-S` option takes no less than 65 cells; the scripts give their stack size with
`#pragma dynamic` instead.

## The patch

`small-2.1.0-operatoradjust.patch`: on the second pass `operatoradjust()` (`sc1.c`) gives a
user-defined operator the referrer count of its first-pass symbol without growing its
referrer list, so `refer_symbol()` writes past the list. Depending on the heap layout this
overwrites other symbols: the prototype of a function far away then no longer matches its
definition (error 025), or a symbol is no longer found. Six scripts of `/opt0/bn` do not compile
without the patch (`guide/menu`, `guide/qa`, `guide/text_nofocus`, `feegaguide/menu`,
`feegaguide/qa`) or compile to another module (`utility/browser`). The list now grows first.
The code of a script that compiled before is the same.
