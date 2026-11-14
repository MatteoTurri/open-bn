/*
 * AM_DES_ENCRYPT: the DES encryption method.
 */
#include "bsafe_amtype.h"
#include "../../src/rcom/descore.h"
#include "bsafe_aitype.h"
#include "bsafe_kitype.h"


static int DESEncryptQuery(unsigned int *contextLen)
{
    *contextLen = 128;
    return 0;
}

static int DESEncryptSetup(DES_LONG *ctx, DES_LONG *key)
{
    DESEncryptInit(ctx, key);
    return 0;
}

static int DESEncryptBlock(DES_LONG *ctx, DES_LONG *out, DES_LONG *in)
{
    DES(ctx, out, in);
    return 0;
}

static int DESEncryptRestart(void)
{
    return 9;
}

static int DESEncryptBlockLen(void *ctx, unsigned int *blockLen)
{
    *blockLen = 8;
    return 0;
}

static A_BlockCipherAlga A_DES_ENCRYPT_ALGA = {
    DESEncryptQuery, DESEncryptSetup, DESEncryptBlock, DESEncryptRestart,
    DESEncryptBlockLen, 0, 1
};

B_AlgorithmMethod AM_DES_ENCRYPT = { &AIT_DES, 1, &KIT_8Byte, &A_DES_ENCRYPT_ALGA };
