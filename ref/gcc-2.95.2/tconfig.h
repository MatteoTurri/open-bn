/* tconfig.h -- NOT a file of the gcc tarball: the gcc build GENERATES it
   (rule `tconfig.h` in the Makefile), including gansidecl.h (which brings
   ansidecl.h, from include/ of the tarball) and the chain of the target's
   `tm_file`. This is the minimum needed to compile libgcc2.c
   for the target of this project (mipsel-linux, PS2 Linux Kit): only the
   macros that libgcc2.c (outside the L_bb/L_trampoline sections) and
   frame.c actually read.
   The values are NOT chosen: they are those of the target.
     WORDS_BIG_ENDIAN 0        mipsEEel is little endian
     BITS_PER_UNIT 8
     LONG_DOUBLE_TYPE_SIZE 64  on MIPS long double is a double
     SUPPORTS_WEAK 1           defaults.h: 1 when ASM_WEAKEN_LABEL is defined,
                               and config/mips/elf.h defines it. For libgcc2.c it
                               only decides WEAK_ALIAS, which also wants
                               ASM_OUTPUT_DEF (not defined here) and which no
                               section uses; gthr.h reads it for the weak
                               pthread references of the L_eh and frame.c members
                               (the weak UND pthread_* of top.so).
   The exception-handling members (L_eh of libgcc2.c, frame.c) also read:
     DWARF2_UNWIND_INFO 1      defaults.h: 1 when INCOMING_RETURN_ADDR_RTX is
                               defined (config/mips/mips.h, line 1192)
     STACK_GROWS_DOWNWARD      config/mips/mips.h, line 2214
     FIRST_PSEUDO_REGISTER 76  config/mips/mips.h, line 1666 (size of the
                               arrays of struct frame_state, frame.h)
   */
#ifndef GCC_TCONFIG_SHIM_H
#define GCC_TCONFIG_SHIM_H

#include "gansidecl.h"

#define WORDS_BIG_ENDIAN 0
#define BITS_PER_UNIT 8
#define LONG_DOUBLE_TYPE_SIZE 64
#define SUPPORTS_WEAK 1

#define DWARF2_UNWIND_INFO 1
#define STACK_GROWS_DOWNWARD
#define FIRST_PSEUDO_REGISTER 76

#endif
