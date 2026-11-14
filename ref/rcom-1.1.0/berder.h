/*
 * The BERDecode/DEREncode layer (berder.c) and its legacy wrappers (oldber.c): the element the BER
 * decoders read and the value records they fill.
 */
#ifndef BERDER_H
#define BERDER_H

#include "aglobal.h"

/* Status codes of the BER/DER and ASN.1 layer (_A_BSafeError turns them into BE_*). 2053..2057
 * are decoding errors of a named field (_A_ConstructFieldName). */
#define BER_ERR_INPUT_LEN 2048   /* the encoding is shorter than its header says */
#define BER_ERR_INPUT_DATA 2049  /* malformed encoding */
#define BER_ERR_OUTPUT_LEN 2050  /* the output buffer is too small */
#define BER_ERR_ALLOC 2051
#define BER_ERR_DATA 2052        /* a value out of range */

/* The element passed to the BERDecode* callbacks of the ASN template engine
 * (_A_DecodeElement builds it on its stack). */
typedef struct BER_ELEMENT {
    /* 0x00 */ int tag;  /* 0x800: one segment of a value split in several */
    /* 0x04 */ unsigned char *next;  /* the byte after the identifier */
    /* 0x08 */ void *info;
    /* 0x0C */ unsigned char *data;  /* content bytes */
    /* 0x10 */ unsigned int len;  /* content length in bytes */
    /* 0x14 */ unsigned char *buf;  /* start of the encoding */
    /* 0x18 */ unsigned int elemLen;  /* length of the whole encoding */
    /* 0x1C */ char *decodeFieldName;  /* set on a decoding error */
} BER_ELEMENT; /* size = 0x20 */

/* A decoded BIT STRING: the first content byte is the count of unused bits
 * in the last byte (0-7). */
typedef struct BerBitString {
    unsigned char *data;
    unsigned int len;
    unsigned int unused;
} BerBitString;

/* The BIT STRING to encode: `unused` is a byte here. */
typedef struct DerBitStringIn {
    unsigned char *data;
    unsigned int len;
    unsigned char unused;
} DerBitStringIn;

/* The values of the optional fields: `present` is set when the field is in the encoding. */
typedef struct BerOptionalInt {
    int present;
    int value;
} BerOptionalInt;

typedef struct BerOptionalUint2 {
    int present;
    unsigned short value;
} BerOptionalUint2;

typedef struct BerOptionalUint4 {
    int present;
    unsigned int value;
} BerOptionalUint4;

int BERDecodeItem(BER_ELEMENT *item, ITEM *out);
int BERDecodeBitString(BER_ELEMENT *item, BerBitString *out);
int BERDecodeUint2(BER_ELEMENT *item, unsigned short *out);
int BERConstraintUint2(BER_ELEMENT *item, unsigned short *out);
int BERDecodeUint4(BER_ELEMENT *item, unsigned int *out);
int BERDecodeUnsignedInt(BER_ELEMENT *item, ITEM *out);
int BERDecodeUnsignedIntPointer(BER_ELEMENT *item, ITEM *out);
int BERDecodeUTCTime(BER_ELEMENT *item, void *out);
int DEREncodeItem(void *ctx, unsigned int tag, unsigned char *tagBytes, ITEM *in);
int DEREncodeBitString(void *ctx, unsigned int tag, unsigned char *tagBytes, DerBitStringIn *in);
int DEREncodeNamedBitString(void *ctx, unsigned int tag, unsigned char *tagBytes, DerBitStringIn *in);
int DEREncodeUint2(void *ctx, unsigned int tag, unsigned char *tagBytes, unsigned short *v);
int DEREncodeUint4(void *ctx, unsigned int tag, unsigned char *tagBytes, unsigned int *v);
int DEREncodeUnsignedInt(void *ctx, unsigned int tag, unsigned char *tagBytes, ITEM *in);
int DEREncodeUTCTime(void *ctx, unsigned int tag, unsigned char *tagBytes, ITEM *in);
int DEREncodeFull(void *ctx, unsigned int tag, unsigned char *tagBytes, ITEM *in);

/* berder.c */
int BERDecodeInt(BER_ELEMENT *item, int *out);

/* oldber.c */
int OldBERConstrainUINT2(BER_ELEMENT *item, unsigned short *out);
int OldBERGetBitString(BER_ELEMENT *item, BerBitString *out);
int OldBERGetCanonicalInteger(BER_ELEMENT *item, ITEM *out);
int OldBERGetContents(BER_ELEMENT *item, ITEM *out);
int OldBERSetBitString(void *ctx, unsigned int tag, unsigned char *tagBytes, DerBitStringIn *in);
int OldBERSetContents(void *ctx, unsigned int tag, unsigned char *tagBytes, ITEM *in);
int OldBERSetNonNegativeInteger(void *ctx, unsigned int tag, unsigned char *tagBytes, ITEM *in);
int OldBERSetUINT2(void *ctx, unsigned int tag, unsigned char *tagBytes, unsigned short *v);
int _A_BSafeError(int status);

#endif
