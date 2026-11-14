/*
 * pk_rsa_public_decrypt_sslc.
 */


#include "pk_rsa_impl.h"


#include "bn_me.h"


/* opFn of the RSA method for op 4 and 9: checks the two lengths against the length of the modulus,
 * converts in/inLen to a BIGNUM, exponentiates it with the public context, converts it back and
 * removes the PKCS #1 padding with padMeth->unpadFn. */
int pk_rsa_public_decrypt_sslc(PK_CTX *ctxArg, int op, void *outBuf, int *outLenPtr, int bufCap, void *in, int inLen, int padFlags)
{
    int ret;
    RsaImpl *rsa;
    int modLen;
    int decodedLen;
    BIGNUM bnIn;
    BIGNUM bnOut;
    BN_CTX *ctx;

    ret = 1;
    ctx = 0;

    if (op != 4 && op != 9)
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

    if ((ctx = BN_CTX_new()) == 0)
        goto done;

    ctx->surrender = rsa->surrender;

    if (BN_bin2bn(in, inLen, &bnIn) == 0)
        goto done;

    ret = BN_ME_CTX_mod_exp(rsa->modExpCtx, &bnOut, &bnIn, 0, 0, ctx);
    if (ret != 0)
        goto done;

    decodedLen = BN_bn2bin(&bnOut, outBuf);
    if (decodedLen > modLen)
        goto done;

    if ((ret = rsa->padMeth->unpadFn(outBuf, outLenPtr, outBuf, decodedLen, modLen, padFlags)) != 0)
        goto done;
    ret = 0;

done:
    if (ctx != 0)
        BN_CTX_free(ctx);
    BN_free(&bnOut);
    BN_free(&bnIn);

    return ret;
}
