/*
 * The BER encoding of the key info structures: Decode/Encode PublicKeyInfo (SubjectPublicKeyInfo)
 * and PrivateKeyInfo (PKCS #8). The value arrays start at index 1.
 */

#include "asn.h"
#include "berder.h"
#include "bsafe.h"


/* The templates of SubjectPublicKeyInfo and PrivateKeyInfo, and the version of PrivateKeyInfo
 * (an INTEGER handled as a UINT2), 0. */

static AsnTemplateEntry PublicKeyInfoTemplate[] = {
    { 0x30, 0, 0, 0, 0 },
    { 0x30, 0, 0, 0, 0 },
    { 6, OldBERGetContents, OldBERSetContents, 1, 0 },
    { 0x302, 0, 0, 2, 0 },
    { 0x103, 0, 0, 0, 0 },
    { 3, OldBERGetBitString, OldBERSetBitString, 3, 0 },
    { 0x103, 0, 0, 0, 0 }
};
static AsnTemplateEntry PrivateKeyInfoTemplate[] = {
    { 0x30, 0, 0, 0, 0 },
    { 2, OldBERConstrainUINT2, OldBERSetUINT2, 1, 0 },
    { 0x30, 0, 0, 0, 0 },
    { 6, OldBERGetContents, OldBERSetContents, 2, 0 },
    { 0x302, 0, 0, 3, 0 },
    { 0x103, 0, 0, 0, 0 },
    { 4, OldBERGetContents, OldBERSetContents, 4, 0 },
    { 0x12a0, 0, 0, 0, 0 },
    { 0x103, 0, 0, 0, 0 },
    { 0x103, 0, 0, 0, 0 }
};
static unsigned short PrivateKeyInfoVersion = 0;

int DecodePublicKeyInfo(void *algorithmOid, void *algorithmParams, ITEM *out, ITEM *in)
{
    BerBitString val;
    void *values[4];
    int status;

    T_memset(values, 0, 16);
    values[1] = algorithmOid;
    values[2] = algorithmParams;
    values[3] = &val;
    status = _A_BSafeError(ASN_Decode(PublicKeyInfoTemplate, 0,
                                      in->data, in->len, 0, values));
    if (status != 0)
        return status;
    if (val.unused != 0)
        return BE_INPUT_DATA;
    out->data = val.data;
    out->len = val.len;
    return 0;
}

int DecodePrivateKeyInfo(void *algorithmOid, void *algorithmParams, void *privateKey, ITEM *in)
{
    void *values[5];

    T_memset(values, 0, 20);
    values[1] = &PrivateKeyInfoVersion;
    values[2] = algorithmOid;
    values[3] = algorithmParams;
    values[4] = privateKey;
    return _A_BSafeError(ASN_Decode(PrivateKeyInfoTemplate, 0,
                                    in->data, in->len, 0, values));
}

int EncodePublicKeyInfoAlloc(void *out, void *algorithmOid, void *algorithmParams, ITEM *in)
{
    BerBitString val;
    void *values[4];

    T_memset(values, 0, 16);
    values[1] = algorithmOid;
    values[2] = algorithmParams;
    val.unused = 0;
    values[3] = &val;
    val.data = in->data;
    val.len = in->len;
    return _A_BSafeError(ASN_EncodeAlloc(PublicKeyInfoTemplate, 0, values, out));
}

int EncodePrivateKeyInfoAlloc(void *out, void *algorithmOid, void *algorithmParams, void *privateKey)
{
    void *values[5];

    T_memset(values, 0, 20);
    values[1] = &PrivateKeyInfoVersion;
    values[2] = algorithmOid;
    values[3] = algorithmParams;
    values[4] = privateKey;
    return _A_BSafeError(ASN_EncodeAlloc(PrivateKeyInfoTemplate, 0, values, out));
}
