/*
 * The info cache of BSAFE (B_InfoCache*): a memory pool and a {key, value} list searched linearly.
 */
#include "bsafe.h"

void B_InfoCacheConstructor(BsafeInfoCache *cache)
{
    B_MemoryPoolConstructor(&cache->pool);
    T_memset(&cache->itemCount, 0, 8);
}

int B_InfoCacheAddInfo(BsafeInfoCache *cache, void *key, void *value)
{
    int status = B_MemoryPoolRealloc(&cache->pool, (void **)&cache->items,
                                      (cache->itemCount + 1) * sizeof(BsafeCacheItem));
    if (status != 0)
        return status;
    cache->items[cache->itemCount].key = key;
    cache->items[cache->itemCount].value = value;
    cache->itemCount = cache->itemCount + 1;
    return 0;
}

int B_InfoCacheFindInfo(BsafeInfoCache *cache, void *outValue, void *key)
{
    unsigned int i;

    for (i = 0; i < cache->itemCount; i++) {
        if (cache->items[i].key == key) {
            *(void **)outValue = cache->items[i].value;
            return 0;
        }
    }
    return BE_NOT_SUPPORTED;
}

void B_InfoCacheReset(BsafeInfoCache *cache)
{
    B_MemoryPoolReset(&cache->pool);
    T_memset(&cache->itemCount, 0, 8);
}
