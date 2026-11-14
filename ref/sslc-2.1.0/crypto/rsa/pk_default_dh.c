/*
 * DH_new and DH_set_default_meth of the SSL-C fork, with their method hook.
 */

#include <stdio.h>
#include "pk_int.h"
#include "dh.h"

static PK_METH *(*dh_default_meth_hook)(void) = PK_METH_dh;

DH *DH_new(void)
{
    return DH_new_meth((*dh_default_meth_hook)());
}

void DH_set_default_meth(PK_METH *(*fn)(void))
{
    dh_default_meth_hook = fn;
}
