/* crtstuff.c of gcc 2.95.2, the **crtend** half (upstream it is the SAME
   file as crtstuff.c compiled with -DCRT_END; here it is a second path
   because each unit is one source file). Bodies 0x9115F0-0x911690, at the
   end of .text, right before the lazy-binding stubs of the linker.
   Same as crtstuff.c for the `asm (INIT_SECTION_ASM_OP)`: it is the
   upstream mechanism, not a way to steer code generation, and the proof
   is that the `jal` is not in .text.
   `__CTOR_END__ - 1` = 0x100D18A4, the last word of .ctors
   (0x100D17A0 + 0x10C). */

typedef void (*func_ptr) (void);

static func_ptr __CTOR_END__[1] __attribute__ ((section (".ctors"))) = { (func_ptr) 0 };
static func_ptr __DTOR_END__[1] __attribute__ ((section (".dtors"))) = { (func_ptr) 0 };
static char __FRAME_END__[4] __attribute__ ((section (".eh_frame"))) = { 0, 0, 0, 0 };

/* 0x9115F0 (116 B) */
static void
__do_global_ctors_aux (void)
{
  func_ptr *p;

  for (p = __CTOR_END__ - 1; *p != (func_ptr) (-1); p--)
    (*p) ();
}

/* 0x911668 (40 B): the `jal __do_global_ctors_aux` lands in .init */
static void
init_dummy (void)
{
  asm ("\t.section\t.init");
  __do_global_ctors_aux ();
  asm ("\t.text");
}
