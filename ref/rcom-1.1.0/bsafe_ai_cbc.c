/*
 * The algorithm info type CBC (AI_CBC, AIT_CBCAddInfo).
 */
#include "bsafe.h"
#include "bsafe_aitype.h"
#include "ah.h"



static B_AlgorithmInfoTypeVTable V_TABLE = {
    AIT_CBCAddInfo, 0, B_AlgorithmInfoTypeMakeError
};
B_AlgorithmInfoType AIT_CBC = { &V_TABLE };

int AI_CBC(void **info)
{
    *info = &AIT_CBC;
    return 0;
}

int AIT_CBCAddInfo(void *key, BsafeInfoCache *cache, void **header)
{
    ITEM *iv;
    AitInitVectorEntry *ivEntry;
    int status = B_MemoryPoolAlloc(&cache->pool, (void **)&iv, 8);
    if (status != 0)
        return status;
    status = B_MemoryPoolAllocAndCopy(&cache->pool, (void **)&iv->data, header[0], (unsigned int)header[1]);
    if (status != 0)
        return status;
    iv->len = (unsigned int)header[1];
    status = B_MemoryPoolAlloc(&cache->pool, (void **)&ivEntry, 8);
    if (status != 0)
        return status;
    ivEntry->params = iv;
    ivEntry->reInitIV = AHFeedbackCipherReInitIV;
    status = B_InfoCacheAddInfo(cache, AIT_INIT_VECTOR, ivEntry);
    if (status != 0)
        return status;
    return B_InfoCacheAddInfo(cache, key, iv);
}
