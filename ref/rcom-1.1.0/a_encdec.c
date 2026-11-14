/*
 * The ASN.1 template engine of BSAFE: _A_EncodeElement and _A_DecodeElement walk the template
 * entries (asn.h) and encode or decode the values of the caller's value array, plus the identifier
 * and length coding (_A_EncodeType, _A_DecodeType) and the ASN_Decode and ASN_EncodeAny entries.
 */
#include "bsafe.h"
#include "asn.h"
#include "../../src/rcom/umempool.h"


/* Closes an indefinite-length constructed value: writes the two end-of-contents octets. */
static unsigned char _A_EndOfContents[2] = { 0, 0 };

/* Decodes the element of entry e from buf/len, refilling from st when the buffer runs out, into
 * values[e->valueIndex], recursing into constructed entries. *elemLen gets the length of the
 * encoding, *out the name of the field that failed. */

static int _A_DecodeRecover(unsigned int *elemLen, AsnTemplateEntry *e,
                            AsnElement **items, unsigned char *buf,
                            unsigned int len, AsnStream *st, unsigned int tag,
                            ITEM *ber, char **name);
static int _A_DecodePrepare(AsnTemplateEntry *e);
static int _A_EmitHeader(AsnEncodeCtx *ctx, int tag, AsnTemplateEntry *e);
static int _A_CloseConstructed(AsnEncodeCtx *ctx);
static int _A_FinishEncode(AsnEncodeCtx *ctx);


/* Decodes data/len with template e into values; *outVal, when not 0, gets the length of the
 * encoding. */
int ASN_Decode(AsnTemplateEntry *e, void *info, unsigned char *data, unsigned int len, void *outVal,
               void *values)
{
    unsigned int elemLen;
    char *name;
    int status;

    name = 0;
    status = _A_DecodeElement(e, info, data, len, 0, &elemLen, values, &name);
    if (name != 0)
        T_free(name);
    if (status != 0)
        return status;
    if (outVal != 0)
        *(unsigned int *)outVal = elemLen;
    return 0;
}

/* As ASN_Decode, but the name of the field that failed is returned in *out. */
int ASN_DecodeX(AsnTemplateEntry *e, void *info, unsigned char *data, unsigned int len, void *outVal,
                void *values, char **out)
{
    unsigned int elemLen;
    int status;

    status = _A_DecodeElement(e, info, data, len, 0, &elemLen, values, out);
    if (status != 0)
        return status;
    if (outVal != 0)
        *(unsigned int *)outVal = elemLen;
    return 0;
}

/* Decodes one element and checks that its identifier is `type`; out gets the contents. */
int ASN_DecodeAny(unsigned int type, unsigned char *data, unsigned int len, ITEM *out)
{
    unsigned int recognized;
    unsigned int decodedType;
    unsigned int extraCount;
    int status;

    status = _A_DecodeType(&recognized, &decodedType, &extraCount, out, data, len);
    if (status != 0)
        return status;
    if (decodedType != type)
        goto bad;
    if (recognized == 0) {
bad:
        return BER_ERR_INPUT_DATA;
    }
    return 0;
}

