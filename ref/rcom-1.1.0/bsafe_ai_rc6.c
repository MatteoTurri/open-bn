/*
 * The algorithm info type RC6 (AI_RC6, AIT_RC6AddInfo).
 */
#include "bsafe.h"
#include "bsafe_aitype.h"

static B_AlgorithmInfoTypeVTable V_TABLE = {
    AIT_RC6AddInfo, 0, B_AlgorithmInfoTypeMakeError
};
B_AlgorithmInfoType AIT_RC6 = { &V_TABLE };

int AI_RC6(void **info)
{
    *info = &AIT_RC6;
    return 0;
}

/* RC6: header[0] must be 20; the 8-byte entry holds the size. */
int AIT_RC6AddInfo(void *key, BsafeInfoCache *cache, unsigned int *header)
{
    unsigned int size = header[0];
    void *entry;
    int status;
    if (size != 20)
        return BE_WRONG_ALGORITHM_INFO;
    status = B_MemoryPoolAlloc(&cache->pool, &entry, 8);
    if (status != 0)
        return status;
    *(unsigned int *)entry = size;
    return B_InfoCacheAddInfo(cache, key, entry);
}
