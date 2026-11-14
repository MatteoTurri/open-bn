/*
 * The encoding context of the ASN.1 engine of BSAFE: the element list the encoders fill
 * (ASN_AddElement), its measurement and serialization, the field names of the error messages and the
 * helpers that walk a template.
 */
#include "bsafe.h"
#include "asn.h"
#include "berder.h"
#include "../../src/rcom/umempool.h"


struct MemPool;


/* Elements: tag bit 0x800 means that the element continues in the next one, 0x103 closes a
 * constructed value, (tag & 0x1FF) == 0x100 is an element without header. */

/* The constant encodings of algorithm parameters: the DER NULL (05 00), passed as parameters by
 * KIT_RSAPublicBERMakeInfo, and the empty ITEM that _A_DecodeElement compares by address (do not
 * encode). */

static unsigned char nullEncoding[] = { 0x05, 0x00 };
ITEM ASN_NullEncoding = { nullEncoding, sizeof (nullEncoding) };
ITEM ASN_NoEncoding = { 0, 0 };

char _A_BeginConstructed = 0;
char _A_EndConstructedSet = 0;


/* Copies the contents into the pool of ctx when size is not 0, otherwise keeps the pointer.
 * BER_ERR_ALLOC when the allocation fails. */
int ASN_AddElement(AsnEncodeCtx *ctx, int tag, unsigned char *tagBytes, void *data, unsigned int size)
{
    void *buf;
    int status;

    if (size != 0) {
        status = U_MemPoolMallocAndCopy(ctx->pool, size, data, &buf);
        if (status != 0)
            return BER_ERR_ALLOC;
    } else {
        buf = data;
    }
    return ASN_AddElementPointer(ctx, tag, tagBytes, buf, size);
}


/* Appends {data, size, tag, tagBytes} to the elements of ctx; a tag 31 needs its tag number octets
 * (BER_ERR_DATA). The array grows by 10 elements, all allocated. */
int ASN_AddElementPointer(AsnEncodeCtx *ctx, int tag, unsigned char *tagBytes, void *data, unsigned int size)
{
    unsigned int newCap;
    unsigned int newBytes;
    AsnElement **newArr;
    AsnElement *p;

    if (tag == 31) {
        if (tagBytes == 0)
            return BER_ERR_DATA;
    }

    if (ctx->count >= ctx->capacity) {
        newCap = ctx->capacity + 10;
        newBytes = newCap << 2;
        if (newBytes < (ctx->capacity << 2))
            return BER_ERR_ALLOC;
        newArr = T_malloc(newBytes);
        if (newArr == 0)
            return BER_ERR_ALLOC;
        T_memset(newArr, 0, newBytes);
        T_memcpy(newArr, ctx->elements, ctx->capacity << 2);
        T_free(ctx->elements);
        ctx->elements = newArr;
        if (ctx->capacity < newCap) {
            do {
                p = T_malloc(sizeof(AsnElement));
                ctx->elements[ctx->capacity] = p;
                if (p == 0)
                    return BER_ERR_ALLOC;
                ctx->capacity = ctx->capacity + 1;
            } while (ctx->capacity < newCap);
        }
    }

    ctx->elements[ctx->count]->data = data;
    ctx->elements[ctx->count]->size = size;
    ctx->elements[ctx->count]->tag = tag;
    ctx->elements[ctx->count]->tagBytes = tagBytes;
    ctx->count = ctx->count + 1;
    return 0;
}

/* Finds the first top-level entry of the template whose tag is `tag`; *out gets the entry or 0. */
void ASN_GetSubTemplate(AsnTemplateEntry *tmpl, int tag, AsnTemplateEntry **out)
{
    int count;
    unsigned int i;

    _A_GetElementEntryCount(&count, tmpl);
    i = 0;
    if (count == 0)
        goto notfound;
    do {
        if (tmpl[i].valueIndex == tag) {
            *out = &tmpl[i];
            return;
        }
        i++;
    } while (i < (unsigned int)count);
notfound:
    *out = 0;
}

/* The PrintableString characters: A-Z a-z 0-9 space ' ( ) + , - . / : = ? */
int ASN_IsPrintableString(char *data, unsigned int len)
{
    unsigned int i;
    unsigned char c;

    if (len == 0)
        goto valid;
    i = 0;
    do {
        c = data[i];
        if ((unsigned char)(c - 65) < 26) {
            i++;
            continue;
        }
        if ((unsigned char)(c - 97) < 26) {
            i++;
            continue;
        }
        if (c == ' ') {
            i++;
            continue;
        }
        if ((unsigned char)(c - 39) < 20 && c != '*') {
            i++;
            continue;
        }
        if (c == '=') {
            i++;
            continue;
        }
        if (c == '?') {
            i++;
            continue;
        }
        return 0;
    } while (i < len);
valid:
    return 1;
}