int _A_DecodeElement(AsnTemplateEntry *e, void *info, unsigned char *buf,
                     unsigned int len, AsnStream *st, unsigned int *elemLen,
                     AsnElement **items, char **out)
{
    ITEM ber;
    BER_ELEMENT args;
    unsigned int tag;
    unsigned int hdrLen;
    char *fieldName;
    unsigned int sublen;
    int count;
    int indefinite;
    unsigned int hdrSize;
    unsigned int idx;
    unsigned int flags;
    unsigned int off;
    unsigned int total;
    unsigned int j;
    unsigned int k;
    int prepared;
    int status;

    fieldName = 0;
    hdrSize = 0;
    idx = (unsigned int)-1;
    *out = 0;
    flags = e->flags & ~0x200;
    prepared = _A_DecodePrepare(e);

    if (st != 0 && (buf == 0 || (e->flags & 0x8800) != 0)) {
        status = st->vtbl->refill(st, &buf, &len);
        if (status != 0)
            return _A_ConstructFieldName(status, e, fieldName,
                                         (unsigned int)-1, out);
        if (_A_DecodeType(elemLen, &tag, &hdrLen, &ber, buf, len) != 0
            && (e->flags & 0x8000) != 0) {
            status = _A_FlushStreamFlushExtended(st, &buf, &len, *elemLen);
            if (status != 0)
                return _A_ConstructFieldName(status, e,
                                             fieldName, (unsigned int)-1, out);
        }
        goto skipHeader;
    }

skipHeader:
    if (items[e->valueIndex] == (AsnElement *)&ASN_NoEncoding) {
        *elemLen = 0;
        return 0;
    }

    status = _A_DecodeType(elemLen, &tag, &hdrLen, &ber, buf, len);
    if (status != 0) {
        if (status == BER_ERR_INPUT_LEN && (flags & 0x800) != 0) {
            status = _A_DecodeRecover(elemLen, e, items, buf, len, st, tag,
                                      &ber, &fieldName);
            return _A_ConstructFieldName(status, e, fieldName,
                                         (unsigned int)-1, out);
        }
        if (prepared == 0)
            return _A_ConstructFieldName(status, e, fieldName,
                                         (unsigned int)-1, out);
    }
    if (*elemLen == 0)
        hdrSize = hdrLen + 4;
    off = (unsigned int)(ber.data - buf);

    if ((flags & 0x1ff) == 258)
        goto anyTag;

    if (tag != (flags & 0xff)) {
        if ((flags & 0x4420) == 0x4420 && (e[1].flags & 0x20) == 0
            && tag == (flags & 0xdf)) {
            status = _A_DecodeElement(e + 1, info, buf, *elemLen, st, &sublen, items,
                                      &fieldName);
            if (status != 0)
                return _A_ConstructFieldName(status, e, fieldName,
                                             (unsigned int)-1, out);
            goto constructed;
        }
        if (e->flags & 0x200) {
            *elemLen = 0;
            return 0;
        }
        return _A_ConstructFieldName(2055, e, fieldName, (unsigned int)-1, out);
    }

constructed:
    if ((tag & 0x20) == 0)
        goto emit;

    _A_GetElementEntryCount(&count, e);
    k = 1;
    while (k < (unsigned int)count
           && (e[k].flags == 259 || items[e[k].valueIndex] == 0))
        k++;
    if (k < (unsigned int)count)
        goto haveEntry;
    if (*elemLen == 0) {
        status = _A_GetIndefiniteElementLen(elemLen, buf, len);
        if (status != 0)
            return _A_ConstructFieldName(status, e, fieldName,
                                         (unsigned int)-1, out);
        ber.len = *elemLen - hdrSize;
    }
    goto emit;

haveEntry:
    if (*elemLen == 0)
        ber.len = len - hdrSize;
    indefinite = 0;
    if ((flags & 0xc0) != 0) {
        unsigned int t = flags & 0x1000;
        indefinite = (t == 0);
    }
    k = 0;
    if (st != 0) {
        ber.data = st->buffer;
        st->pos = st->pos + off;
        k = st->pos;
    }
    total = 0;
    j = 1;
    for (;;) {
        unsigned int room;
        if (indefinite == 0) {
            if ((flags & 0x400) != 0)
                idx = idx + 1;
            else if (e[j].flags == 259)
                break;
            if (*elemLen == 0) {
                if (ber.data[k] == 0 && ber.data[k + 1] == 0)
                    break;
            } else if (total >= ber.len)
                break;
        }
        if (st != 0)
            room = st->used - st->pos;
        else
            room = ber.len - k;
        status = _A_DecodeElement(&e[j], info, ber.data + k, room, st, &sublen,
                                  items, &fieldName);
        if (status != 0)
            return _A_ConstructFieldName(status, e, fieldName, idx, out);
        if (st != 0) {
            ber.data = st->buffer;
            k = st->pos;
        } else {
            k = k + sublen;
        }
        total = total + sublen;
        if (indefinite != 0)
            break;
        if ((flags & 0x400) == 0) {
            _A_GetElementEntryCount(&count, &e[j]);
            j = j + count;
        }
    }
    if (*elemLen == 0) {
        if (st != 0) {
            hdrSize = 2;
            st->pos = st->pos + 2;
        }
        ber.len = k;
        *elemLen = k + hdrSize;
    } else if (total != ber.len) {
        return _A_ConstructFieldName(2054, e, fieldName, idx, out);
    }
    if (indefinite == 0 && (flags & 0x400) == 0 && e[j].flags != 259) {
        for (;;) {
            if ((e[j].flags & 0x200) == 0
                && items[e[j].valueIndex] != (AsnElement *)&ASN_NoEncoding) {
                if (e[j].name != 0) {
                    fieldName = T_malloc(T_strlen(e[j].name) + 1);
                    if (fieldName != 0)
                        T_strcpy(fieldName, e[j].name);
                }
                return _A_ConstructFieldName(2053, e, fieldName, idx, out);
            }
            _A_GetElementEntryCount(&count, &e[j]);
            j = j + count;
            if (e[j].flags == 259)
                break;
        }
    }

    goto emit;

anyTag:
    if (*elemLen == 0) {
        status = _A_GetIndefiniteElementLen(elemLen, buf, len);
        if (status != 0)
            return _A_ConstructFieldName(status, e, fieldName,
                                         (unsigned int)-1, out);
        ber.len = *elemLen - hdrSize;
    }

emit:
    {
        if (items[e->valueIndex] != 0) {
            if (e->decodeFn != 0) {
                args.tag = tag;
                args.next = buf + 1;
                args.info = info;
                args.buf = buf;
                args.elemLen = *elemLen;
                args.data = ber.data;
                args.len = ber.len;
                args.decodeFieldName = 0;
                status = e->decodeFn(&args, items[e->valueIndex]);
                fieldName = args.decodeFieldName;
                _A_StripFirstField(&fieldName);
                if (status != 0)
                    return _A_ConstructFieldName(status, e, fieldName, idx,
                                                 out);
                if (st != 0
                    && ((e->flags & 0x20) == 0 || e[1].flags == 259))
                    st->pos = st->pos + *elemLen;
            } else {
                items[e->valueIndex]->data = buf;
                items[e->valueIndex]->size = *elemLen;
                if (st != 0)
                    st->pos = st->pos + *elemLen;
            }
        }
    }
    if (e->flags & 0x8000) {
        buf = buf + *elemLen;
        status = st->vtbl->refill(st, &buf, &len);
        if (status != 0)
            return status;
    }
    return 0;
}


