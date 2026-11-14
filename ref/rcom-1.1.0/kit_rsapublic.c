/*
 * KIT_RSAPublic: the key info type of an RSA public key; its MakeInfo takes the modulus and the
 * public exponent of a multi-prime private key.
 */

#include "aglobal.h"
#include "bsafe_kitype.h"
#include "bsafe.h"


B_KeyInfoType KIT_RSAPublic = { KIT_RSAPublicAddInfo, KIT_RSAPublicMakeInfo };

int KI_RSAPublic(void **info)
{
    *info = &KIT_RSAPublic;
    return 1;
}

int KIT_RSAPublicAddInfo(void *ctx, void *header)
{
    void *buf;
    ITEM layout[2];
    ITEM *items[2];
    int status;

    status = B_MemoryPoolAlloc(ctx, &buf, 16);
    if (status != 0)
        return status;

    items[0] = &layout[0];
    items[1] = &layout[1];
    status = AllocAndCopyIntegerItems(buf, header, &layout[0], items, 2, ctx);
    if (status != 0)
        return status;

    return B_InfoCacheAddInfo(ctx, &KIT_RSAPublic, buf);
}

int KIT_RSAPublicMakeInfo(void **out, void *key)
{
    unsigned int *data;
    unsigned int *dst;
    int status = B_KeyGetInfo(key, (void **)&data, &KIT_PKCS_RSAMultiPrimePrivate);
    if (status != 0)
        return status;
    status = B_MemoryPoolAlloc(key, out, 16);
    if (status != 0)
        return status;
    dst = *out;
    dst[0] = data[0];
    dst[1] = data[1];
    dst[2] = data[2];
    dst[3] = data[3];
    return 0;
}
