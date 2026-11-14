/* Small "file format 6" -- the "core" native module of the program, the
 * cut-down build of Small's AMX/amxcore.c (compiled as C++ here, because the
 * native modules include the script host headers). It holds amx_StrPack/
 * amx_StrUnpack, which convert an array of cells between the "unpacked"
 * form (one cell per character, value <= UCHAR_MAX) and the "packed" one
 * (several characters per cell, big-endian inside the cell), and the core
 * natives. Upstream they are in amxcore.c, not in amx.c: ref/small-fmt6/amx.c
 * stops at amx_StrLen/amx_SetString/amx_GetString, which work on C strings.
 *
 * Both call amx_StrLen(cstr,len) with ONE instruction (lw t9,OFF(gp), no
 * addiu t9,t9 after it): amx_StrLen lives in ANOTHER TU (ref/small-fmt6/amx.c)
 * and is only declared extern here; defined here it would be a same-TU call
 * (lw+addiu, 2 instructions).
 */

/* The TU is also the native module "core Small" of the AMX_NATIVE_INFO
 * table @0x10001920 (smallext_core, 14 entries, numargs..clamp): the
 * registered natives sit at 0x4DEAF0-0x4DF320, AROUND the two named
 * functions above, and two of them (strpack/strunpack) call
 * amx_StrPack/amx_StrUnpack with the two-instruction GOT_PAGE+LO16 form
 * that only a same-TU callee gets (static_004DEEB0 -> 0x4DEC40 and
 * static_004DF010 -> 0x4DED50). The TU is therefore larger than the
 * span of the registered addresses and also holds .dynsym symbols.
 * The functions below are in ADDRESS ORDER.
 *   Compared with amxcore.c of Small 2.73: no funcidx, no
 * getproperty/setproperty and no property list, and amx_StrPack/
 * amx_StrUnpack test `> UCHAR_MAX` instead of UNPACKEDMAX.
 *   FLAGS: no -finline-functions here (unlike ref/small-fmt6/amx.c).
 * With it gcc expands amx_StrPack and amx_StrUnpack INSIDE the natives
 * strpack/strunpack (130 instructions against 87), while the original
 * CALLS them -- with the GOT_PAGE+LO16 form, so the out-of-line copy in
 * the same TU really exists.
 *
 */
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include "amx.h"
/* script.h: the header of the script host, which the smallext_* native
   modules include after amx.h (the Script is what the natives reach through
   the user data of the AMX). In the original every native TU, this one too
   although it uses nothing of the Script, has "bad_alloc" at the start of its
   .rodata (<vector> from script.h) and "out of memory" at the end (the inline
   constructor of Script::MessageStr, which copies a string). */
#include "../../src/script/scenewindow.h"
#include "../../src/script/script.h"

#define CHARBITS        (8*sizeof(char))

extern int AMXAPI amx_StrLen(cell *cstr, int *length);

extern "C" {
	/* GOT-17904 */ int AMXAPI amx_GetAddr(AMX *amx, cell amx_addr, cell **phys_addr);
	/* GOT-5092  */ int AMXAPI amx_RaiseError(AMX *amx, int error);
}

/* ---- "core" natives of the module (table @0x10001920) -----------------
 * The accesses to amx->base/frm/hea/stk/stp come out as lwl/lwr because
 * struct AMX of amx.h is __attribute__((packed)) (e.g. at 0x4DEAFC).
 */

static cell AMX_NATIVE_CALL n_numargs(AMX *amx, cell *params)
{
  AMX_HEADER *hdr;
  unsigned char *data;
  cell bytes;

  hdr = (AMX_HEADER *)amx->base;
  data = amx->base + (int)hdr->dat;
  /* the number of bytes of the arguments is on the stack at "frm + 2*cell" */
  bytes = * (cell *)(data + (int)amx->frm + 2*sizeof(cell));
  return bytes / sizeof(cell);
}

static cell AMX_NATIVE_CALL n_getarg(AMX *amx, cell *params)
{
  AMX_HEADER *hdr;
  unsigned char *data;
  cell value;

  hdr = (AMX_HEADER *)amx->base;
  data = amx->base + (int)hdr->dat;
  value = * (cell *)(data + (int)amx->frm + ((int)params[1] + 3)*sizeof(cell));
  value += params[2]*sizeof(cell);
  value = * (cell *)(data + (int)value);
  return value;
}

