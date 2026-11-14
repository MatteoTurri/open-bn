/*
 * The resize context of BSAFE: a buffer with a pending destructor (ResizeContextConstructor,
 * ResizeContextDestructor, ResizeContextMakeNewContext).
 */
#include "bsafe.h"


void ResizeContextConstructor(BsafePoolEntry *ctx)
{
    ctx->destructor = 0;
    T_memset(ctx, 0, sizeof(BsafePoolEntry));
}

void ResizeContextDestructor(BsafePoolEntry *ctx)
{
    if (ctx->destructor != 0) {
        ctx->destructor(ctx->dataPtr);
    }
    T_memset(ctx->dataPtr, 0, ctx->size);
    ctx->size = 0;
    T_free(ctx->dataPtr);
}

/* Resizes the buffer: runs the pending destructor, returns at once when the size does not change,
 * then clears the old contents and reallocates. BE_ALLOC when the allocation fails (the old buffer stays,
 * its size becomes 0). */
int ResizeContextMakeNewContext(BsafePoolEntry *ctx, unsigned int newSize)
{
    if (ctx->destructor != 0)
        ctx->destructor(ctx->dataPtr);
    if (ctx->size == newSize)
        return 0;
    T_memset(ctx->dataPtr, 0, ctx->size);
    ctx->dataPtr = T_realloc(ctx->dataPtr, newSize);
    if (ctx->dataPtr == 0) {
        ctx->size = 0;
        return BE_ALLOC;
    }
    ctx->size = newSize;
    return 0;
}
