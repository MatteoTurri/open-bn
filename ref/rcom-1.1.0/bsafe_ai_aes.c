/*
 * The algorithm info types of AES in ECB mode (AI_AES_ECB, AI_AES128/192/256_ECB).
 */
#include "bsafe.h"
#include "bsafe_aitype.h"
#include "ah.h"

static int AITECBNewHandler(void *key, BsafeAlgorithmObj *algorithm);

static B_AlgorithmInfoTypeVTable V_TABLE = {
    AIT_AESAddInfo, AITECBNewHandler, B_AlgorithmInfoTypeMakeError
};
B_AlgorithmInfoType AIT_AES_ECB = { &V_TABLE };
B_AlgorithmInfoType AIT_AES128_ECB = { &V_TABLE };
B_AlgorithmInfoType AIT_AES192_ECB = { &V_TABLE };
B_AlgorithmInfoType AIT_AES256_ECB = { &V_TABLE };

int AI_AES_ECB(void **info)
{
    *info = &AIT_AES_ECB;
    return 0;
}

int AI_AES128_ECB(void **info)
{
    *info = &AIT_AES128_ECB;
    return 0;
}

int AI_AES192_ECB(void **info)
{
    *info = &AIT_AES192_ECB;
    return 0;
}

int AI_AES256_ECB(void **info)
{
    *info = &AIT_AES256_ECB;
    return 0;
}

/* AES: a 4-byte entry with the block size, 16. */
int AIT_AESAddInfo(void *key, BsafeInfoCache *cache)
{
    void *entry;
    int status = B_MemoryPoolAlloc(&cache->pool, &entry, 4);
    if (status != 0)
        return status;
    *(unsigned int *)entry = 16;
    return B_InfoCacheAddInfo(cache, key, entry);
}


/* Builds an AHFeedbackCipher already bound to the ECB feedback mode (type 3). */
static int AITECBNewHandler(void *key, BsafeAlgorithmObj *algorithm)
{
    void *found;

    if (B_InfoCacheFindInfo(&algorithm->cache, &found, key) != 0)
        return 0;
    return (int)AHFeedbackCipherConstructor(0, &algorithm->extError, key, 0,
                                            &AIT_ECB, found, 0, 3);
}
