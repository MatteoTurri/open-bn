/*
 * The legacy OldBER* wrappers of the BER/DER layer: each calls the BERDecode or DEREncode function
 * and translates its status with _A_BSafeError. Also _A_AlgaError.
 */
#include "bsafe.h"
#include "berder.h"

/* Translates a BER_ERR_* status into the algae code of the same meaning (6, 5, 10, 16, 2: the codes
 * ConvertAlgaeError turns into BE_INPUT_LEN, BE_INPUT_DATA, BE_OUTPUT_LEN, BE_ALLOC, BE_DATA). */
int _A_AlgaError(int status)
{
    switch (status - BER_ERR_INPUT_LEN) {
    case 0: return 6;
    case 1: return 5;
    case 2: return 10;
    case 3: return 16;
    case 4: return 2;
    default: return status;
    }
}

/* Translates a raw BSAFE status (600, 2048-2057) into a reason code (BE_ALLOC, BE_DATA, BE_INPUT_DATA, BE_INPUT_LEN, BE_OUTPUT_LEN); any
 * other value passes unchanged. */
int _A_BSafeError(int status)
{
    switch (status) {
    case BER_ERR_INPUT_LEN:
        return BE_INPUT_LEN;
    case BER_ERR_INPUT_DATA:
    case 2053:
    case 2054:
    case 2055:
    case 2056:
        return BE_INPUT_DATA;
    case BER_ERR_OUTPUT_LEN:
        return BE_OUTPUT_LEN;
    case 600:
    case BER_ERR_ALLOC:
        return BE_ALLOC;
    case BER_ERR_DATA:
    case 2057:
        return BE_DATA;
    default:
        return status;
    }
}

int OldBERConstrainUINT2(BER_ELEMENT *item, unsigned short *out)
{
    return _A_BSafeError(BERConstraintUint2(item, out));
}

/* OldBERGet*: the decoding side. */
int OldBERGetBitString(BER_ELEMENT *item, BerBitString *out)
{
    return _A_BSafeError(BERDecodeBitString(item, out));
}

int OldBERGetContents(BER_ELEMENT *item, ITEM *out)
{
    return _A_BSafeError(BERDecodeItem(item, out));
}

int OldBERGetNonNegativeInteger(BER_ELEMENT *item, ITEM *out)
{
    return _A_BSafeError(BERDecodeUnsignedInt(item, out));
}

int OldBERGetCanonicalInteger(BER_ELEMENT *item, ITEM *out)
{
    return _A_BSafeError(BERDecodeUnsignedIntPointer(item, out));
}

int OldBERGetUINT2(BER_ELEMENT *item, unsigned short *out)
{
    return _A_BSafeError(BERDecodeUint2(item, out));
}

int OldBERGetUINT4(BER_ELEMENT *item, unsigned int *out)
{
    return _A_BSafeError(BERDecodeUint4(item, out));
}

int OldBERGetUTCTime(BER_ELEMENT *item, void *out)
{
    return _A_BSafeError(BERDecodeUTCTime(item, out));
}

int OldBERSetBitString(void *ctx, unsigned int tag, unsigned char *tagBytes, DerBitStringIn *in)
{
    return _A_BSafeError(DEREncodeBitString(ctx, tag, tagBytes, in));
}

int OldBERSetNamedBitString(void *ctx, unsigned int tag, unsigned char *tagBytes, DerBitStringIn *in)
{
    return _A_BSafeError(DEREncodeNamedBitString(ctx, tag, tagBytes, in));
}

int OldBERSetContents(void *ctx, unsigned int tag, unsigned char *tagBytes, ITEM *in)
{
    return _A_BSafeError(DEREncodeItem(ctx, tag, tagBytes, in));
}

int OldBERSetEncoding(void *ctx, unsigned int tag, unsigned char *tagBytes, ITEM *in)
{
    return _A_BSafeError(DEREncodeFull(ctx, tag, tagBytes, in));
}

int OldBERSetNonNegativeInteger(void *ctx, unsigned int tag, unsigned char *tagBytes, ITEM *in)
{
    return _A_BSafeError(DEREncodeUnsignedInt(ctx, tag, tagBytes, in));
}

int OldBERSetUINT2(void *ctx, unsigned int tag, unsigned char *tagBytes, unsigned short *v)
{
    return _A_BSafeError(DEREncodeUint2(ctx, tag, tagBytes, v));
}

int OldBERSetUINT4(void *ctx, unsigned int tag, unsigned char *tagBytes, unsigned int *v)
{
    return _A_BSafeError(DEREncodeUint4(ctx, tag, tagBytes, v));
}

int OldBERSetUTCTime(void *ctx, unsigned int tag, unsigned char *tagBytes, ITEM *in)
{
    return _A_BSafeError(DEREncodeUTCTime(ctx, tag, tagBytes, in));
}
