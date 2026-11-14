/*
 * AM_RSA_ENCRYPT and AM_RSA_DECRYPT: the PKCS RSA methods, one table for both.
 */
#include "bsafe.h"
#include "bsafe_amtype.h"
#include "alg.h"
#include "bsafe_aitype.h"
#include "bsafe_kitype.h"


static int PKCSRSAQuery(unsigned int *contextLen, ITEM *key)
{
    if (A_IntegerBits(key->data, key->len) > 2048)
        return 7;
    *contextLen = 64;
    return 0;
}

static int PKCSRSAInit(ALG_RSAContext *ctx, A_RSA_KEY *info)
{
    A_SetModularOperations(ctx->modOps, 0, 0, 0, 0, 0, 0, 0);
    return ALG_RSAInit(ctx, info);
}

static int PKCSRSAUpdate(ALG_RSAContext *ctx, unsigned char *out, unsigned int *outLen, int maxOutLen,
                         const unsigned char *in, unsigned int inLen, void *surrender)
{
    return ALG_RSAUpdate(ctx, out, outLen, maxOutLen, in, inLen, surrender);
}

static int PKCSRSAFinal(ALG_RSAContext *ctx, void *out, unsigned int *outLen, void *maxOutLen)
{
    *outLen = 0;
    return ALG_RSAFinal(ctx);
}

static int PKCSRSAOutputLen(unsigned int *ctx, unsigned int *outLen, unsigned int inLen)
{
    unsigned int blockLen = *ctx;

    if (inLen % blockLen != 0) {
        inLen += blockLen - inLen % blockLen;
        *outLen = inLen;
    } else {
        *outLen = inLen;
    }
    return 0;
}

static int PKCSRSABlockLen(unsigned int *ctx, unsigned int *blockLen)
{
    *blockLen = *ctx;
    return 0;
}

static int PKCSRSASetContextDestructor(BsafePoolEntry *entry)
{
    entry->destructor = (BsafeDestructorFn)ALG_RSAContextDestroy;
    return 0;
}


static A_PublicKeyAlga A_PKCS_RSA_ALGA = {
    PKCSRSAQuery, PKCSRSAInit, PKCSRSAUpdate, PKCSRSAFinal, PKCSRSAOutputLen,
    PKCSRSABlockLen, PKCSRSASetContextDestructor, 0
};

B_AlgorithmMethod AM_RSA_DECRYPT = { &AIT_RSAPublic, 0, &KIT_RSAPublic, &A_PKCS_RSA_ALGA };
B_AlgorithmMethod AM_RSA_ENCRYPT = { &AIT_RSAPublic, 1, &KIT_RSAPublic, &A_PKCS_RSA_ALGA };
