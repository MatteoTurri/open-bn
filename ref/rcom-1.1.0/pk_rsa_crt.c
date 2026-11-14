/*
 * pk_rsa_crt_sslc.
 */

/* The RSA private key operation by CRT (Garner) over the factors in rsa->primes: the first factor
 * gives result directly; for each next one the product of the previous moduli (bnAcc), m_i =
 * base^d_i mod p_i, the Garner correction with the CRT coefficient, accumulated into result. */

#include "pk_rsa_impl.h"
#include "bn_me.h"


int pk_rsa_crt_sslc(BIGNUM *result, BIGNUM *base, RsaImpl *rsa)
{
    int i;
    int j;
    BIGNUM bnA;
    BIGNUM bnAcc;
    BIGNUM bnT;
    int ret;
    BN_CTX *ctx;

    ret = 1;
    ctx = 0;

    if ((ctx = BN_CTX_new()) == 0)
        goto done;

    ctx->surrender = rsa->surrender;

    BN_init(&bnT);
    BN_init(&bnA);
    BN_init(&bnAcc);

    if (BN_ME_CTX_mod_exp(rsa->primeCtxs[0], result, base, 0, 0, ctx) != 0)
        goto done;

    j = 3;
    i = 1;

    while (i < rsa->primeCount) {
        if (i < 2) {
            BN_copy(&bnAcc, rsa->primes);
        } else {
            BN_mul(&bnAcc, &bnAcc, &rsa->primes[j - 3], ctx);
        }

        if (BN_ME_CTX_mod_exp(rsa->primeCtxs[i], &bnT, base, 0, 0, ctx) != 0)
            goto done;

        if (BN_sub(&bnT, &bnT, result) == 0)
            goto done;

        if (bnT.neg) {
            if (BN_add(&bnT, &bnT, &rsa->primes[j]) == 0)
                goto done;
            if (bnT.neg) {
                if (BN_mod(&bnT, &bnT, &rsa->primes[j], ctx) == 0)
                    goto done;
                if (BN_add(&bnT, &bnT, &rsa->primes[j]) == 0)
                    goto done;
            }
        }

        if (BN_mul(&bnA, &bnT, &rsa->primes[j + 2], ctx) == 0)
            goto done;

        if (BN_mod(&bnT, &bnA, &rsa->primes[j], ctx) == 0)
            goto done;

        if (BN_mul(&bnA, &bnT, &bnAcc, ctx) == 0)
            goto done;

        if (BN_add(result, result, &bnA) == 0)
            goto done;

        i = i + 1;
        j = j + 3;
    }

    ret = 0;

done:
    if (ctx != 0)
        BN_CTX_free(ctx);
    BN_clear_free(&bnT);
    BN_clear_free(&bnA);
    BN_clear_free(&bnAcc);

    return ret;
}
