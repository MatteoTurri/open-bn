/*
 * RSA_new_meth of the SSL-C fork.
 */

#include <string.h>
#include <stdio.h>
#include "cryptlib.h"
#include "r_eitems.h"
#include "pk_int.h"
#include "rsa.h"
#include "rsa_int.h"
#include "r_rand.h"

RSA *RSA_new_meth(PK_METH *meth)
{
    RSA *ret;
    RSA *result;
    void *eitems;
    void *rnd;

    ret = 0;
    result = 0;

    if (meth == 0) {
#line 175 /* the line number is part of the binary */
        ERR_put_error(4, 107, 14, __FILE__, __LINE__);
        return 0;
    } else if ((ret = (RSA *)R_malloc(28)) == 0) {
#line 182 /* the line number is part of the binary */
        ERR_put_error(4, 107, 33, __FILE__, __LINE__);
        return 0;
    }

    memset(ret, 0, 28);
    ret->references = 1;
    ret->meth = meth;
    ret->pk_ctx = 0;
    ret->extra_buf = 0;

    if ((ret->eitems = R_EITEMS_new()) == 0) goto err;

    ret->references = 1;
    R_EITEMS_add(ret->eitems, 1, 2, 0, (int)R_rand_get_default(), 0, 0);
    R_EITEMS_add(ret->eitems, 1, 3, 0, 0, 0, 0);
    result = ret;
    ret = 0;
err:
    if (ret != 0) {
        if (ret->eitems != 0)
            R_EITEMS_free(ret->eitems);
        R_free(ret);
    }
    return result;
}
