/*
 * B_KeySetInfo, B_KeyGetInfo and B_KeyAddItemInfo: the info of a key object, through its key info
 * type.
 */
#include "bsafe.h"
#include "bsafe_kitype.h"

/* The key info type of a generic {data, len} item; its address is the cache key. */


int B_KeySetInfo(BsafeKeyObj *key, B_KeyInfoType *infoType, void *info)
{
    if (key == 0)
        return BE_KEY_OBJ;
    if (key->cache.itemCount != 0)
        return BE_KEY_ALREADY_SET;
    return (*infoType->AddInfo)(key, info);
}

int B_KeyGetInfo(BsafeKeyObj *key, void **info, B_KeyInfoType *infoType)
{
    int status;
    if (key == 0)
        return BE_KEY_OBJ;
    if (key->cache.itemCount == 0)
        return BE_KEY_NOT_SET;
    status = B_InfoCacheFindInfo(&key->cache, info, infoType);
    if (status == 0)
        return 0;
    status = (*infoType->MakeInfo)(info, key);
    if (status != 0)
        return status;
    return B_InfoCacheAddInfo(&key->cache, infoType, *info);
}

int B_KeyAddItemInfo(BsafeKeyObj *key, unsigned char *data, unsigned int len)
{
    ITEM *item;
    int status = B_MemoryPoolAlloc(&key->cache.pool, (void **)&item, 8);
    if (status != 0)
        return status;
    item->data = data;
    item->len = len;
    return B_InfoCacheAddInfo(&key->cache, &KITItem, item);
}

int B_IntegerBits(const unsigned char *data, unsigned int len)
{
    unsigned int i;
    unsigned int bits;
    unsigned int mask;
    unsigned char byte;

    for (i = 0; i < len && data[i] == 0; i++)
        ;
    if (i == len)
        return 0;
    byte = data[i++];
    bits = 8;
    for (mask = 0x80; (byte & mask) == 0; mask >>= 1)
        bits--;
    return (len - i) * 8 + bits;
}
