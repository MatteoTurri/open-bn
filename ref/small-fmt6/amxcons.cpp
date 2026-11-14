/* Small "file format 6" -- the "console" native module (amxcons) of the program.
 *
 * A TU SEPARATE from amx_strpack.cpp: the table smallext_cons (.data
 * 0x10001900, 2 entries + terminator) is followed by 8 bytes of padding up
 * to the 16-byte boundary where smallext_core starts (0x10001920); the two
 * bodies (0x4DEAC0, 0x4DEAD8) end the .text right before n_numargs
 * (0x4DEAF0), and the two strings (0x94011C, 0x940128) come before those
 * of the core table. Of the console natives of the library only getstring
 * and printf remain, reduced to `return 0`: the script console is off in
 * this program (20 B bodies each). The file name is that of the module in
 * the Small distribution (AMX/amxcons.c); the binary does not record it.
 */
#include "amx.h"
/* script.h: the header of the script host, which the smallext_* native
   modules include after amx.h (the Script is what the natives reach through
   the user data of the AMX). In the original every native TU, this one too
   although it uses nothing of the Script, has "bad_alloc" at the start of its
   .rodata (<vector> from script.h) and "out of memory" at the end (the inline
   constructor of Script::MessageStr, which copies a string). */
#include "../../src/script/scenewindow.h"
#include "../../src/script/script.h"

static cell AMX_NATIVE_CALL n_getstring(AMX *amx, cell *params)
{
  return 0;
}

static cell AMX_NATIVE_CALL n_printf(AMX *amx, cell *params)
{
  return 0;
}

AMX_NATIVE_INFO smallext_cons[] = {
  { "getstring", n_getstring },
  { "printf",    n_printf },
  { 0, 0 }
};
