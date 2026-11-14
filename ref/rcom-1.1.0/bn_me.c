/*
 * BN_get_default_exp_table and BN_default_init: the default table of modular exponentiation methods,
 * registered once in the BN_bnme_* cache.
 */

#include "bn_me.h"

/* The default table holds only its closing entry, so BN_library_init registers nothing. */
static BNME_ENTRY g_default_exp_table[] = { { 0, 0, 0, 0 } };
static int g_default_init_flag = 1;

void *BN_get_default_exp_table(void)
{
    return &g_default_exp_table;
}


int BN_default_init(void)
{
    if (g_default_init_flag) {
        g_default_init_flag = 0;
        BN_library_init(g_default_exp_table);
    }
    return 0;
}