/* Decodes an identifier and a length. *recognized gets the length of header and contents (0 for
 * an indefinite-length constructed value), *decodedType the identifier, *extraCount the extra tag
 * octets of a high tag number, out the contents. Returns BER_ERR_INPUT_LEN when the data is too
 * short and BER_ERR_INPUT_DATA for an invalid length form. */
int _A_DecodeType(unsigned int *recognized, unsigned int *decodedType, unsigned int *extraCount,
                   ITEM *out, unsigned char *data, unsigned int maxlen)
{
    if (maxlen == 0)
        return BER_ERR_INPUT_LEN;

    *decodedType = data[0];
    *extraCount = 0;

    if ((*decodedType & 0x1f) == 0x1f) {
        do {
            if (maxlen < *extraCount + 2)
                return BER_ERR_INPUT_LEN;
        } while (data[(*extraCount)++ + 1] & 0x80);
    }

    if (maxlen < *extraCount + 2)
        return BER_ERR_INPUT_LEN;

    data += *extraCount + 1; /* data now points at the length octet */

    if ((signed char)data[0] >= 0) {
        unsigned int len = data[0];
        unsigned int hdrLen;
        out->len = len;
        hdrLen = *extraCount + 2;
        *recognized = len + hdrLen;
        out->data = data + 1;
    } else if (data[0] == 0x80) {
        if ((*decodedType & 0x20) == 0)
            return BER_ERR_INPUT_DATA;
        out->data = data + 1;
        out->len = 0;
        *recognized = 0;
    } else if (data[0] == 0x81) {
        unsigned int len;
        unsigned int hdrLen;
        if (maxlen < *extraCount + 3)
            return BER_ERR_INPUT_LEN;
        len = data[1];
        out->len = len;
        hdrLen = *extraCount + 3;
        *recognized = len + hdrLen;
        out->data = data + 2;
    } else if (data[0] == 0x82) {
        unsigned int len;
        unsigned int hdrLen;
        if (maxlen < *extraCount + 4)
            return BER_ERR_INPUT_LEN;
        len = (data[1] << 8) | data[2];
        out->len = len;
        hdrLen = *extraCount + 4;
        *recognized = len + hdrLen;
        out->data = data + 3;
    } else if (data[0] == 0x83) {
        unsigned int len;
        unsigned int hdrLen;
        if (maxlen < *extraCount + 5)
            return BER_ERR_INPUT_LEN;
        len = (data[1] << 16) | (data[2] << 8) | data[3];
        out->len = len;
        hdrLen = *extraCount + 5;
        *recognized = len + hdrLen;
        out->data = data + 4;
    } else if (data[0] == 0x84) {
        unsigned int len;
        unsigned int hdrLen;
        if (maxlen < *extraCount + 6)
            return BER_ERR_INPUT_LEN;
        len = (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];
        out->len = len;
        hdrLen = *extraCount + 6;
        *recognized = len + hdrLen;
        out->data = data + 5;
    } else {
        return BER_ERR_INPUT_DATA;
    }

    if (maxlen < *recognized)
        return BER_ERR_INPUT_LEN;
    return 0;
}

