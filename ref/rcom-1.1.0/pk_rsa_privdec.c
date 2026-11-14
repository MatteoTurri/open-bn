/*
 * pk_rsa_private_decrypt_sslc and pk_rsa_public_encrypt_sslc.
 */


#include "pk_rsa_impl.h"


#include "bn_me.h"


/* opFn of the RSA method for op 2: the exponentiation with the private key goes through
 * pk_rsa_crt_sslc; the padding is removed by padMeth->privUnpadFn into a local length, copied to
 * *outLenPtr afterwards. */


int pk_rsa_private_decrypt_sslc(PK_CTX *ctxArg, int op, void *outBuf, int *outLenPtr, int bufCap, void *in, int inLen, int padFlags)
{
    int ret;
    RsaImpl *rsa;
    int modLen;
    int tmp;
    BIGNUM bnIn;
    BIGNUM bnOut;

    ret = 1;

    if (op != 2)
        return 2;

    BN_init(&bnIn);
    BN_init(&bnOut);

    rsa = ctxArg->impl;

    modLen = (BN_num_bits(&rsa->n) + 7) / 8;

    if ((unsigned int)bufCap < (unsigned int)modLen) {
        ret = 1;
        goto done;
    }
    if ((unsigned int)inLen > (unsigned int)modLen) {
        ret = 3;
        goto done;
    }

    BN_bin2bn(in, inLen, &bnIn);

    if ((tmp = pk_rsa_crt_sslc(&bnOut, &bnIn, rsa)) != 0)
        goto done;

    tmp = BN_bn2bin(&bnOut, outBuf);

    tmp = rsa->padMeth->privUnpadFn(outBuf, &modLen, outBuf, tmp, modLen, padFlags);
    if (tmp != 0)
        goto done;
    *outLenPtr = modLen;
    ret = 0;

done:
    BN_free(&bnOut);
    BN_free(&bnIn);

    return ret;
}

/* opFn of the RSA method for op 3: pads the input with padMeth->padFn into outBuf, exponentiates it
 * with the public context and pads the result on the left with zeros. */

int pk_rsa_public_encrypt_sslc(PK_CTX *ctxArg, int op, void *outBuf, int *outLenPtr, int bufCap, void *in, int inLen, int padFlags)
{
    int ret;
    RsaImpl *rsa;
    int modLen;
    int tmp;
    int n;
    BIGNUM bnIn;
    BIGNUM bnOut;
    BN_CTX *ctx;

    ret = 1;
    ctx = 0;

    if (op != 3)
        return 2;

    BN_init(&bnIn);
    BN_init(&bnOut);

    rsa = ctxArg->impl;

    modLen = (BN_num_bits(&rsa->n) + 7) / 8;

    if ((unsigned int)bufCap < (unsigned int)modLen) {
        ret = 1;
        goto done;
    }
    if ((unsigned int)inLen > (unsigned int)modLen) {
        ret = 1;
        goto done;
    }

    if ((ret = rsa->padMeth->padFn(rsa->rng, outBuf, modLen, in, inLen, padFlags)) != 0)
        goto done;

    if (BN_bin2bn(outBuf, modLen, &bnIn) == 0)
        goto done;

    if ((ctx = BN_CTX_new()) == 0) {
        ret = 1;
        goto done;
    }

    ctx->surrender = rsa->surrender;

    if ((ret = BN_ME_CTX_mod_exp(rsa->modExpCtx, &bnOut, &bnIn, 0, 0, ctx)) != 0)
        goto done;

    n = (BN_num_bits(&bnOut) + 7) / 8;
    tmp = BN_bn2bin(&bnOut, (char *)outBuf + (modLen - n));
    if (n > modLen)
        goto done;

    for (tmp = 0; tmp < modLen - n; tmp++)
        ((unsigned char *)outBuf)[tmp] = 0;

    *outLenPtr = modLen;
    ret = 0;

done:
    if (ctx != 0)
        BN_CTX_free(ctx);
    BN_free(&bnOut);
    BN_free(&bnIn);

    return ret;
}
