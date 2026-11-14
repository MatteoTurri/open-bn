/*
 * pk_dh_sslc_cleanup.
 */
#include "pk_dh_impl.h"

int pk_dh_sslc_cleanup(PK_CTX *ctxArg)
{
    DhImpl *dh;

    if (ctxArg->impl == 0)
        return 0;
    dh = ctxArg->impl;
    BN_free(&dh->p);
    BN_free(&dh->g);
    BN_free(&dh->pubKey);
    BN_free(&dh->privKey);
    if (dh->modExpCtx != 0)
        BN_ME_CTX_free(dh->modExpCtx);
    R_free(dh);
    return 0;
}
