/*
 * The growing input stream of the incremental ASN.1 decoding.
 */
#include "bsafe.h"


#include "asn.h"

/* Grows ctx->buffer by growAmount + 50 bytes, then, unless the stream is at its end (ctx->full),
 * reads from the source into the new space; a short read sets ctx->full. *outBuf and *outLen get the
 * buffer and its length. */
int _A_FlushStreamFlushExtended(AsnStream *ctx, unsigned char **outBuf, unsigned int *outLen,
                                 unsigned int growAmount)
{
    unsigned char *newBuf;
    unsigned int newLen;
    unsigned int readCount;
    int status;

    if (growAmount == 0)
        return 0;

    ctx->size = growAmount + 50;
    newBuf = T_malloc(ctx->size);
    if (newBuf == 0)
        return BER_ERR_ALLOC;

    T_memcpy(newBuf, ctx->buffer, ctx->used);
    T_free(ctx->buffer);
    ctx->buffer = newBuf;

    if (!ctx->full) {
        status = ctx->source->vtbl->read(ctx->source, ctx->buffer + ctx->used, &readCount,
                                        ctx->size - ctx->used);
        if (status != 0)
            return status;

        newLen = ctx->used + readCount;
        ctx->used = newLen;
        if (newLen < ctx->size)
            ctx->full = 1;
    }

    *outBuf = ctx->buffer;
    ctx->pos = 0;
    *outLen = ctx->used;
    return 0;
}
