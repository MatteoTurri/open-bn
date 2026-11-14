/*
 * AM_RC6_ENCRYPT: the RC6 encryption method.
 */
#include "bsafe_amtype.h"
#include "rc6.h"
#include "bsafe_aitype.h"
#include "bsafe_kitype.h"


static int RC6EncryptQuery(unsigned int *contextLen, void *unused, unsigned int *pver)
{
    if (*pver != 20)
        return 9;
    *contextLen = 196;
    return 0;
}

static int RC6EncryptOp(RC6_KEY *ctx, void *out, const void *in);

static int RC6EncryptInit(RC6_KEY *ctx, ITEM *key, unsigned int *pver)
{
    int (*initFn)(RC6_KEY *, ITEM *, int) = A_RC6Init_20;
    int keyBits = key->len * 8;
    unsigned int ver = *pver;

    ctx->rounds = ver;
    if (ver != 20)
        return 9;
    ctx->encrypt = A_RC6Encrypt_20;
    ctx->decrypt = RC6EncryptOp;
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

static int RC6EncryptOp(RC6_KEY *ctx, void *out, const void *in)
{
    return ctx->encrypt(ctx, out, in);
}

static int RC6EncryptRestart(void)
{
    return 9;
}

static int RC6EncryptBlockLen(void *ctx, unsigned int *blockLen)
{
    *blockLen = 16;
    return 0;
}

static A_BlockCipherAlga A_RC6_ENCRYPT_ALGA = {
    RC6EncryptQuery, RC6EncryptInit, RC6EncryptOp, RC6EncryptRestart,
    RC6EncryptBlockLen, 0, 9
};

B_AlgorithmMethod AM_RC6_ENCRYPT = { &AIT_RC6, 1, &KITItem, &A_RC6_ENCRYPT_ALGA };
