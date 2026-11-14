/*
 * KIT_DES8: the key info type of an 8-byte DES key, checked for parity.
 */

#include "bsafe_kitype.h"
#include "bsafe.h"


B_KeyInfoType KIT_DES8 = { KIT_DES8AddInfo, KIT_DES8MakeInfo };

int KI_DES8(void **info)
{
    *info = &KIT_DES8;
    return 1;
}

int KIT_DES8AddInfo(void *ctx, void *header)
{
    void *entry;
    int status;

    status = B_MemoryPoolAllocAndCopy(ctx, &entry, header, 8);
    if (status != 0)
        return status;
    SetDESParity(entry);
    status = B_KeyAddItemInfo(ctx, entry, 8);
    if (status != 0)
        return status;
    return B_InfoCacheAddInfo(ctx, &KIT_DES8, entry);
}

int KIT_DES8MakeInfo(void **out, void *key)
{
    int status = B_KeyGetInfo(key, out, &KIT_8Byte);
    if (status != 0)
        return status;
    return CheckDESParity(*out);
}