/* Decodes an element split into segments (the 0x800 continuation flag): calls the decoder of the
 * type once per segment, refilling the stream in between, until the segments add up to the length
 * read at the start. */
static int _A_DecodeRecover(unsigned int *elemLen, AsnTemplateEntry *e,
                            AsnElement **items, unsigned char *buf,
                            unsigned int len, AsnStream *st, unsigned int tag,
                            ITEM *ber, char **name)
{
    BER_ELEMENT args;
    unsigned int want;
    unsigned int total;
    unsigned int rest;
    int status;

    total = 0;
    want = *elemLen;
    *name = 0;
    if (st == 0)
        return 2056;
    len = st->used - st->pos;
    if ((e->flags & 0x820) != 0x800)
        return BER_ERR_DATA;
    if (items[e->valueIndex] == 0)
        return BER_ERR_DATA;
    if (e->decodeFn == 0)
        return BER_ERR_DATA;
    if (len < *elemLen)
        *elemLen = len;
    args.tag = tag | 0x800;
    args.buf = buf;
    args.next = buf + 1;
    args.data = ber->data;
    args.elemLen = *elemLen;
    args.len = *elemLen - (ber->data - buf);
    args.decodeFieldName = 0;
    for (;;) {
        status = e->decodeFn(&args, items[e->valueIndex]);
        if (status != 0) {
            if (args.decodeFieldName != 0) {
                *name = args.decodeFieldName;
                _A_StripFirstField(name);
            }
            return status;
        }
        st->pos = st->pos + *elemLen;
        total = total + *elemLen;
        buf = buf + *elemLen;
        status = st->vtbl->refill(st, &buf, &len);
        if (status != 0)
            return status;
        if ((args.tag & 0x800) != 0) {
            rest = want - total;
            if (len < rest) {
                *elemLen = len;
            } else {
                *elemLen = rest;
                args.tag = args.tag & ~0x800;
            }
            args.buf = buf;
            args.data = buf;
            args.elemLen = *elemLen;
            args.len = *elemLen;
        } else
            break;
    }
    if (total != want)
        return BER_ERR_DATA;
    *elemLen = want;
    return 0;
}

/* Walks the template from e to the end of the constructed value that starts here; returns 1 if
 * an entry in it has 0x8800 set, 0 otherwise. */
static int _A_DecodePrepare(AsnTemplateEntry *e)
{
    unsigned int entryFlags;
    unsigned int flags;
    int depth;

    depth = 0;
    for (;;) {
        entryFlags = e->flags;
        flags = entryFlags & ~0x200;
        if (entryFlags & 0x8800)
            return 1;
        e++;
        if (entryFlags & 0x20) {
            if (flags - 48 >= 2 && (entryFlags & 0x1400) != 0x1000)
                continue;
            depth++;
        } else if (flags == 0x103) {
            depth--;
        }
        if (depth <= 0)
            return 0;
    }
}

