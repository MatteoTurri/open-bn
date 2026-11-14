/*
 * RSA_dup, RSA_flags and rsa_get_pk_ctx of the SSL-C fork.
 */
#define RSA_PK_FILE "./pubkey/rsa/../../pubkey/rsa/rsa_pk.c"
/* SSL-C: the RSA of the fork keeps its key as an R_EITEMS list and its method as a PK_METH;
 * pk_ctx is created on first use, under the lock. */

#include <stdio.h>
#include "cryptlib.h"
#include "r_eitems.h"
#include "pk_int.h"
#include "rsa.h"
#include "rsa_int.h"

/* ---- costruttore/distruttore reali ---- */

RSA *RSA_dup(RSA *r)
{
    RSA *ret = 0;

    ret = RSA_new_meth(r->meth);
    if (ret == 0)
        goto err;

    if (!R_EITEMS_dup(ret->eitems, r->eitems, 18))
        goto err;

    if (r->extra_buf != 0) {
        ret->extra_buf = (RSA_EXTRA *)R_malloc(12);
        if (ret->extra_buf == 0)
            goto err;
        ret->extra_buf->unk_00 = r->extra_buf->unk_00;
        ret->extra_buf->unk_04 = r->extra_buf->unk_04;
        ret->extra_buf->unk_08 = r->extra_buf->unk_08;
    }
    return ret;

err:
    if (ret != 0)
        RSA_free(ret);
    return 0;
}

int RSA_flags(RSA *r)
{
    return (r != 0) ? r->flags : 0;
}

void *rsa_get_pk_ctx(RSA *rsa)
{
    void *ctx = 0;

    if (rsa->pk_ctx == 0) {
        R_lock_ctrl(9, 9, RSA_PK_FILE, 850);

        if (rsa->pk_ctx != 0) {
            ctx = rsa->pk_ctx;
        } else {
            ctx = PK_CTX_new();
            if (ctx == 0)
                ;
            else if (PK_CTX_set(ctx, rsa->meth, rsa->eitems, 0, 0) != 0) {
                PK_CTX_free(ctx);
                ctx = 0;
            } else {
                rsa->pk_ctx = ctx;
            }
        }

        R_lock_ctrl(10, 9, RSA_PK_FILE, 871);
    } else {
        ctx = rsa->pk_ctx;
    }
    return ctx;
}