static cell AMX_NATIVE_CALL n_setarg(AMX *amx, cell *params)
{
  AMX_HEADER *hdr;
  unsigned char *data;
  cell value;

  hdr = (AMX_HEADER *)amx->base;
  data = amx->base + (int)hdr->dat;
  value = * (cell *)(data + (int)amx->frm + ((int)params[1] + 3)*sizeof(cell));
  value += params[2]*sizeof(cell);
  if (value < 0 || (value >= amx->hea && value < amx->stk))
    return 0;
  * (cell *)(data + (int)value) = params[3];
  return 1;
}

static cell AMX_NATIVE_CALL n_heapspace(AMX *amx, cell *params)
{
  return amx->stk - amx->hea;
}

/* The shape of amx_StrPack in Small's amxcore.c (as amx_StrUnpack below):
 * `while` on the already packed branch, `for` with `i % sizeof(cell)` on
 * the branch that packs. */
int AMXAPI amx_StrPack(cell *dest, cell *source)
{
  int len;

  amx_StrLen(source, &len);
  if ((ucell)*source > UCHAR_MAX) {
    /* source string is already packed */
    while (len >= 0) {
      *dest++ = *source++;
      len -= sizeof(cell);
    } /* while */
  } else {
    /* pack string, from bottom up */
    cell c;
    int i;
    for (c = 0, i = 0; i < len; i++) {
      assert((*source & ~0xffL) == 0);
      c = (c << CHARBITS) | *source++;
      if (i % sizeof(cell) == sizeof(cell) - 1) {
        *dest++ = c;
        c = 0;
      } /* if */
    } /* for */
    if (i % sizeof(cell) != 0)  /* store remaining packed characters */
      *dest = c << (sizeof(cell) - i % sizeof(cell)) * CHARBITS;
    else
      *dest = 0;                /* store full cell of zeros */
  } /* if */
  return AMX_ERR_NONE;
}

/* The shape of amx_StrUnpack in Small's amxcore.c: a decreasing `for` on
 * the packed branch and `while ((*dest++ = *source++) != 0);` on the
 * already unpacked one. The first iteration peeled off in front of the
 * loop in the original (store of the *source already read, then beqz)
 * is gcc duplicating the exit test of the while, not a hand-written
 * do/while. */
int AMXAPI amx_StrUnpack(cell *dest, cell *source)
{
  if ((ucell)*source > UCHAR_MAX) {
    /* source string is packed */
    int len, i;
    amx_StrLen(source, &len);
    dest[len] = 0;
    for (i = len - 1; i >= 0; i--)
      dest[i] = (source[i/sizeof(cell)] >> ((sizeof(cell)-1-i % sizeof(cell))*CHARBITS)) & UCHAR_MAX;
  } else {
    /* source string is already unpacked */
    while ((*dest++ = *source++) != 0)
      /* nothing */;
  } /* if */
  return 0;
}


static cell AMX_NATIVE_CALL n_strlen(AMX *amx, cell *params)
{
  cell *cptr;
  int len = 0;

  if (amx_GetAddr(amx, params[1], &cptr) == AMX_ERR_NONE)
    amx_StrLen(cptr, &len);
  return len;
}

/* The validity check of the destination block: the first and the last
 * cell must fall in the SAME region -- both in data/heap (0..hea) or
 * both in the stack (stk..stp). Written inline, with the second region
 * as a boolean value in the `else` branch: this form keeps the boolean
 * in a register live on both branches (a3) without the `movn`
 * if-conversion; with an inline helper, or with `ok = 0; if (...) ok =
 * ...;`, gcc emits movn or keeps the boolean in v0.
 */
static cell AMX_NATIVE_CALL n_strpack(AMX *amx, cell *params)
{
  cell *cstr, *cdest;
  int len, needed, first, lastaddr, err;
  int ok;

  amx_GetAddr(amx, params[2], &cstr);
  amx_StrLen(cstr, &len);
  needed = (len + sizeof(cell)) / sizeof(cell);
  lastaddr = params[1] + sizeof(cell) * needed - 1;
  first = params[1];
  if (first >= 0 && first < amx->hea && lastaddr >= 0 && lastaddr < amx->hea)
    ok = 1;
  else
    ok = first >= amx->stk && first < amx->stp && lastaddr >= amx->stk && lastaddr < amx->stp;
  if (!ok)
  {
    amx_RaiseError(amx, AMX_ERR_MEMACCESS);
    return 0;
  } /* if */
  amx_GetAddr(amx, params[1], &cdest);
  err = amx_StrPack(cdest, cstr);
  if (err != AMX_ERR_NONE)
    return amx_RaiseError(amx, err);
  return len;
}

