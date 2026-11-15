# src/

The Sony code of `bn`, one folder per subsystem. Third-party libraries (SSLeay,
SQLite, zlib, the image libraries, the Small interpreter) are in `../ref/`.

## Where to start

`main()` and the main loop are in `app/jobinit2.cpp`. The application is built
around a 3D scene (`scene/`) drawn with the PS2 graphics code in `gfx/` and
`fw/`. Its pages are XML documents (`xml/`) driven by scripts in the Small
language (`script/`), which reach the rest of the program through the native
functions in `natives/`. Long operations (file copies, downloads, image
decoding) run as worker threads built on `thread/`.

## Folders

| folder | contents |
|---|---|
| `app/` | main loop, the `BN` and `Desktop` classes, commands, modules, system configuration, resources, database, time, downloads, mail notification |
| `script/` | the script host: `Script`, `SceneWindow` (a page), the page loader thread, error handling |
| `natives/` | the native functions the scripts call, grouped by area (files, images, models, strings, XML, ...) |
| `xml/` | XML DOM and SAX parser |
| `scene/` | scene graph: objects, hooks, animations, camera and lights, 3D widgets (icons, models, particles, clouds, ...) |
| `ui/` | text widgets and menus: labels, text fields, text editor, message box |
| `gfx/` | graphics: GS/VU1 drawing (`gm_*`), model loader (GMO), texture manager, image loaders, vector math, text rendering, IPU decoder |
| `fw/` | low-level PS2 graphics framework: GS device, DMA packets and memory |
| `image/` | image pipeline: decoders and encoders (BMP, GIF, JPEG, MNG, PNG, TIFF), raw images, scaling, EXIF, decoder thread |
| `fs/` | file layer: BB* filesystem and URL functions, file list and file operation threads, icon loader, media checker, database thread |
| `thread/` | worker thread base classes and semaphores |
| `ime/` | Japanese text input: ATOK, FEP, software keyboard |
| `input/` | IR remote, pad, USB mouse |
| `memcard/` | memory card access, PS1 virtual memory card |
| `install/` | title installer |
| `http/` | HTTP and HTTPS client, URLs, base64, MD5 |
| `dnas/` | DNAS network authentication, including its self-modifying protection, and system update |
| `audio/` | sound driver, sound library, MIDI input |
| `chloe/`, `cfd/`, `rcom/` | Sony crypto and security libraries |
| `lib/` | statically linked runtime pieces: libgcc and glibc fragments, allocators, character set conversion, PFS client |

`chloe/` is the code that leaves Linux and restarts the console as a native PS2
(reached only from `bn_do_reboot()` in `app/bnexec.cpp`, before `/sbin/reboot`): it
fills a context object, then decrypts and runs two embedded encrypted stages, a
user-space loader that writes the second stage to physical memory through
`/dev/akmem`, and a bare-metal EE program that takes the machine over from Linux.
The two stages are encrypted blobs extracted from the original at build time.

Every `.c`/`.cpp` is a compilation unit listed in `../config/units.txt` with its
compiler flags.

## Includes

The build adds `-Isrc`. A header in the same folder is included by name
(`#include "gm_packet.h"`); a header in another folder by its path from `src/`
(`#include "gfx/vec.h"`). Each header sits next to the `.cpp` that implements it.
`#pragma implementation "x.h"` always names the header without its folder,
otherwise g++ 2.95 does not pair it with the header's `#pragma interface`.

## Reading this code

This code reproduces the original binary byte for byte: the order of
declarations, definitions and includes, local re-declarations, types and
temporaries are deliberate. Run `make check` and `make link-cmp` after any change.

Names found in the binary's dynamic symbol table (classes, methods, exported
functions, the names in the native tables of the scripts) are the original ones.
The native functions of the scripts are file-local, but their names are original
too: they are registered in 28 `AMX_NATIVE_INFO` tables, each named `smallext_*`
in the dynamic symbol table, whose `{name, function}` entries give every native
its script name.
All other names, including those of the 2,308 file-local functions, were chosen
during the decompilation from what the code does.

Comment markers:
- `/* NOTE: bug: ... */` a defect of the original program, kept as it is;
- `/* unused, but part of the layout */` a variable, buffer or field that is never read;
- `/* redundant as written */` a dead store, a dead branch or a redundant test;
- `/* the line number is part of the binary */` on a `#line` directive.
- `/* clang-format off */` ... `/* clang-format on */` around the few places the formatter must not touch: the two
  lines whose line number reaches the binary, an inline asm block, K&R definitions and tables laid out in rows.

## Formatting

The sources are formatted with clang-format 23.1.5 and the `.clang-format` at the root: run `make format`
before committing (`make check` fails on unformatted files). Details in `tools/README.md`.
