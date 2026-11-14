/*
 * pk_rsa_sslc_init.
 */


#include "pk_rsa_impl.h"
#include "bn_me.h"
#include "r_eitems.h"


/* The initFn of the RSA method: allocates the RsaImpl (92 bytes; an existing one goes through
 * pk_rsa_sslc_cleanup) and takes the padding methods of the method table. Two R_EITEMS_check_list
 * calls set the flags of the context: 4 when the multi-prime private key is complete, 1 when n and e
 * are there. With a private key it loads n, the factors (p, the CRT exponent and, from the second
 * factor on, the CRT coefficient) and d, and builds a modular exponentiation context per factor;
 * then it always loads n and, with flag 1, e and the context of the whole modulus. */

#include "r_eitem.h"


static int g_rsa_full_types[] = { 16, 18, 22, 23, 19, 20, 24, 0 };
static int g_rsa_min_types[] = { 16, 17, 0 };

int pk_rsa_sslc_init(PK_CTX *ctxArg, void *eitems, int unused3, int unused4)
{
    R_EITEM *item;
    RsaImpl *rsa;
    int count;
    int i;
    int n;
    int ret;
    BIGNUM *primes;
    void **primeCtxs;
    BN_CTX *ctx;

    n = 0;
    ret = 1;
    primes = 0;
    primeCtxs = 0;
    ctx = 0;

    rsa = ctxArg->impl;
    if (rsa != 0)
        pk_rsa_sslc_cleanup(ctxArg);

    rsa = (RsaImpl *)R_malloc(92);
    if (rsa == 0)
        return 1;

    memset(rsa, 0, 92);
    ctxArg->impl = rsa;

    i = 0;
    ctxArg->flags = ctxArg->flags & ~5;

    if (R_EITEMS_check_list(eitems, 16, g_rsa_full_types) == 0) {
        ctxArg->flags = ctxArg->flags | 4;
        i = 1;
    }
    if (R_EITEMS_check_list(eitems, 16, g_rsa_min_types) == 0) {
        ctxArg->flags = ctxArg->flags | 1;
        i = 1;
    }
    if (i == 0)
        return 4;

    rsa->padMeth = ctxArg->methods->padding;

    i = 0;
    R_EITEMS_find_item(eitems, 1, 2, &i, &item, 4);
    rsa->rng = (item != 0) ? item->data : 0;

    R_EITEMS_find_item(eitems, 1, 3, &i, &item, 4);
    rsa->surrender = (int)((item != 0) ? item->data : 0);

    if ((ctx = BN_CTX_new()) == 0)
        goto done;

    ctx->surrender = rsa->surrender;

    if (ctxArg->flags & 4) {
        i = 0;
        R_EITEMS_find_item(eitems, 16, 1, &i, &item, 4);
        if (item == 0) {
            ret = 2;
            goto done;
        }

        count = item->length;
        rsa->primeCount = count;

        if ((primes = (BIGNUM *)R_malloc(count * 60)) == 0)
            goto done;

        for (i = 0; i < count * 3; i++)
            BN_init(&primes[i]);

        if ((primeCtxs = (void **)R_malloc(count * 4)) == 0)
            goto done;

        rsa->primeCtxs = primeCtxs;

        for (i = 0; i < count; i++)
            BN_ME_CTX_new(0, &primeCtxs[i]);

        rsa->primes = primes;

        R_EITEMS_find_item(eitems, 16, 18, &i, &item, 4);
        BN_bin2bn(item->data, item->length, &rsa->d);

        i = 0;
        while (i < count) {
            R_EITEMS_find_item(eitems, 16, i * 3 + 19, &n, &item, 4);
            BN_bin2bn(item->data, item->length, &primes[i * 3]);

            R_EITEMS_find_item(eitems, 16, i * 3 + 20, &n, &item, 4);
            if (item == 0)
                goto done;
            BN_bin2bn(item->data, item->length, &primes[i * 3 + 1]);

            if (i != 0) {
                R_EITEMS_find_item(eitems, 16, i * 3 + 21, &n, &item, 4);
                if (item == 0)
                    goto done;
                BN_bin2bn(item->data, item->length, &primes[i * 3 + 2]);
            }

            if (BN_ME_CTX_set(primeCtxs[i], &primes[i * 3], 1, 0, ctx) != 0)
                goto done;
            if (BN_ME_CTX_set(primeCtxs[i], &primes[i * 3 + 1], 3, 0, ctx) != 0)
                goto done;

            i = i + 1;
        }
    }

    i = 0;
    R_EITEMS_find_item(eitems, 16, 16, &i, &item, 4);
    BN_bin2bn(item->data, item->length, &rsa->n);

    if (ctxArg->flags & 1) {
        R_EITEMS_find_item(eitems, 16, 17, &i, &item, 4);
        if (item == 0)
            goto done;

        BN_bin2bn(item->data, item->length, &rsa->e);

        BN_ME_CTX_new(0, &rsa->modExpCtx);

        if (BN_ME_CTX_set(rsa->modExpCtx, &rsa->n, 1, 0, ctx) != 0)
            goto done;

        if (BN_ME_CTX_set(rsa->modExpCtx, &rsa->e, 3, 0, ctx) != 0)
            goto done;
    }

    ret = 0;

done:
    if (ctx != 0)
        BN_CTX_free(ctx);

    return ret;
}
