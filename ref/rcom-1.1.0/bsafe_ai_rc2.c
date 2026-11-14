/*
 * The algorithm info type RC2 (AI_RC2, AIT_RC2AddInfo).
 */
#include "bsafe.h"
#include "bsafe_aitype.h"

static B_AlgorithmInfoTypeVTable V_TABLE = {
    AIT_RC2AddInfo, 0, B_AlgorithmInfoTypeMakeError
};
B_AlgorithmInfoType AIT_RC2 = { &V_TABLE };

int AI_RC2(void **info)
{
    *info = &AIT_RC2;
    return 0;
}

/* RC2: one bound on header[0] (the key size in bytes); the 4-byte entry is a copy of the field. */
int AIT_RC2AddInfo(void *key, BsafeInfoCache *cache, unsigned int *header)
{
    unsigned int *entry;
    int status;
    if (header[0] >= 1025)
        return BE_ALGORITHM_INFO;
    status = B_MemoryPoolAlloc(&cache->pool, (void **)&entry, 4);
    if (status != 0)
        return status;
    *entry = header[0];
    return B_InfoCacheAddInfo(cache, key, entry);
}