/* The second argument is the output stream. Returns the error of U_MemPoolCreate. */
int ASN_EncodeCtxConstructor(AsnEncodeCtx *ctx, AsnOutStream *stream)
{
    int status;

    T_memset(&ctx->count, 0, 24);
    status = U_MemPoolCreate(&ctx->pool);
    if (status != 0)
        return status;
    ctx->stream = stream;
    return 0;
}

/* Frees up to capacity, not count: ASN_AddElementPointer keeps every slot below capacity
 * allocated. */
void ASN_EncodeCtxDestructor(AsnEncodeCtx *ctx)
{
    unsigned int i;

    if (ctx->buffer != 0) {
        T_memset(ctx->buffer, 0, ctx->elemSize);
        T_free(ctx->buffer);
    }
    if (ctx->capacity != 0) {
        i = 0;
        do {
            T_free(ctx->elements[i]);
            i++;
        } while (i < ctx->capacity);
    }
    T_free(ctx->elements);
    if (ctx->pool != 0)
        U_MemPoolDestroy(&ctx->pool);
}

/* Two passes over _A_EncodeType: the first computes the size, the second writes into the
 * allocated buffer. *out is set even when the allocation fails. */
int ASN_EncodeAnyAlloc(unsigned int type, void *data, unsigned int len, unsigned char **out,
                       unsigned int *outLen)
{
    unsigned char *buf;
    int status;

    status = _A_EncodeType(0, outLen, 0, type, 0, data, len);
    if (status != 0)
        return status;
    buf = T_malloc(*outLen);
    *out = buf;
    if (buf == 0)
        return BER_ERR_ALLOC;
    status = _A_EncodeType(buf, outLen, *outLen, type, 0, data, len);
    return status;
}

/* As ASN_EncodeAnyAlloc, with the tag number octets of a high tag number. */
int ASN_EncodeAnyExtendedAlloc(unsigned int type, unsigned char *contBytes, void *data, unsigned int len,
                               unsigned char **out, unsigned int *outLen)
{
    unsigned char *buf;
    int status;

    status = _A_EncodeType(0, outLen, 0, type, contBytes, data, len);
    if (status != 0)
        return status;
    buf = T_malloc(*outLen);
    *out = buf;
    if (buf == 0)
        return BER_ERR_ALLOC;
    status = _A_EncodeType(buf, outLen, *outLen, type, contBytes, data, len);
    return status;
}

