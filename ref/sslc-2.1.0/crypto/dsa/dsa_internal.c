/*
 * DSA_new_internal and DSA_free_internal: the default constructor and destructor of the DSA method
 * table. SSL-C adds flags (1 at creation), method_mont_p (a Montgomery context, freed with
 * BN_MONT_CTX_free) and moves references after them.
 */

#include <stdio.h>
#include "r_com.h"
#include "bn.h"
#include "dsa.h"

char *DSA_version="DSA part of RCOM 1.1.0 28-Nov-2001";

DSA *DSA_new_internal(void)
{
    DSA *ret;

    ret = (DSA *)R_malloc(52);
    ret->pad = 0;
    ret->version = 0;
    ret->write_params = 1;
    ret->p = 0;
    ret->q = 0;
    ret->g = 0;
    ret->pub_key = 0;
    ret->priv_key = 0;
    ret->kinv = 0;
    ret->r = 0;
    ret->method_mont_p = 0;
    ret->flags = 1;
    ret->references = 1;
    return ret;
}

void DSA_free_internal(DSA *dsa)
{
    int refs;

    if (dsa == 0)
        return;

    refs = R_locked_add(&dsa->references, -1, 8, __FILE__, 65);
    if (refs > 0)
        return;

    if (dsa->p != 0)
        BN_clear_free(dsa->p);
    if (dsa->q != 0)
        BN_clear_free(dsa->q);
    if (dsa->g != 0)
        BN_clear_free(dsa->g);
    if (dsa->pub_key != 0)
        BN_clear_free(dsa->pub_key);
    if (dsa->priv_key != 0)
        BN_clear_free(dsa->priv_key);
    if (dsa->kinv != 0)
        BN_clear_free(dsa->kinv);
    if (dsa->r != 0)
        BN_clear_free(dsa->r);
    if (dsa->method_mont_p != 0)
        BN_MONT_CTX_free((BN_MONT_CTX *)dsa->method_mont_p);
    R_free(dsa);
}
