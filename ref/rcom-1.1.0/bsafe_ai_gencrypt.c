/*
 * The algorithm info type GenericCrypt (AI_GenericCrypt, AIT_GenericCryptAddInfo).
 */
#include "bsafe.h"
#include "bsafe_aitype.h"


static B_AlgorithmInfoTypeVTable V_TABLE = {
    AIT_GenericCryptAddInfo, 0, B_AlgorithmInfoTypeMakeError
};
B_AlgorithmInfoType AIT_GenericCrypt = { &V_TABLE };

int AI_GenericCrypt(void **info)
{
    *info = &AIT_GenericCrypt;
    return 0;
}

/* An 8-byte entry {data, len}, data a copy of header[0..len) in the pool. */
int AIT_GenericCryptAddInfo(void *key, BsafeInfoCache *cache, void **header)
{
    ITEM *entry;
    int status = B_MemoryPoolAlloc(&cache->pool, (void **)&entry, 8);
    if (status != 0)
        return status;
    status = B_MemoryPoolAllocAndCopy(&cache->pool, (void **)&entry->data, header[0], (unsigned int)header[1]);
    if (status != 0)
        return status;
    entry->len = (unsigned int)header[1];
    return B_InfoCacheAddInfo(cache, key, entry);
}
