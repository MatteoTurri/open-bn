/*
 * The ASN.1 template engine of BSAFE: the template entries, the elements an encoding collects and
 * the encoding context.
 */
#ifndef ASN_H
#define ASN_H

#include "aglobal.h"
#include "berder.h"

struct MemPool;

/* One element collected by ASN_AddElement: the content and its tag. A constructed value is
 * opened by an element whose data is &_A_BeginConstructed and closed by tag 0x103. */
typedef struct AsnElement {
    /* 0x00 */ void *data;
    /* 0x04 */ unsigned int size;
    /* 0x08 */ int tag;  /* 0x800: continued in the next element */
    /* 0x0C */ unsigned char *tagBytes;  /* the tag number octets of a tag 31 (high tag number) */
} AsnElement; /* size = 0x10 */

/* The output stream of an encoding: a single write method, (stream, data, len), defined by each
 * kind of stream on its own type. */
typedef struct AsnOutStream AsnOutStream;

typedef struct AsnOutStreamVtbl {
    int (*write)();
} AsnOutStreamVtbl;

struct AsnOutStream {
    /* 0x00 */ AsnOutStreamVtbl *vtbl;
};

/* The output stream of ASN_EncodeAlloc: the encoding goes to a newly allocated ITEM. */
typedef struct AsnAllocStream {
    /* 0x00 */ AsnOutStream base;
    /* 0x04 */ ITEM *out;
} AsnAllocStream; /* size = 0x08 */

/* The source of an input stream: method 1 reads up to maxLen bytes. */
typedef struct AsnStreamSource AsnStreamSource;

typedef struct AsnStreamSourceVtbl {
    void *method0;
    int (*read)(AsnStreamSource *self, void *buf, unsigned int *outCount, unsigned int maxLen);
} AsnStreamSourceVtbl;

struct AsnStreamSource {
    /* 0x00 */ AsnStreamSourceVtbl *vtbl;
};

/* The input stream of a decoding (ASN_DecodeFromStream): refill (ASN_FlushStreamFlush) moves
 * the unread bytes to the start of the buffer and asks the source for more. */
typedef struct AsnStream AsnStream;

typedef struct AsnStreamVtbl {
    int (*refill)(AsnStream *st, unsigned char **buf, unsigned int *len);
} AsnStreamVtbl;

struct AsnStream {
    /* 0x00 */ AsnStreamVtbl *vtbl;
    /* 0x04 */ unsigned char *buffer;  /* 32000 bytes, allocated by the first refill */
    /* 0x08 */ unsigned int full;  /* the source gave less than asked: do not ask again */
    /* 0x0C */ unsigned int size;
    /* 0x10 */ unsigned int used;  /* bytes in the buffer */
    /* 0x14 */ unsigned int pos;  /* bytes consumed by the decoder */
    /* 0x18 */ AsnStreamSource *source;
}; /* size = 0x1C */

/* The encoding context (ASN_EncodeCtxConstructor/Destructor). */
typedef struct AsnEncodeCtx {
    /* 0x00 */ struct MemPool *pool;
    /* 0x04 */ unsigned int count;  /* elements added */
    /* 0x08 */ unsigned int capacity;  /* elements allocated (all valid) */
    /* 0x0C */ AsnElement **elements;
    /* 0x10 */ unsigned int elemSize;  /* size of buffer */
    /* 0x14 */ void *buffer;
    /* 0x18 */ char *encodeFieldName;  /* set on an encoding error */
    /* 0x1C */ AsnOutStream *stream;
} AsnEncodeCtx; /* size = 0x20 */

/* The value handlers of a template entry: each takes its own value type, so the types are
 * unprototyped. Encoders take (AsnEncodeCtx *ctx, int tag, unsigned char *tagBytes, value), decoders
 * (BER_ELEMENT *item, value). */
typedef int (*AsnEncodeFn)();
typedef int (*AsnDecodeFn)();

