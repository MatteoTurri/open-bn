/*
 * The CBC feedback mode of BSAFE (AM_CBC_ENCRYPT, AM_CBC_DECRYPT). The context is 16 bytes: the
 * reference IV, the current IV (realigned on the reference by CBCFinal), a block of scratch and the
 * IV length. The info is the {data, len} that AIT_CBCAddInfo stores in the cache.
 */
#include "bsafe.h"
#include "bsafe_amtype.h"
#include "bsafe_aitype.h"


typedef struct CbcContext {
    void *iv;
    void *ivWork;
    void *block;
    unsigned int len;
} CbcContext;

int CBCQuery(unsigned int *contextLen)
{
    *contextLen = 16;
    return 0;
}

int CBCInit(CbcContext *ctx, void *unused, ITEM *info)
{
    unsigned int len = info->len;
    ctx->len = len;
    ctx->iv = T_malloc(len);
    if (ctx->iv == 0)
        return 16;
    ctx->ivWork = T_malloc(len);
    if (ctx->ivWork == 0)
        return 16;
    ctx->block = T_malloc(len);
    if (ctx->block == 0)
        return 16;
    T_memcpy(ctx->iv, info->data, len);
    T_memcpy(ctx->ivWork, info->data, len);
    return 0;
}

static int CBCEncryptUpdate(CbcContext *ctx, A_BlockCipherAlga *ops, void *algCtx,
                            unsigned char *output, unsigned int *outputLen,
                            const unsigned char *input, unsigned int inputLen)
{
    unsigned int blockLen = ctx->len;
    unsigned int words = blockLen >> 2;
    unsigned int total = 0;
    unsigned char *block = (unsigned char *)ctx->block;
    unsigned int i;

    while (inputLen >= blockLen) {
        for (i = 0; i < blockLen; i += 8) {
            unsigned int w0, w1;
            w0 = *(const unsigned int *)(input + i);
            *(unsigned int *)(block + i) = w0;
            w1 = *(const unsigned int *)(input + i + 4);
            *(unsigned int *)(block + i + 4) = w1;
        }
        for (i = 0; i < words; i++)
            ((unsigned int *)block)[i] ^= ((unsigned int *)ctx->ivWork)[i];
        ops->Encrypt(algCtx, block, block);
        for (i = 0; i < words; i++)
            ((unsigned int *)ctx->ivWork)[i] = ((unsigned int *)block)[i];
        for (i = 0; i < blockLen; i += 8) {
            *(unsigned int *)(output + i) = *(const unsigned int *)(block + i);
            *(unsigned int *)(output + i + 4) = *(const unsigned int *)(block + i + 4);
        }
        input += blockLen;
        inputLen -= blockLen;
        output += blockLen;
        total += blockLen;
    }
    *outputLen = total;
    return 0;
}

static int CBCDecryptUpdate(CbcContext *ctx, A_BlockCipherAlga *ops, void *algCtx,
                            unsigned char *output, unsigned int *outputLen,
                            const unsigned char *input, unsigned int inputLen)
{
    unsigned char *block = (unsigned char *)ctx->block;
    unsigned int blockLen = ctx->len;
    unsigned int words = blockLen >> 2;
    unsigned int i;
    unsigned int total = 0;

    while (inputLen >= blockLen) {
        for (i = 0; i < blockLen; i += 8) {
            unsigned int w0, w1;
            w0 = *(const unsigned int *)(input + i);
            *(unsigned int *)(block + i) = w0;
            w1 = *(const unsigned int *)(input + i + 4);
            *(unsigned int *)(block + i + 4) = w1;
        }
        ops->Decrypt(algCtx, block, block);
        for (i = 0; i < words; i++)
            ((unsigned int *)block)[i] ^= ((unsigned int *)ctx->ivWork)[i];
        for (i = 0; i < words; i++)
            ((unsigned int *)ctx->ivWork)[i] = ((const unsigned int *)input)[i];
        for (i = 0; i < blockLen; i += 8) {
            *(unsigned int *)(output + i) = *(const unsigned int *)(block + i);
            *(unsigned int *)(output + i + 4) = *(const unsigned int *)(block + i + 4);
        }
        input += blockLen;
        inputLen -= blockLen;
        output += blockLen;
        total += blockLen;
    }
    *outputLen = total;
    return 0;
}

int CBCFinal(CbcContext *ctx)
{
    T_memcpy(ctx->ivWork, ctx->iv, ctx->len);
    return 0;
}

int CBCGetBlockLen(CbcContext *ctx, unsigned int *blockLen)
{
    unsigned int len = ctx->len;
    *blockLen = len;
    return len ? 0 : 11;
}

static int CBCContextDestructor(CbcContext *ctx);

int CBCSetContextDestructor(BsafePoolEntry *entry)
{
    entry->destructor = (BsafeDestructorFn)CBCContextDestructor;
    return 0;
}

static int CBCContextDestructor(CbcContext *ctx)
{
    if (ctx == 0)
        return 0;
    if (ctx->block != 0) {
        T_memset(ctx->block, 0, ctx->len);
        T_free(ctx->block);
    }
    if (ctx->iv != 0) {
        T_memset(ctx->iv, 0, ctx->len);
        T_free(ctx->iv);
    }
    if (ctx->ivWork != 0) {
        T_memset(ctx->ivWork, 0, ctx->len);
        T_free(ctx->ivWork);
    }
    return 0;
}

int CBCResetIV(CbcContext *ctx, ITEM *info)
{
    if (ctx->len != info->len)
        return 6;
    T_memcpy(ctx->iv, info->data, ctx->len);
    return 0;
}

/* The two method tables, each one just before its AM. */
static A_FeedbackAlga A_CBC_ENCRYPT_ALGA = {
    CBCQuery, CBCInit, CBCResetIV, CBCEncryptUpdate, CBCFinal, CBCGetBlockLen,
    CBCSetContextDestructor
};

B_AlgorithmMethod AM_CBC_ENCRYPT = { &AIT_CBC, 1, 0, &A_CBC_ENCRYPT_ALGA };

static A_FeedbackAlga A_CBC_DECRYPT_ALGA = {
    CBCQuery, CBCInit, CBCResetIV, CBCDecryptUpdate, CBCFinal, CBCGetBlockLen,
    CBCSetContextDestructor
};

B_AlgorithmMethod AM_CBC_DECRYPT = { &AIT_CBC, 0, 0, &A_CBC_DECRYPT_ALGA };
