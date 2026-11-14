/* crtstuff.c of gcc 2.95.2 (the crtbegin half), rebuilt from the binary:
   0x47C230-0x47C3D8, the first five bodies of .text after `_start`.
   It lives in ref/ because it is a TOOLCHAIN file, not Sony code, and it
   really contains the upstream `asm (INIT_SECTION_ASM_OP)` statements:
   they are the MECHANISM of the file (they move the `jal` into .init and
   .fini), not a way to steer code generation, and they are exactly the
   inline asm a vendored library is allowed to keep.

   Upstream, in the toolchain tarball of this repository
   (docker/sonygcc/gcc-2.95.2.tar.gz -> gcc-2.95.2/gcc/crtstuff.c):
     line 187  `asm (FINI_SECTION_ASM_OP);`  in `fini_dummy`
     line 211  `asm (INIT_SECTION_ASM_OP);`  in `init_dummy`  (EH_FRAME branch)
     line 376  `asm (INIT_SECTION_ASM_OP);`  in `init_dummy`  (CRT_END branch)
   The upstream functions are named `init_dummy` / `fini_dummy` like
   ours, carry `__attribute__ ((__unused__))` and have the same shape
   (section asm, the call, the asm that returns to .text). The Sony
   ps2linux-1.0.0 patch does not touch this file: it has no hunk on
   crtstuff.c, whose name appears only in two config comments
   (docker/sonygcc/gcc-ps2linux-1.0.0.patch).
   Here the three section switches are written as string literals
   instead of the INIT/FINI/TEXT_SECTION_ASM_OP macros, which come from
   the target headers (not vendored).

   The mechanism is visible in the binary (disassembly of .init and
   .fini):
     .init (0x47C168, 100 B) = crti prologue + `jal 0x47C230` +
       `jal 0x47C360` + `jal 0x4915F0` + crtn epilogue
     .fini (0x93BEB8, 136 B) = prologue + `jal 0x47C270` + epilogue
   so the three functions that are "empty with a frame" (0x47C338,
   0x47C3B0 and 0x911668) have their `jal` EMITTED INTO ANOTHER SECTION:
   only the prologue and the epilogue stay in .text, with `.cprestore 16`
   and the save of `ra`, because `current_function_outgoing_args_size`
   and `regs_ever_live[31]` were already set when the call was expanded.

   The crtend half (0x911668 and `__do_global_ctors_aux`) is not here:
   it is the same file compiled with -DCRT_END, so it is a unit of its
   own (crtstuff_end.c). */

typedef void (*func_ptr) (void);

/* The heads of the two lists: a single element, the -1 sentinel. */
static func_ptr __CTOR_LIST__[1] __attribute__ ((section (".ctors"))) = { (func_ptr) (-1) };
static func_ptr __DTOR_LIST__[1] __attribute__ ((section (".dtors"))) = { (func_ptr) (-1) };
static char __EH_FRAME_BEGIN__[] __attribute__ ((section (".eh_frame"))) = { };

struct object { long long space[4]; };

extern void __register_frame_info (void *, struct object *) __attribute__ ((weak));
extern void __deregister_frame_info (void *) __attribute__ ((weak));
extern void __gmon_start__ (void) __attribute__ ((weak));

/* 0x47C230 (64 B) */
static void
call_gmon_start (void)
{
  /* the address goes into a LOCAL and the call goes through it
     (`move t9,v0`); calling `__gmon_start__ ()` directly makes gcc
     reload the GOT slot (`lw t9,%call16`), which the binary does not. */
  void (*gmon) (void) = __gmon_start__;

  if (gmon)
    gmon ();
}

/* 0x47C270 (200 B) */
static void
__do_global_dtors_aux (void)
{
  static func_ptr *p = __DTOR_LIST__ + 1;
  static int completed = 0;

  if (completed)
    return;

  while (*p)
    {
      p++;
      (*(p - 1)) ();
    }

  if (__deregister_frame_info)
    __deregister_frame_info (__EH_FRAME_BEGIN__);

  completed = 1;
}

/* 0x47C338 (40 B): the `jal __do_global_dtors_aux` lands in .fini */
static void
fini_dummy (void)
{
  asm ("\t.section\t.fini");
  __do_global_dtors_aux ();
  asm ("\t.text");
}

/* 0x47C360 (80 B) */
static void
frame_dummy (void)
{
  static struct object object;

  if (__register_frame_info)
    __register_frame_info (__EH_FRAME_BEGIN__, &object);
}

/* 0x47C3B0 (40 B): the `jal frame_dummy` lands in .init */
static void
init_dummy (void)
{
  asm ("\t.section\t.init");
  frame_dummy ();
  asm ("\t.text");
}
