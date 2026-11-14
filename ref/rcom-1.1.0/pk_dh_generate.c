/*
 * pk_dh_sslc_generate_key.
 */
#include "pk_dh_impl.h"

int pk_dh_sslc_generate_key(PK_CTX *ctxArg, int op, void *unusedA, int flags)
{
    int ret;
    DhImpl *dh;
    BN_CTX *ctx;

    ret = 1;
    ctx = 0;

    if (op != 5)
        return 2;

    if ((ctx = BN_CTX_new()) == 0)
        return 1;

    dh = ctxArg->impl;

    if (!(dh->flags & 0x1))
        return 3;

    if (dh->rng == 0)
        return 4;

    if (!(flags & 1) || !(dh->flags & 0x4)) {
        BN_rand(&dh->privKey, dh->rng, dh->privBits, 1, 0);
    }

    ret = BN_ME_CTX_mod_exp(dh->modExpCtx, &dh->pubKey, &dh->g, &dh->privKey, 0, ctx);
    if (ret != 0) {
    } else {
        dh->flags = dh->flags | 0x6;
        ret = 0;
    }

    if (ctx != 0)
        BN_CTX_free(ctx);

    return ret;
}
