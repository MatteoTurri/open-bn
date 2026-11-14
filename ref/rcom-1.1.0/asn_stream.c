/*
 * The ASN.1 streams of BSAFE: encoding into an output stream (ASN_EncodeToStream/X), and decoding
 * from an input source through the flush stream.
 */
#include "bsafe.h"
#include "asn.h"


/* The method table of the flush stream. */
static AsnStreamVtbl asn_flush_stream_vtable = { ASN_FlushStreamFlush };

void ASN_FlushStreamConstructor(AsnStream *ctx, AsnStreamSource *source)
{
    ctx->source = source;
    T_memset(&ctx->buffer, 0, 8);
    ctx->vtbl = &asn_flush_stream_vtable;
}

void ASN_FlushStreamDestructor(AsnStream *ctx)
{
    if (ctx->buffer != 0) {
        T_memset(ctx->buffer, 0, ctx->size);
        T_free(ctx->buffer);
    }
}

/* flags is not used. */
int ASN_EncodeToStream(AsnTemplateEntry *e, unsigned int flags, void *values, AsnOutStream *stream)
{
    AsnEncodeCtx ctx;
    char *extra;
    int status;

    extra = 0;
    do {
        if ((status = ASN_EncodeCtxConstructor(&ctx, stream)) != 0)
            break;
        status = _A_EncodeElement(&ctx, e, values, &extra);
        if (extra != 0)
            T_free(extra);
        ASN_EncodeCtxDestructor(&ctx);
    } while (0);
    return status;
}

int ASN_EncodeToStreamX(AsnTemplateEntry *e, unsigned int flags, void *values, AsnOutStream *stream, char **extraOut)
{
    AsnEncodeCtx ctx;
    int status;

    do {
        if ((status = ASN_EncodeCtxConstructor(&ctx, stream)) != 0)
            break;
        status = _A_EncodeElement(&ctx, e, values, extraOut);
        ASN_EncodeCtxDestructor(&ctx);
    } while (0);
    return status;
}

/* As ASN_Decode, but the input comes from a source through a flush stream. The X variant returns
 * the name of the field that failed in *name; the other one frees it. */
int ASN_DecodeFromStream(AsnTemplateEntry *e, void *info, AsnStreamSource *source, void *values)
{
    AsnStream ctx;
    unsigned int elemLen;
    char *name;
    int status;

    name = 0;
    ASN_FlushStreamConstructor(&ctx, source);
    status = _A_DecodeElement(e, info, 0, 0, &ctx, &elemLen, values, &name);
    if (name != 0)
        T_free(name);
    ASN_FlushStreamDestructor(&ctx);
    return status;
}

int ASN_DecodeFromStreamX(AsnTemplateEntry *e, void *info, AsnStreamSource *source, void *values, char **name)
{
    AsnStream ctx;
    unsigned int elemLen;
    int status;

    ASN_FlushStreamConstructor(&ctx, source);
    status = _A_DecodeElement(e, info, 0, 0, &ctx, &elemLen, values, name);
    ASN_FlushStreamDestructor(&ctx);
    return status;
}

/* Refills the stream: the first call allocates the 32000-byte buffer (BER_ERR_ALLOC on failure). Moves the
 * unread bytes to the start of the buffer and, unless the source is exhausted (full), asks it for
 * more, returning its error as is and setting full when it gave less than asked. Then *pcur is the
 * start of the buffer and *pcount the bytes in it. */
int ASN_FlushStreamFlush(ctx, pcur, pcount)
    AsnStream *ctx;
    unsigned char **pcur;
    unsigned int *pcount;
{
    unsigned int written;
    int r;

    if (ctx->buffer == 0) {
        ctx->buffer = T_malloc(32000);
        if (ctx->buffer == 0)
            return BER_ERR_ALLOC;
        *pcur = ctx->buffer + 32000;
        ctx->used = 32000;
        ctx->size = 32000;
        ctx->pos = 0;
    }

    if (*pcur != ctx->buffer) {
        ctx->used = ctx->used - (*pcur - ctx->buffer);
        T_memmove(ctx->buffer, *pcur, ctx->used);

        ctx->pos = 0;
        if (ctx->full == 0) {
            r = ctx->source->vtbl->read(
                    ctx->source,
                    ctx->buffer + ctx->used,
                    &written, ctx->size - ctx->used);
            if (r != 0)
                return r;
            ctx->used = ctx->used + written;
            if (ctx->used < ctx->size)
                ctx->full = 1;
        }
    }

    *pcur = ctx->buffer;
    *pcount = ctx->used;
    return 0;
}
