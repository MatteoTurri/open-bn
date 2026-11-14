/*
 * The algorithm info type CFB (AI_CFB, AIT_CFBAddInfo).
 */
#include "bsafe.h"
#include "bsafe_aitype.h"
#include "ah.h"



/* CFB parameters: the IV and the number of bits fed back per step. */
typedef struct {
    /* 0x00 */ ITEM iv;
    /* 0x08 */ unsigned int transferSize;
} CFBParams; /* size = 0x0C */

static B_AlgorithmInfoTypeVTable V_TABLE = {
    AIT_CFBAddInfo, 0, B_AlgorithmInfoTypeMakeError
};
B_AlgorithmInfoType AIT_CFB = { &V_TABLE };

int AI_CFB(void **info)
{
    *info = &AIT_CFB;
    return 0;
}

int AIT_CFBAddInfo(void *key, BsafeInfoCache *cache, unsigned int *header)
{
    CFBParams *params;
    AitInitVectorEntry *ivEntry;
    int status = B_MemoryPoolAlloc(&cache->pool, (void **)&params, 12);
    if (status != 0)
        return status;
    status = B_MemoryPoolAllocAndCopy(&cache->pool, (void **)&params->iv.data, (void *)header[0], header[1]);
    if (status != 0)
        return status;
    params->iv.len = header[1];
    params->transferSize = header[2];
    status = B_MemoryPoolAlloc(&cache->pool, (void **)&ivEntry, 8);
    if (status != 0)
        return status;
    ivEntry->params = params;
    ivEntry->reInitIV = AHFeedbackCipherReInitIV;
    status = B_InfoCacheAddInfo(cache, AIT_INIT_VECTOR, ivEntry);
    if (status != 0)
        return status;
    return B_InfoCacheAddInfo(cache, key, params);
}
