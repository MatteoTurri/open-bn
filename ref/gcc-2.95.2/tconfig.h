/* tconfig.h -- NOT a file of the gcc tarball: the gcc build GENERATES it
   (rule `tconfig.h` in the Makefile), including ansidecl.h and the chain of
   the target's `tm_file`. This is the minimum needed to compile libgcc2.c
   for the target of this project (mipsel-linux, PS2 Linux Kit): only the
   macros that libgcc2.c actually reads outside the L_bb/L_trampoline sections.
   The values are NOT chosen: they are those of the target.
     WORDS_BIG_ENDIAN 0        mipsEEel is little endian
     BITS_PER_UNIT 8
     LONG_DOUBLE_TYPE_SIZE 64  on MIPS long double is a double
   `SUPPORTS_WEAK` stays 0 on purpose: it only decides whether libgcc2.c
   defines weak aliases, and the original has none.
   */
#ifndef GCC_TCONFIG_SHIM_H
#define GCC_TCONFIG_SHIM_H

#define WORDS_BIG_ENDIAN 0
#define BITS_PER_UNIT 8
#define LONG_DOUBLE_TYPE_SIZE 64
#define SUPPORTS_WEAK 0

#endif
