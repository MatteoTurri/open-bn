/*
 * ASN_EncodeAlloc and ASN_EncodeAllocX: encode a value array into a newly allocated ITEM, through
 * the allocating output stream.
 */
#include "bsafe.h"
#include "asn.h"
#include "berder.h"


struct MemPool;


/* The method table of the allocating stream. */
static AsnOutStreamVtbl asn_alloc_stream_vtable = { ASN_AllocStreamWrite };


/* As ASN_EncodeToStream/X, but the stream is built here: an AsnAllocStream that writes into out. */
int ASN_EncodeAlloc(AsnTemplateEntry *e, unsigned int flags, void *values, ITEM *out)
{
    AsnAllocStream stream;
    AsnEncodeCtx ctx;
    char *extra;
    int status;

    T_memset(&stream, 0, 8);
    T_memset(&ctx, 0, 32);
    ASN_AllocStreamConstructor(&stream, out);
    status = _A_BSafeError(ASN_EncodeCtxConstructor(&ctx, &stream.base));
    if (status == 0) {
        extra = 0;
        status = _A_BSafeError(_A_EncodeElement(&ctx, e, values, &extra));
        if (extra != 0)
            T_free(extra);
        ASN_EncodeCtxDestructor(&ctx);
    }
    return status;
}


int ASN_EncodeAllocX(AsnTemplateEntry *e, unsigned int flags, void *values, ITEM *out,
                     char **extraOut)
{
    AsnEncodeCtx ctx;
    AsnAllocStream stream;
    int status;

    T_memset(&stream, 0, 8);
    T_memset(&ctx, 0, 32);
    ASN_AllocStreamConstructor(&stream, out);
    status = _A_BSafeError(ASN_EncodeCtxConstructor(&ctx, &stream.base));
    if (status == 0) {
        status = _A_BSafeError(_A_EncodeElement(&ctx, e, values, extraOut));
        ASN_EncodeCtxDestructor(&ctx);
    }
    return status;
}

void ASN_AllocStreamConstructor(AsnAllocStream *ctx, ITEM *out)
{
    ctx->out = out;
    out->data = 0;
    out->len = 0;
    ctx->base.vtbl = &asn_alloc_stream_vtable;
}

int ASN_AllocStreamWrite(AsnAllocStream *ctx, void *data, unsigned int len)
{
    void *p;
    void *dst;
    ITEM *out;
    ITEM *result;
    if (ctx->out->data != 0)
        return BER_ERR_DATA;
    p = T_malloc(len);
    out = ctx->out;
    out->data = p;
    if (p == 0)
        return BER_ERR_ALLOC;
    result = ctx->out;
    dst = result->data;
    result->len = len;
    T_memcpy(dst, data, len);
    return 0;
}
