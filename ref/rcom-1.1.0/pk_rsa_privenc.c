/*
 * pk_rsa_private_encrypt_sslc.
 */


#include "pk_rsa_impl.h"


#include "bn_me.h"


/* opFn of the RSA method for op 1 and 10 (sign): builds the padded block with padMeth->signFn in
 * outBuf, exponentiates it with the private key through pk_rsa_crt_sslc, and pads the result on the
 * left with zeros, byte by byte. */

int pk_rsa_private_encrypt_sslc(PK_CTX *ctxArg, int op, void *outBuf, int *outLenPtr, int bufCap, void *in, int inLen, int padFlags)
{
    int ret;
    RsaImpl *rsa;
    int modLen;
    int tmp;
    int n;
    BIGNUM bnIn;
    BIGNUM bnOut;

    ret = 1;

    if (op != 1 && op != 10)
        return 2;

    BN_init(&bnIn);
    BN_init(&bnOut);

    rsa = ctxArg->impl;

    modLen = (BN_num_bits(&rsa->n) + 7) / 8;

    if ((unsigned int)bufCap < (unsigned int)modLen) {
        ret = 1;
        goto done;
    }

    if ((tmp = rsa->padMeth->signFn(rsa->rng, outBuf, modLen, in, inLen, padFlags)) != 0)
        goto done;

    BN_bin2bn(outBuf, modLen, &bnIn);

    if ((tmp = pk_rsa_crt_sslc(&bnOut, &bnIn, rsa)) != 0)
        goto done;

    n = (BN_num_bits(&bnOut) + 7) / 8;
    if (n > modLen)
        goto done;

    tmp = BN_bn2bin(&bnOut, (char *)outBuf + (modLen - n));

    n = 0;
    while (n < modLen - tmp) {
        ((unsigned char *)outBuf)[n] = 0;
        n = n + 1;
    }

    *outLenPtr = modLen;
    ret = 0;

done:
    BN_free(&bnOut);
    BN_free(&bnIn);

    return ret;
}
