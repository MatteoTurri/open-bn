/*
 * The memory pool of BSAFE (B_MemoryPool*): the blocks it owns, each with its destructor.
 */
#include "bsafe.h"

void B_MemoryPoolConstructor(BsafeMemoryPool *pool)
{
    T_memset(pool, 0, sizeof(BsafeMemoryPool));
}

void B_MemoryPoolDestructor(BsafeMemoryPool *pool)
{
    B_MemoryPoolReset(pool);
    T_free(pool->entries);
}

void B_MemoryPoolReset(BsafeMemoryPool *pool)
{
    unsigned int i;
    BsafePoolEntry *entry;

    i = pool->count;
    entry = pool->entries;
    while (i-- > 0) {
        if (entry->dataPtr != 0) {
            if (entry->destructor != 0) {
                entry->destructor(entry->dataPtr);
            } else {
                T_memset(entry->dataPtr, 0, entry->size);
                T_free(entry->dataPtr);
            }
            entry->dataPtr = 0;
        }
        entry++;
    }
    pool->count = 0;
}

int B_MemoryPoolAlloc(BsafeMemoryPool *pool, void **outPtr, unsigned int size)
{
    void *p = T_malloc(size);
    *outPtr = p;
    if (p == 0)
        return BE_ALLOC;
    return B_MemoryPoolAdoptData(pool, outPtr, size);
}

int B_MemoryPoolAllocAndCopy(BsafeMemoryPool *pool, void **outPtr, const void *src, unsigned int size)
{
    int status = B_MemoryPoolAlloc(pool, outPtr, size);
    if (status != 0)
        return status;
    T_memcpy(*outPtr, src, size);
    return 0;
}

int B_MemoryPoolAdoptData(BsafeMemoryPool *pool, void **pData, unsigned int size)
{
    int status;

    /* Adopts the block without a destructor; if the pool cannot grow, the block is cleared and freed
         * here. */
    status = B_MemoryPoolAdoptHelper(pool, *pData, size, 0);
    if (status != 0) {
        T_memset(*pData, 0, size);
        T_free(*pData);
        *pData = 0;
        return status;
    }
    return 0;
}

int B_MemoryPoolAdoptHelper(BsafeMemoryPool *pool, void *data, unsigned int size, BsafeDestructorFn destructor)
{
    if (pool->capacity < pool->count + 1) {
        unsigned int newCapacity = pool->count + 10;
        void *newBuf = T_malloc(newCapacity * sizeof(BsafePoolEntry));
        if (newBuf == 0)
            return BE_ALLOC;
        T_memcpy(newBuf, pool->entries, pool->count * sizeof(BsafePoolEntry));
        T_free(pool->entries);
        pool->entries = newBuf;
        pool->capacity = newCapacity;
    }
    pool->entries[pool->count].dataPtr = data;
    pool->entries[pool->count].size = size;
    {
        unsigned int cnt = pool->count;
        pool->entries[cnt].destructor = destructor;
        pool->count = cnt + 1;
    }
    return 0;
}

int B_MemoryPoolRealloc(BsafeMemoryPool *pool, void **pData, unsigned int newSize)
{
    BsafePoolEntry *entry = B_MemoryPoolFindAllocedObject(pool, *pData);
    void *newPtr = T_realloc(*pData, newSize);
    *pData = newPtr;
    if (newPtr == 0) {
        if (entry != 0)
            entry->dataPtr = 0;
        return BE_ALLOC;
    }
    if (entry == 0)
        return B_MemoryPoolAdoptData(pool, pData, newSize);
    entry->dataPtr = newPtr;
    entry->size = newSize;
    return 0;
}

void B_MemoryPoolFree(BsafeMemoryPool *pool, void **pData)
{
    BsafePoolEntry *entry = B_MemoryPoolFindAllocedObject(pool, *pData);
    if (entry != 0) {
        if (entry->destructor != 0) {
            entry->destructor(entry->dataPtr);
        } else {
            T_memset(*pData, 0, entry->size);
            T_free(*pData);
        }
        entry->dataPtr = 0;
    }
    *pData = 0;
}

BsafePoolEntry *B_MemoryPoolFindAllocedObject(BsafeMemoryPool *pool, void *dataPtr)
{
    unsigned int i;
    BsafePoolEntry *entry;

    if (dataPtr == 0)
        return 0;
    i = pool->count;
    entry = pool->entries;
    while (i-- > 0) {
        if (entry->dataPtr == dataPtr)
            return entry;
        entry++;
    }
    return 0;
}
