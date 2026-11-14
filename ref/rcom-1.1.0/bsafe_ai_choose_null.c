/*
 * AITChooseEncryptNewHandler and AITNullAddInfo: the methods of AITChooseEncryptNull_V_TABLE, shared
 * by the algorithm info types without parameters.
 */
#include "bsafe.h"
#include "bsafe_aitype.h"
#include "ah.h"


B_AlgorithmInfoTypeVTable AITChooseEncryptNull_V_TABLE = {
    AITNullAddInfo, AITChooseEncryptNewHandler, B_AlgorithmInfoTypeMakeError
};

/* Builds a new AHChooseEncrypt from the entry stored under key; 0 when there is none. */
int AITChooseEncryptNewHandler(void *key, BsafeAlgorithmObj *algorithm)
{
    void *found;
    if (B_InfoCacheFindInfo(&algorithm->cache, &found, key) != 0)
        return 0;
    return (int)AHChooseEncryptConstructor2(0, &algorithm->extError, key, found);
}

/* Registers key with the value 0: there is no entry to allocate. */
int AITNullAddInfo(void *key, BsafeInfoCache *cache, void **header)
{
    return B_InfoCacheAddInfo(cache, key, 0);
}