int _A_EncodeElement(AsnEncodeCtx *ctx, AsnTemplateEntry *e, AsnElement **items,
                     char **out)
{
    unsigned int savedCount;
    unsigned int flags;
    unsigned int inner;
    int status;
    unsigned int i;
    unsigned int j;
    int count;
    char *fieldName;
    AsnElement *item;
    AsnElement *last;
    void *marker;

    i = (unsigned int)-1;
    fieldName = 0;
    *out = 0;
    savedCount = ctx->count;

    for (;;) {
    flags = e->flags & ~0x200;
    if (e->encodeFn == 0)
        goto plain;
    status = e->encodeFn(ctx, flags & 0x1ff, 0, items[e->valueIndex]);
    if (status != 0) {
        fieldName = ctx->encodeFieldName;
        _A_StripFirstField(&fieldName);
        return _A_ConstructFieldName(status, e, fieldName, (unsigned int)-1, out);
    }
    if (savedCount < ctx->count) {
        if (ctx->elements[ctx->count - 1]->tag != 0x103)
            goto done;
        ctx->count = ctx->count - 1;
        status = _A_EmitHeader(ctx, flags & 0x1ff, e);
        if (status != 0)
            return _A_ConstructFieldName(status, e, fieldName, (unsigned int)-1, out);
        status = ASN_AddElementPointer(ctx, 0x103, 0, 0, 0);
        if (status != 0)
            return _A_ConstructFieldName(status, e, fieldName, (unsigned int)-1, out);
        break;
    }
    goto dispatch;
plain:
    item = items[e->valueIndex];
    if (item != 0) {
        status = ASN_AddElementPointer(ctx, 0x100, 0, item->data, item->size);
        if (status != 0)
            return _A_ConstructFieldName(status, e, fieldName, (unsigned int)-1, out);
        goto done;
    }

dispatch:
    if ((flags & 0x20) == 0) {
        if (e->flags & 0x200)
            return 0;
        return _A_ConstructFieldName(2057, e, fieldName, (unsigned int)-1, out);
    }
    if ((flags & 0xc0) != 0 && (flags & 0x1000) == 0) {
        status = _A_EmitHeader(ctx, flags & 0x1ff, e);
        if (status != 0)
            return _A_ConstructFieldName(status, e, fieldName, (unsigned int)-1, out);
        status = _A_EncodeElement(ctx, e + 1, items, &fieldName);
        if (status != 0)
            return _A_ConstructFieldName(status, e, fieldName, (unsigned int)-1, out);
        status = ASN_AddElementPointer(ctx, 0x103, 0, 0, 0);
        if (status != 0)
            return _A_ConstructFieldName(status, e, fieldName, (unsigned int)-1, out);
        goto done;
    }
    if (e[1].flags == 0x103) {
        if (e->flags & 0x200)
            return 0;
        return _A_ConstructFieldName(2057, e, fieldName, (unsigned int)-1, out);
    }
    if (flags & 0x400) {
        if (e->encodeFn == 0)
            return _A_ConstructFieldName(BER_ERR_DATA, e, fieldName, (unsigned int)-1, out);
        status = _A_EmitHeader(ctx, flags & 0x1ff, e);
        if (status != 0)
            return _A_ConstructFieldName(status, e, fieldName, (unsigned int)-1, out);
        for (;;) {
            i = i + 1;
            status = _A_EncodeElement(ctx, e + 1, items, &fieldName);
            if (status != 0)
                return _A_ConstructFieldName(status, e, fieldName, i, out);
            inner = ctx->count;
            status = e->encodeFn(ctx, flags & 0x1ff, 0, items[e->valueIndex]);
            if (status != 0) {
                fieldName = ctx->encodeFieldName;
                _A_StripFirstField(&fieldName);
                return _A_ConstructFieldName(status, e, fieldName, i, out);
            }
            if (inner < ctx->count
                && ctx->elements[ctx->count - 1]->tag == 0x103)
                break;
        }
        last = ctx->elements[ctx->count - 1];
        last->data = (flags == 0x431 || (flags & 0x3020) == 0x3020)
            ? &_A_EndConstructedSet : 0;
        goto done;
    }
    if (flags - 48 >= 2 && (flags & 0x1000) == 0)
        goto done;
    status = _A_EmitHeader(ctx, flags & 0x1ff, e);
    if (status != 0)
        return _A_ConstructFieldName(status, e, fieldName, (unsigned int)-1, out);
    j = 1;
    while (e[j].flags != 0x103) {
        status = _A_EncodeElement(ctx, &e[j], items, &fieldName);
        if (status != 0)
            return _A_ConstructFieldName(status, e, fieldName, (unsigned int)-1, out);
        _A_GetElementEntryCount(&count, &e[j]);
        j = j + count;
    }
    if (flags == 49 || (flags & 0x3020) == 0x3020)
        marker = &_A_EndConstructedSet;
    else
        marker = 0;
    status = ASN_AddElementPointer(ctx, 0x103, 0, marker, 0);
    if (status != 0)
        return _A_ConstructFieldName(status, e, fieldName, (unsigned int)-1, out);
    goto done;
    }

done:
    if (savedCount == 0) {
        if (ctx->count != 0) {
            if (ctx->elements[ctx->count - 1]->tag == 0x103) {
                status = _A_CloseConstructed(ctx);
                if (status != 0)
                    return _A_ConstructFieldName(status, e, fieldName, (unsigned int)-1, out);
            }
        }
        status = _A_FinishEncode(ctx);
        if (status != 0)
            return _A_ConstructFieldName(status, e, fieldName, (unsigned int)-1, out);
    }
    return 0;
}

/* Encodes an identifier and a length, and the contents when data is not 0. With buf 0 it only
 * computes *outLen, the size needed. contBytes are the tag number octets of a tag 31. */
