/*
 * The algorithm info type ECB (AI_ECB, AIT4ByteAddInfo).
 */
#include "bsafe.h"
#include "bsafe_aitype.h"

static int AIT4ByteAddInfo(void *key, void *cache, const void *src);

static B_AlgorithmInfoTypeVTable V_TABLE = {
    AIT4ByteAddInfo, 0, B_AlgorithmInfoTypeMakeError
};
B_AlgorithmInfoType AIT_ECB = { &V_TABLE };

int AI_ECB(void **info)
{
    *info = &AIT_ECB;
    return 0;
}

/* A generic 4-byte AddInfo: copies the 4 bytes of src into the pool and registers them under key. */
static int AIT4ByteAddInfo(void *key, void *cache, const void *src)
{
    void *entry;
    int status;

    status = B_MemoryPoolAlloc(cache, &entry, 4);
    if (status != 0)
        return status;
    T_memcpy(entry, src, 4);
    return B_InfoCacheAddInfo(cache, key, entry);
}
