/*
 * pk_rsa_sslc_cleanup.
 */


#include "bn_me.h"
#include "r_com.h"

#include "pk_rsa_impl.h"

int pk_rsa_sslc_cleanup(PK_CTX *ctxArg)
{
    RsaImpl *rsa;
    int i;
    int count;

    if (ctxArg->impl == 0)
        return 0;
    rsa = ctxArg->impl;
    count = rsa->primeCount;
    BN_free(&rsa->n);
    BN_free(&rsa->e);
    BN_free(&rsa->d);
    i = 0;
    while (i < count) {
        BN_free(&rsa->primes[i * 3]);
        BN_free(&rsa->primes[i * 3 + 1]);
        BN_free(&rsa->primes[i * 3 + 2]);
        if (rsa->primeCtxs[i] != 0)
            BN_ME_CTX_free(rsa->primeCtxs[i]);
        i = i + 1;
    }
    if (rsa->primes != 0)
        R_free(rsa->primes);
    if (rsa->primeCtxs != 0)
        R_free(rsa->primeCtxs);
    if (rsa->modExpCtx != 0)
        BN_ME_CTX_free(rsa->modExpCtx);
    R_free(rsa);
    return 0;
}
