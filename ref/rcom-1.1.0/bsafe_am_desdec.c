/*
 * AM_DES_DECRYPT: the DES decryption method.
 */
#include "bsafe_amtype.h"
#include "../../src/rcom/descore.h"
#include "bsafe_aitype.h"
#include "bsafe_kitype.h"


static int DESDecryptQuery(unsigned int *contextLen)
{
    *contextLen = 128;
    return 0;
}

static int DESDecryptSetup(DES_LONG *ctx, DES_LONG *key)
{
    DESDecryptInit(ctx, key);
    return 0;
}

static int DESDecryptBlock(DES_LONG *ctx, DES_LONG *out, DES_LONG *in)
{
    DES(ctx, out, in);
    return 0;
}

static int DESDecryptRestart(void)
{
    return 9;
}

static int DESDecryptBlockLen(void *ctx, unsigned int *blockLen)
{
    *blockLen = 8;
    return 0;
}

static A_BlockCipherAlga A_DES_DECRYPT_ALGA = {
    DESDecryptQuery, DESDecryptSetup, DESDecryptRestart, DESDecryptBlock,
    DESDecryptBlockLen, 0, 1
};

B_AlgorithmMethod AM_DES_DECRYPT = { &AIT_DES, 0, &KIT_8Byte, &A_DES_DECRYPT_ALGA };