/* Counts the entries of the template that make up one top-level element, following the nesting of
 * the constructed entries (0x20, 0x1400) and their 0x103 ends. *outCount is updated at every step. */
void _A_GetElementEntryCount(int *outCount, AsnTemplateEntry *entries)
{
    unsigned int flags;
    unsigned int masked;
    int depth;
    int i;

    *outCount = 0;
    depth = 0;
    for (;;) {
        i = *outCount;
        flags = entries[i].flags;
        *outCount = i + 1;
        masked = flags & 0xffff75ff;
        if (flags & 0x20) {
            if ((masked - 48) < 2)
                depth++;
            else if ((flags & 0x1400) == 0x1000)
                depth++;
            else
                continue;
        } else if (masked == 0x103)
            depth--;
        if (depth <= 0)
            break;
    }
}

/* The encoded length of one top-level element from elems[0]: recursive on constructed values,
 * iterative on 0x800 segments, plus the header from _A_EncodeType. *idxOut gets the number of
 * elements used. 0x802 when the sum overflows. */
int _A_GetElementLen(unsigned int *lenOut, unsigned int *idxOut,
                     AsnElement **elems)
{
    unsigned int i;
    unsigned int sublen;
    unsigned int subcount;
    int rc;

    *lenOut = 0;
    if (elems[0]->data == &_A_BeginConstructed) {
        i = 1;
        while (elems[i]->tag != 0x103) {
            rc = _A_GetElementLen(&sublen, &subcount, &elems[i]);
            if (rc != 0)
                return rc;
            *lenOut += sublen;
            if (*lenOut < sublen)
                return 0x802;
            i += subcount;
        }
        *idxOut = i + 1;
    } else {
        i = 0;
        if (elems[0]->tag & 0x800) {
            do {
                *lenOut += elems[i]->size;
                if (*lenOut < elems[i]->size)
                    return 0x802;
                i++;
            } while (elems[i]->tag & 0x800);
        }
        *lenOut += elems[i]->size;
        if (*lenOut < elems[i]->size)
            return 0x802;
        *idxOut = i + 1;
        if ((elems[i]->tag & 0x1ff) == 0x100)
            return 0;
    }
    _A_EncodeType(0, &sublen, 0, elems[0]->tag, elems[0]->tagBytes, 0, *lenOut);
    *lenOut += sublen;
    if (*lenOut < sublen)
        return 0x802;
    return 0;
}

/* Serializes one element and its children backwards from the end of the buffer, so that each
 * header, known only after its contents, lands in front of them. In a SET the new element is moved
 * to its DER position (_A_FindPositionInSet). */
void _A_ConstructElement(char *base, unsigned int *lenOut, unsigned int size,
                         unsigned int *idxOut, AsnElement **elems, unsigned int i)
{
    unsigned int sublen;
    unsigned int subcount;
    unsigned int pos;
    unsigned int j;
    AsnElement **p;

    pos = 0;
    *lenOut = 0;
    if (elems[i]->tag == 0x103) {
        j = i - 1;
        while (elems[j]->data != &_A_BeginConstructed) {
            _A_ConstructElement(base, &sublen, size - *lenOut, &subcount, elems, j);
            if (elems[i]->data == &_A_EndConstructedSet) {
                _A_FindPositionInSet(&pos, base + size - (*lenOut + sublen), sublen,
                                     base + size - *lenOut, *lenOut);
                if (pos != 0) {
                    T_memmove(base + size - (*lenOut + sublen), base + size - *lenOut, pos);
                    _A_ConstructElement(base + size - (*lenOut + sublen) + pos, &sublen,
                                        sublen, &subcount, elems, j);
                }
            }
            *lenOut += sublen;
            j -= subcount;
        }
        *idxOut = (i - j) + 1;
        goto encode;
    }
    j = i;
    T_memcpy(base + size - elems[i]->size, elems[i]->data, elems[i]->size);
    *lenOut += elems[i]->size;
    if (j != 0 && (elems[i-1]->tag & 0x800)) {
        p = &elems[i];
        do {
            p--;
            T_memcpy(base + size - (*lenOut + p[0]->size), p[0]->data, p[0]->size);
            j--;
            *lenOut += p[0]->size;
        } while (j != 0 && (p[-1]->tag & 0x800));
    }
    *idxOut = (i - j) + 1;
    if ((elems[j]->tag & 0x1ff) != 0x100) {
encode:
        _A_EncodeType(0, &sublen, 0, elems[j]->tag, elems[j]->tagBytes, 0, *lenOut);
        _A_EncodeType(base + size - (*lenOut + sublen), &sublen, sublen, elems[j]->tag,
                      elems[j]->tagBytes, 0, *lenOut);
        *lenOut += sublen;
    }
}

