/* gthr-default.h -- NOT a file of the gcc tarball: configure GENERATES it
   (configure.in, line 4169) when gcc is configured with a thread package,
   together with -DHAVE_GTHR_DEFAULT in GTHREAD_FLAGS. The libgcc.a linked
   into top.so was built with POSIX threads: weak pthread_* references and
   a thread-specific EH context. */
#include "gthr-posix.h"
