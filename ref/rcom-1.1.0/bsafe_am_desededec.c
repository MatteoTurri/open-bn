/*
 * AM_DES_EDE_DECRYPT: the triple DES (EDE) decryption method. The context holds three key
 * schedules of 32 words; the key is three 8-byte DES keys.
 */
#include "bsafe_amtype.h"
#include "../../src/rcom/descore.h"
#include "../../src/rcom/desimpl.h"
#include "bsafe_aitype.h"
#include "bsafe_kitype.h"


static int DESEDEDecryptQuery(unsigned int *contextLen)
{
    *contextLen = 384;
    return 0;
}

static int DESEDEDecryptSetup(DES_LONG *ctx, DES_LONG *key)
{
    DESDecryptInit(ctx + 64, key);
    DESEncryptInit(ctx + 32, key + 2);
    DESDecryptInit(ctx, key + 4);
    return 0;
}

static int DESEDEDecryptBlock(void *ctx, void *out, void *in)
{
    DES_EDE(ctx, out, in);
    return 0;
}

static int DESEDEDecryptRestart(void)
{
    return 9;
}

static int DESEDEDecryptBlockLen(void *ctx, unsigned int *blockLen)
{
    *blockLen = 8;
    return 0;
}

static A_BlockCipherAlga A_DES_EDE_DECRYPT_ALGA = {
    DESEDEDecryptQuery, DESEDEDecryptSetup, DESEDEDecryptRestart, DESEDEDecryptBlock,
    DESEDEDecryptBlockLen, 0, 5
};

B_AlgorithmMethod AM_DES_EDE_DECRYPT = { &AIT_DES_EDE, 0, &KIT_24Byte, &A_DES_EDE_DECRYPT_ALGA };
