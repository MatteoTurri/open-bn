/*
 * DH_free.
 */
/* The DH object of SSL-C: the key is an R_EITEMS list and the operations go through a PK_CTX that
 * DH_new_meth creates. */

#include <stdio.h>
#include "cryptlib.h"
#include "r_eitems.h"
#include "pk_int.h"
#include "dh.h"

void DH_free(DH *dh)
{
    int refs;

    if (dh == 0)
        return;

    refs = R_locked_add(&dh->references, -1, 23, __FILE__, 112);
    if (refs > 0)
        return;

    if (dh->pk_ctx != 0)
        PK_CTX_free(dh->pk_ctx);
    if (dh->eitems != 0)
        R_EITEMS_free(dh->eitems);
    R_free(dh);
}
