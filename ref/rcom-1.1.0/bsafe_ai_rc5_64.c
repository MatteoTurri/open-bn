/*
 * The algorithm info type RC5_64 (AI_RC5_64, AIT_RC5_64AddInfo).
 */
#include "bsafe.h"
#include "bsafe_aitype.h"


static B_AlgorithmInfoTypeVTable V_TABLE = {
    AIT_RC5_64AddInfo, 0, B_AlgorithmInfoTypeMakeError
};
B_AlgorithmInfoType AIT_RC5_64 = { &V_TABLE };

int AI_RC5_64(void **info)
{
    *info = &AIT_RC5_64;
    return 0;
}

int AIT_RC5_64AddInfo(void *key, BsafeInfoCache *cache, unsigned int *header)
{
    unsigned int *entry;
    int status;
    if (header[0] >= 17)
        return BE_WRONG_ALGORITHM_INFO;
    if (header[1] >= 256)
        return BE_WRONG_ALGORITHM_INFO;
    if (header[2] != 64)
        return BE_WRONG_ALGORITHM_INFO;
    status = B_MemoryPoolAlloc(&cache->pool, (void **)&entry, 16);
    if (status != 0)
        return status;
    entry[0] = header[0];
    entry[1] = header[1];
    entry[2] = header[2];
    return B_InfoCacheAddInfo(cache, key, entry);
}
