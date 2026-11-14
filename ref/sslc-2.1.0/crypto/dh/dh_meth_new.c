/*
 * DH_new_meth.
 */
/* The DH object of SSL-C: the key is an R_EITEMS list and the operations go through a PK_CTX that
 * DH_new_meth creates. */

#include <string.h>
#include <stdio.h>
#include "cryptlib.h"
#include "r_eitems.h"
#include "pk_int.h"
#include "dh.h"
#include "r_rand.h"

DH *DH_new_meth(PK_METH *meth)
{
    DH *ret;
    DH *result;

    ret = 0;
    result = 0;

    if (meth == 0) {
        ERR_put_error(5, 105, 14, __FILE__, 47);
        return 0;
    } else if ((ret = (DH *)R_malloc(24)) == 0) {
        ERR_put_error(5, 105, 33, __FILE__, 54);
        return 0;
    }

    memset(ret, 0, 24);
    ret->meth = meth;

    if ((ret->pk_ctx = PK_CTX_new()) == 0)
        goto cleanup;
    if ((ret->eitems = R_EITEMS_new()) == 0)
        goto cleanup;
    ret->references = 1;
    R_EITEMS_add(ret->eitems, 1, 2, 0, (int)R_rand_get_default(), 0, 0);
    result = ret;
    ret = 0;

cleanup:
    if (ret != 0) {
        if (ret->pk_ctx != 0)
            PK_CTX_free(ret->pk_ctx);
        if (ret->eitems != 0)
            R_EITEMS_free(ret->eitems);
        R_free(ret);
    }
    return result;
}
