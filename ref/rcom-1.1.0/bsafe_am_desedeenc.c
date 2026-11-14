/*
 * AM_DES_EDE_ENCRYPT: the triple DES (EDE) encryption method. The context holds three key
 * schedules of 32 words; the key is three 8-byte DES keys.
 */
#include "bsafe_amtype.h"
#include "../../src/rcom/descore.h"
#include "../../src/rcom/desimpl.h"
#include "bsafe_aitype.h"
#include "bsafe_kitype.h"


static int DESEDEEncryptQuery(unsigned int *contextLen)
{
    *contextLen = 384;
    return 0;
}

static int DESEDEEncryptSetup(DES_LONG *ctx, DES_LONG *key)
{
    DESEncryptInit(ctx, key);
    DESDecryptInit(ctx + 32, key + 2);
    DESEncryptInit(ctx + 64, key + 4);
    return 0;
}

static int DESEDEEncryptBlock(void *ctx, void *out, void *in)
{
    DES_EDE(ctx, out, in);
    return 0;
}

static int DESEDEEncryptRestart(void)
{
    return 9;
}

static int DESEDEEncryptBlockLen(void *ctx, unsigned int *blockLen)
{
    *blockLen = 8;
    return 0;
}

static A_BlockCipherAlga A_DES_EDE_ENCRYPT_ALGA = {
    DESEDEEncryptQuery, DESEDEEncryptSetup, DESEDEEncryptBlock, DESEDEEncryptRestart,
    DESEDEEncryptBlockLen, 0, 5
};

B_AlgorithmMethod AM_DES_EDE_ENCRYPT = { &AIT_DES_EDE, 1, &KIT_24Byte, &A_DES_EDE_ENCRYPT_ALGA };