/* An entry of an ASN.1 template (arrays of 20-byte entries, 0x103 closes a level). */
typedef struct AsnTemplateEntry {
    /* 0x00 */ unsigned int flags;  /* tag in the low 9 bits; 0x20 constructed, 0x200 OPTIONAL,
                                       0x400 SET OF / SEQUENCE OF */
    /* 0x04 */ AsnDecodeFn decodeFn;
    /* 0x08 */ AsnEncodeFn encodeFn;
    /* 0x0C */ unsigned int valueIndex;  /* slot of the value in the caller's array */
    /* 0x10 */ char *name;  /* field name for the error messages */
} AsnTemplateEntry; /* size = 0x14 */

/* a_encdec.c */
int ASN_Decode(AsnTemplateEntry *e, void *info, unsigned char *data, unsigned int len,
               void *outVal, void *values);
int ASN_DecodeAny(unsigned int type, unsigned char *data, unsigned int len, ITEM *out);
int ASN_EncodeAnyAlloc(unsigned int type, void *data, unsigned int len, unsigned char **out,
                       unsigned int *outLen);
int ASN_EncodeCtxConstructor(AsnEncodeCtx *ctx, AsnOutStream *stream);
void ASN_EncodeCtxDestructor(AsnEncodeCtx *ctx);
int _A_DecodeElement(AsnTemplateEntry *e, void *info, unsigned char *buf, unsigned int len,
                     AsnStream *st, unsigned int *elemLen, AsnElement **items, char **out);
int _A_DecodeType(unsigned int *recognized, unsigned int *decodedType, unsigned int *extraCount,
                  ITEM *out, unsigned char *data, unsigned int maxlen);
int _A_EncodeElement(AsnEncodeCtx *ctx, AsnTemplateEntry *e, AsnElement **items, char **out);
int _A_EncodeType(unsigned char *buf, unsigned int *outLen, unsigned int size, unsigned int type,
                  unsigned char *contBytes, void *data, unsigned int len);

/* a_stream.c */
int _A_FlushStreamFlushExtended(AsnStream *ctx, unsigned char **outBuf, unsigned int *outLen,
                                unsigned int growAmount);

/* asn.c */
int ASN_AddElement(AsnEncodeCtx *ctx, int tag, unsigned char *tagBytes, void *data, unsigned int size);
int ASN_AddElementPointer(AsnEncodeCtx *ctx, int tag, unsigned char *tagBytes, void *data, unsigned int size);
void _A_ConstructElement(char *base, unsigned int *lenOut, unsigned int size, unsigned int *idxOut,
                         AsnElement **elems, unsigned int i);
int _A_ConstructFieldName(int status, AsnTemplateEntry *ctx, char *name, unsigned int index,
                          char **out);
int _A_FindPositionInSet(unsigned int *outPos, void *target, unsigned int targetLen,
                         unsigned char *data, unsigned int totalLen);
void _A_GetElementEntryCount(int *outCount, AsnTemplateEntry *entries);
int _A_GetElementLen(unsigned int *lenOut, unsigned int *idxOut, AsnElement **elems);
int _A_GetIndefiniteElementLen(unsigned int *outLen, unsigned char *data, unsigned int maxlen);
void _A_StripFirstField(char **p);

/* asn_alloc.c */
void ASN_AllocStreamConstructor(AsnAllocStream *ctx, ITEM *out);
int ASN_AllocStreamWrite(AsnAllocStream *ctx, void *data, unsigned int len);
int ASN_EncodeAlloc(AsnTemplateEntry *e, unsigned int flags, void *values, ITEM *out);

/* asn_stream.c */
int ASN_FlushStreamFlush(AsnStream *ctx, unsigned char **pcur, unsigned int *pcount);

/* The constant encodings of algorithm parameters (asn.c): the DER NULL, and the empty ITEM
 * compared by address (do not encode); the marker elements of constructed values. */
extern ITEM ASN_NullEncoding;
extern ITEM ASN_NoEncoding;
extern char _A_BeginConstructed;
extern char _A_EndConstructedSet;

#endif
