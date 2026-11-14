/*
 * KIT_24Byte: the key info type of a 24-byte key, copied as is.
 */

#include "aglobal.h"
#include "bsafe_kitype.h"
#include "bsafe.h"


B_KeyInfoType KIT_24Byte = { KIT_24ByteAddInfo, KIT_24ByteMakeInfo };

int KI_24Byte(void **info)
{
    *info = &KIT_24Byte;
    return 1;
}

int KIT_24ByteAddInfo(void *ctx, void *header)
{
    void *entry;
    int status;

    status = B_MemoryPoolAllocAndCopy(ctx, &entry, header, 24);
    if (status != 0)
        return status;
    status = B_KeyAddItemInfo(ctx, entry, 24);
    if (status != 0)
        return status;
    return B_InfoCacheAddInfo(ctx, &KIT_24Byte, entry);
}

int KIT_24ByteMakeInfo(unsigned char **out, void *key)
{
    ITEM *data;
    int status = B_KeyGetInfo(key, (void **)&data, &KITItem);
    if (status != 0)
        return status;
    if (data->len != 24)
        return BE_WRONG_KEY_INFO;
    *out = data->data;
    return 0;
}