/* Finds the offset from data at which target/targetLen goes in a SET sorted on the encodings;
 * totalLen when it goes last. Returns the error of _A_DecodeType, if any. */
int _A_FindPositionInSet(unsigned int *outPos, void *target, unsigned int targetLen,
                          unsigned char *data, unsigned int totalLen)
{
    unsigned int pos;
    unsigned int recognized;
    unsigned int decodedType;
    unsigned int scratch3;
    ITEM out;
    int status;
    int cmp;
    unsigned int cmpLen;

    pos = 0;
    while (pos < totalLen) {
        status = _A_DecodeType(&recognized, &decodedType, &scratch3, &out,
                                data + pos, totalLen - pos);
        if (status != 0)
            return status;

        cmpLen = (targetLen < recognized) ? targetLen : recognized;
        cmp = T_memcmp(target, data + pos, cmpLen);

        if (cmp < 0 || (cmp == 0 && targetLen < recognized)) {
            *outPos = pos;
            return 0;
        }
        pos += recognized;
    }

    *outPos = totalLen;
    return 0;
}


/* The total length (header, contents and end of contents) of an indefinite-length element: decodes
 * its header, then walks the contents recursively up to the 00 00 octets. */
int _A_GetIndefiniteElementLen(unsigned int *outLen, unsigned char *data, unsigned int maxlen)
{
    unsigned int tag;
    unsigned int hdrExtra;
    ITEM content;
    unsigned int pos;
    unsigned int step;
    unsigned int subLen;
    int status;

    status = _A_DecodeType(outLen, &tag, &hdrExtra, &content, data, maxlen);
    if (status != 0)
        return status;
    if (*outLen != 0)
        return 0;

    pos = 0;
    step = hdrExtra + 4;

    while (content.data[pos] != 0 || content.data[pos + 1] != 0) {
        status = _A_GetIndefiniteElementLen(&subLen, content.data + pos, maxlen - (pos + step));
        if (status != 0)
            return status;
        pos += subLen;
    }

    *outLen = pos + step;
    return 0;
}

void ASN_SetEncodeFieldName(AsnEncodeCtx *ctx, char *name)
{
    if (ctx->encodeFieldName != 0)
        T_free(ctx->encodeFieldName);
    ctx->encodeFieldName = name;
}

void ASN_SetDecodeFieldName(BER_ELEMENT *item, char *name)
{
    if (item->decodeFieldName != 0)
        T_free(item->decodeFieldName);
    item->decodeFieldName = name;
}

/* Builds the hierarchical field name "<ctx->name>[<index>].<name>" in the buffer of `name`, growing
 * it in place; index (unsigned)-1 means no index. Returns its first argument, a status passed
 * through. */
int _A_ConstructFieldName(int status, AsnTemplateEntry *ctx, char *name,
                          unsigned int index, char **out)
{
    unsigned int extra;
    unsigned int v;
    unsigned int nlen;
    unsigned int tlen;
    unsigned int pos;
    char *buf;

    if (ctx->name == 0)
        buf = name;
    else {
        if (index == (unsigned int)-1)
            extra = 0;
        else {
            extra = 3;
            v = index;
            while (v >= 11) {
                extra++;
                v = v / 10;
            }
        }
        if (extra != 0)
            _A_StripFirstField(&name);
        tlen = 0;
        if (name != 0)
            tlen = T_strlen(name) + 1;
        nlen = T_strlen(ctx->name);
        pos = nlen + extra;
        buf = T_realloc(name, pos + tlen + 1);
        if (buf == 0) {
            *out = 0;
            return status;
        }
        if (tlen != 0) {
            T_memmove(buf + nlen + extra + 1, buf, tlen + 1);
            buf[pos] = '.';
        } else
            buf[pos] = 0;
        T_memcpy(buf, ctx->name, nlen);
        if (index != (unsigned int)-1) {
            extra--;
            buf[nlen + extra] = ']';
            do {
                extra--;
                buf[nlen + extra] = index % 10 + '0';
                index = index / 10;
            } while (index != 0);
            buf[nlen] = '[';
        }
    }
    *out = buf;
    return status;
}

/* Removes the first field of a hierarchical name, "a.b.c" -> "b.c", in place; with no '.' the
 * buffer is freed and *p set to 0. */
void _A_StripFirstField(char **p)
{
    unsigned int i;

    if (*p == 0)
        return;
    for (i = 0; (*p)[i] != '.' && (*p)[i] != '\0'; i++)
        ;
    if ((*p)[i] == '\0') {
        T_free(*p);
        *p = 0;
    } else {
        i++;
        T_memmove(*p, *p + i, T_strlen(*p + i) + 1);
    }
}
