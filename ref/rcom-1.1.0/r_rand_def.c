/*
 * R_rand_get_default and the default generator object.
 */

#include "r_rand.h"

#include <string.h>


static RRandMeth g_rrandDefault = { 0 };
static int g_rrandDefaultNeedsInit = 1;

/* The flag means "to initialize": the first call builds the default generator from
 * R_rand_meth_sha1 and clears the flag. */
RRandMeth *R_rand_get_default(void)
{
    if (g_rrandDefaultNeedsInit != 0) {
        memcpy(&g_rrandDefault, R_rand_meth_sha1(0), 20);
        g_rrandDefaultNeedsInit = 0;
    }
    return &g_rrandDefault;
}
