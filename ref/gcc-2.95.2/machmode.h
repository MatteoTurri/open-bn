/* machmode.h -- an EMPTY placeholder, on purpose.
   libgcc2.c includes it (line 45) because in the gcc build it brings the
   enum of the machine modes, but the sections compiled from it here
   (L_moddi3, L_floatdisf) use none of it: the modes come from the
   `__attribute__ ((mode (SI/DI/SF/DF)))` attributes, resolved by the compiler.
   The real file cannot be vendored without machmode.def and a good part
   of the chain of gcc's internal headers. */