int _A_EncodeType(unsigned char *buf, unsigned int *outLen, unsigned int size,
                   unsigned int type, unsigned char *contBytes, void *data, unsigned int len)
{
    unsigned int i;
    int lenBytes;
    unsigned int headerLen;
    int shortForm;

    i = 0;
    if ((type & 0x1f) == 0x1f) {
        while (contBytes[i] & 0x80)
            i++;
        i++;
    }

    shortForm = (len < 128);
    if (shortForm)
        lenBytes = 1;
    else if (len < 256)
        lenBytes = 2;
    else if (len <= 0xffff)
        lenBytes = 3;
    else if (len <= 0xffffff)
        lenBytes = 4;
    else
        lenBytes = 5;

    /* headerLen = identifier octet + length octets; *outLen = extra tag octets + header (+ contents
         * below). */
    headerLen = lenBytes + 1;
    *outLen = i + headerLen;
    if (data != 0)
        *outLen += len;

    if (buf == 0)
        return 0;
    if (size < *outLen)
        return BER_ERR_OUTPUT_LEN;

    *buf++ = (unsigned char)type;

    if ((type & 0x1f) == 0x1f) {
        T_memcpy(buf, contBytes, i);
        buf += i;
    }

    if (shortForm) {
        *buf++ = (unsigned char)len;
    } else if (len < 256) {
        *buf++ = 0x81;
        *buf++ = (unsigned char)len;
    } else if (len <= 0xffff) {
        *buf++ = 0x82;
        *buf++ = (unsigned char)(len >> 8);
        *buf++ = (unsigned char)len;
    } else if (len <= 0xffffff) {
        *buf++ = 0x83;
        *buf++ = (unsigned char)(len >> 16);
        *buf++ = (unsigned char)(len >> 8);
        *buf++ = (unsigned char)len;
    } else {
        *buf++ = 0x84;
        *buf++ = (unsigned char)(len >> 24);
        *buf++ = (unsigned char)(len >> 16);
        *buf++ = (unsigned char)(len >> 8);
        *buf++ = (unsigned char)len;
    }

    if (data != 0)
        T_memcpy(buf, data, len);
    return 0;
}

/* Emits the header of the constructed value that starts here. If an entry of the group has 0x4000
 * set the length is not known in advance: the header goes to the stream at once with the
 * indefinite length (identifier + 0x80). Otherwise a marker element _A_BeginConstructed is queued
 * and the serializer fills in the length. */
static int _A_EmitHeader(AsnEncodeCtx *ctx, int tag, AsnTemplateEntry *e)
{
    unsigned char hdr[2];
    int count;
    unsigned int i;
    AsnOutStream *stream;

    _A_GetElementEntryCount(&count, e);
    for (i = 0; i < count; i++) {
        if (e[i].flags & 0x4000)
            break;
    }
    if (i < count) {
        hdr[0] = tag;
        hdr[1] = 0x80;
        stream = ctx->stream;
        return stream->vtbl->write(stream, hdr, 2);
    }
    return ASN_AddElementPointer(ctx, tag, 0, &_A_BeginConstructed, 0);
}

static int _A_CloseConstructed(AsnEncodeCtx *ctx)
{
    AsnOutStream *stream;

    if (ctx->count == 1) {
        stream = ctx->stream;
        ctx->count = 0;
        return stream->vtbl->write(stream, _A_EndOfContents, 2);
    }
    return 0;
}

/* Ends an encoding: measures the queued elements, grows the work buffer if needed, serializes
 * them to the stream, then recycles the pool and resets the count. */
static int _A_FinishEncode(AsnEncodeCtx *ctx)
{
    unsigned int len;
    unsigned int idx;
    int status;
    AsnOutStream *stream;

    if (ctx->count == 0)
        return 0;
    status = _A_GetElementLen(&len, &idx, ctx->elements);
    if (status != 0)
        return status;
    if (len > ctx->elemSize) {
        if (ctx->buffer != 0)
            T_memset(ctx->buffer, 0, ctx->elemSize);
        ctx->buffer = T_realloc(ctx->buffer, len);
        if (ctx->buffer == 0)
            return BER_ERR_ALLOC;
        ctx->elemSize = len;
    }
    _A_ConstructElement(ctx->buffer, &len, len, &idx, ctx->elements,
                        ctx->count - 1);
    U_MemPoolReset(ctx->pool, 1);
    ctx->count = 0;
    stream = ctx->stream;
    return stream->vtbl->write(stream, ctx->buffer, len);
}
