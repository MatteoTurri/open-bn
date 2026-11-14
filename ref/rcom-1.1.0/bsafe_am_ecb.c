/*
 * AM_ECB_ENCRYPT and AM_ECB_DECRYPT: the ECB feedback mode.
 */
#include "bsafe.h"
#include "bsafe_amtype.h"
#include "bsafe_aitype.h"


/* ECB: the context is {void *block; unsigned int len;} */
typedef struct EcbContext {
    void *block;
    unsigned int len;
} EcbContext;

static int ECBQuery(unsigned int *contextLen)
{
    *contextLen = 8;
    return 0;
}

static int ECBInit(EcbContext *ctx, void *unused, const unsigned char *info)
{
    unsigned int len = *info;
    void *p = T_malloc(len);

    ctx->block = p;
    if (p == 0)
        return 16;
    ctx->len = len;
    return 0;
}

/* Copies a cipher block by 32-bit words (len a multiple of 8). */
#define CBC_COPY_BLOCK(d, s, len) {                                 \
        unsigned char *dst = (d);                                    \
        const unsigned char *src = (s);                              \
                                                                     \
        for (i = 0; i < (len); i += 8) {                             \
            unsigned int w0, w1;                                     \
            w0 = *(const unsigned int *)(src + i);                   \
            *(unsigned int *)(dst + i) = w0;                         \
            w1 = *(const unsigned int *)(src + i + 4);               \
            *(unsigned int *)(dst + i + 4) = w1;                     \
        }                                                            \
    }

static int ECBEncryptUpdate(EcbContext *ctx, A_BlockCipherAlga *ops, void *algCtx,
                            unsigned char *output, unsigned int *outputLen,
                            const unsigned char *input, unsigned int inputLen)
{
    unsigned int blockLen = ctx->len;
    unsigned char *block = (unsigned char *)ctx->block;
    unsigned int i;
    unsigned int total = 0;

    while (inputLen >= blockLen) {
        CBC_COPY_BLOCK(block, input, blockLen);
        ops->Encrypt(algCtx, block, block);
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

static int ECBDecryptUpdate(EcbContext *ctx, A_BlockCipherAlga *ops, void *algCtx,
                            unsigned char *output, unsigned int *outputLen,
                            const unsigned char *input, unsigned int inputLen)
{
    unsigned int blockLen = ctx->len;
    unsigned int total = 0;
    unsigned char *block = (unsigned char *)ctx->block;
    unsigned int i;

    while (inputLen >= blockLen) {
        CBC_COPY_BLOCK(block, input, blockLen);
        ops->Decrypt(algCtx, block, block);
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

/* Final has nothing to realign: there is no IV. */
static int ECBFinal(EcbContext *ctx)
{
    return 0;
}

static int ECBGetBlockLen(EcbContext *ctx, unsigned int *blockLen)
{
    unsigned int len = ctx->len;

    *blockLen = len;
    return len ? 0 : 11;
}

static int ECBContextDestructor(EcbContext *ctx);

static int ECBSetContextDestructor(BsafePoolEntry *entry)
{
    entry->destructor = (BsafeDestructorFn)ECBContextDestructor;
    return 0;
}

static int ECBContextDestructor(EcbContext *ctx)
{
    if (ctx == 0)
        return 0;
    if (ctx->block != 0) {
        T_memset(ctx->block, 0, ctx->len);
        T_free(ctx->block);
    }
    return 0;
}

static int ECBRestart(void)
{
    return 9;
}

static A_FeedbackAlga A_ECB_ENCRYPT_ALGA = {
    ECBQuery, ECBInit, ECBRestart, ECBEncryptUpdate, ECBFinal, ECBGetBlockLen,
    ECBSetContextDestructor
};

B_AlgorithmMethod AM_ECB_ENCRYPT = { &AIT_ECB, 1, 0, &A_ECB_ENCRYPT_ALGA };

static A_FeedbackAlga A_ECB_DECRYPT_ALGA = {
    ECBQuery, ECBInit, ECBRestart, ECBDecryptUpdate, ECBFinal, ECBGetBlockLen,
    ECBSetContextDestructor
};

B_AlgorithmMethod AM_ECB_DECRYPT = { &AIT_ECB, 0, 0, &A_ECB_DECRYPT_ALGA };