static cell AMX_NATIVE_CALL n_strunpack(AMX *amx, cell *params)
{
  cell *cstr, *cdest;
  int len, first, lastaddr, err;
  int ok;

  amx_GetAddr(amx, params[2], &cstr);
  amx_StrLen(cstr, &len);
  lastaddr = len * sizeof(cell) + params[1] + sizeof(cell) - 1;
  first = params[1];
  if (first >= 0 && first < amx->hea && lastaddr >= 0 && lastaddr < amx->hea)
    ok = 1;
  else
    ok = first >= amx->stk && first < amx->stp && lastaddr >= amx->stk && lastaddr < amx->stp;
  if (!ok)
  {
    amx_RaiseError(amx, AMX_ERR_MEMACCESS);
    return 0;
  } /* if */
  amx_GetAddr(amx, params[1], &cdest);
  err = amx_StrUnpack(cdest, cstr);
  if (err != AMX_ERR_NONE)
    return amx_RaiseError(amx, err);
  return len;
}

static cell AMX_NATIVE_CALL n_swapchars(AMX *amx, cell *params)
{
  union {
    cell c;
    unsigned char b[sizeof(cell)];
  } value;
  unsigned char t;

  /* byte swap for SMALL_CELL_SIZE==32 as in Small's amxcore.c, which writes
   * it out in an #if for each cell size: with constant indexes the union
   * stays in a REGISTER (with a `for` over a variable index it goes to the
   * stack and the instruction count changes). */
  value.c = params[1];
  t = value.b[0];
  value.b[0] = value.b[3];
  value.b[3] = t;
  t = value.b[1];
  value.b[1] = value.b[2];
  value.b[2] = t;
  return value.c;
}

/* tolower/toupper as in Small's amxcore.c (the non-Windows branch): the
 * call to the `extern __inline` of the kit glibc's <ctype.h> is expanded
 * in the body. */
static cell AMX_NATIVE_CALL n_tolower(AMX *amx, cell *params)
{
  return tolower((int)params[1]);
}

static cell AMX_NATIVE_CALL n_toupper(AMX *amx, cell *params)
{
  return toupper((int)params[1]);
}

static cell AMX_NATIVE_CALL n_min(AMX *amx, cell *params)
{
  return params[1] <= params[2] ? params[1] : params[2];
}

static cell AMX_NATIVE_CALL n_max(AMX *amx, cell *params)
{
  return params[1] >= params[2] ? params[1] : params[2];
}

static cell AMX_NATIVE_CALL n_clamp(AMX *amx, cell *params)
{
  cell value = params[1];

  if (params[2] > params[3])
    amx_RaiseError(amx, AMX_ERR_NATIVE);
  if (value < params[2])
    value = params[2];
  else if (value > params[3])
    value = params[3];
  return value;
}

static cell AMX_NATIVE_CALL n_random(AMX *amx, cell *params)
{
  if (params[1] == 0) {
    amx_RaiseError(amx, AMX_ERR_NATIVE);
    return 0;
  } /* if */
  return rand() % params[1];
}

/* The table of the module, in .data @0x10001920. The order is that of
 * the original and of amxcore.c (random BEFORE min/max/clamp), not the
 * address order. */
AMX_NATIVE_INFO smallext_core[] = {
  { "numargs",   n_numargs },
  { "getarg",    n_getarg },
  { "setarg",    n_setarg },
  { "heapspace", n_heapspace },
  { "strlen",    n_strlen },
  { "strpack",   n_strpack },
  { "strunpack", n_strunpack },
  { "swapchars", n_swapchars },
  { "tolower",   n_tolower },
  { "toupper",   n_toupper },
  { "random",    n_random },
  { "min",       n_min },
  { "max",       n_max },
  { "clamp",     n_clamp },
  { 0, 0 }
};
