/*
 * KIT_8Byte: the key info type of an 8-byte key, copied as is.
 */

#include "aglobal.h"
#include "bsafe_kitype.h"
#include "bsafe.h"


B_KeyInfoType KIT_8Byte = { KIT_8ByteAddInfo, KIT_8ByteMakeInfo };

int KI_8Byte(void **info)
{
    *info = &KIT_8Byte;
    return 1;
}

int KIT_8ByteAddInfo(void *ctx, void *header)
{
    void *entry;
    int status;

    status = B_MemoryPoolAllocAndCopy(ctx, &entry, header, 8);
    if (status != 0)
        return status;
    status = B_KeyAddItemInfo(ctx, entry, 8);
    if (status != 0)
        return status;
    return B_InfoCacheAddInfo(ctx, &KIT_8Byte, entry);
}

int KIT_8ByteMakeInfo(unsigned char **out, void *key)
{
    ITEM *data;
    int status = B_KeyGetInfo(key, (void **)&data, &KITItem);
    if (status != 0)
        return status;
    if (data->len != 8)
        return BE_WRONG_KEY_INFO;
    *out = data->data;
    return 0;
}
