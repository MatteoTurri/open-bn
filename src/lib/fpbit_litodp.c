/*
 * litodp: signed int -> double conversion (si_to_float of the soft-float
 * library fp-bit.c, under its "US Software goFast" name).
 */

#define FINE_GRAINED_LIBRARIES
#define US_SOFTWARE_GOFAST
#define L_si_to_df
#include "../../ref/gcc-2.95.2/fp-bit.c"
