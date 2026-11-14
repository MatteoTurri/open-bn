/*
 * dptoul: double -> unsigned int conversion (float_to_usi of the soft-float
 * library fp-bit.c, under its "US Software goFast" name).
 */

#define FINE_GRAINED_LIBRARIES
#define US_SOFTWARE_GOFAST
#define L_df_to_usi
#include "../../ref/gcc-2.95.2/fp-bit.c"
