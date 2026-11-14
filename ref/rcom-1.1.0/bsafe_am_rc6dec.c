/*
 * AM_RC6_DECRYPT: the RC6 decryption method.
 */
#include "bsafe_amtype.h"
#include "rc6.h"
#include "bsafe_aitype.h"
#include "bsafe_kitype.h"


static int RC6DecryptQuery(unsigned int *contextLen, void *unused, unsigned int *pver)
{
    if (*pver != 20)
        return 9;
    *contextLen = 196;
    return 0;
}

static int RC6DecryptRestart(void);

static int RC6DecryptInit(RC6_KEY *ctx, ITEM *key, unsigned int *pver)
{
    int (*initFn)(RC6_KEY *, ITEM *, int) = A_RC6Init_20;
    int keyBits = key->len * 8;
    unsigned int ver = *pver;

    ctx->rounds = ver;
    if (ver != 20)
        return 9;
    ctx->encrypt = RC6DecryptRestart;
    ctx->decrypt = A_RC6Decrypt_20;
    switch (keyBits) {
    case 128:
        initFn = A_RC6Init_20_128;
        break;
    case 192:
        initFn = A_RC6Init_20_192;
        break;
    case 256:
        initFn = A_RC6Init_20_256;
        break;
    }
    return initFn(ctx, key, ver);
}

static int RC6DecryptOp(RC6_KEY *ctx, void *out, const void *in)
{
    return ctx->decrypt(ctx, out, in);
}

static int RC6DecryptRestart(void)
{
    return 9;
}

static int RC6DecryptBlockLen(void *ctx, unsigned int *blockLen)
{
    *blockLen = 16;
    return 0;
}

static A_BlockCipherAlga A_RC6_DECRYPT_ALGA = {
    RC6DecryptQuery, RC6DecryptInit, RC6DecryptRestart, RC6DecryptOp,
    RC6DecryptBlockLen, 0, 9
};

B_AlgorithmMethod AM_RC6_DECRYPT = { &AIT_RC6, 0, &KITItem, &A_RC6_DECRYPT_ALGA };
