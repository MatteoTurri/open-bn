/*
 * The algorithm info type ECAcceleratorTable (AI_ECAcceleratorTable, AIT_ECAcceleratorTableAddInfo).
 */
#include "bsafe.h"
#include "bsafe_aitype.h"


unsigned char AIT_ECAcceleratorTablePointer[12];

static B_AlgorithmInfoTypeVTable V_TABLE = {
    AIT_ECAcceleratorTableAddInfo, 0, B_AlgorithmInfoTypeMakeError
};
B_AlgorithmInfoType AIT_ECAcceleratorTable = { &V_TABLE };

int AI_ECAcceleratorTable(void **info)
{
    *info = &AIT_ECAcceleratorTable;
    return 0;
}

/* Returns 0 at once if the table is already registered under its own tag; otherwise needs an entry
 * under AIT_ECAcceleratorTablePointer, fills it with a copy of header and clears flag 4 of the
 * algorithm object. */
int AIT_ECAcceleratorTableAddInfo(void *key, BsafeAlgorithmObj *obj, void **header)
{
    void *existing;
    ITEM **slot;
    ITEM *table;
    int status;
    status = B_InfoCacheFindInfo(&obj->cache, &existing, &AIT_ECAcceleratorTable);
    if (status == 0)
        return 0;
    status = B_InfoCacheFindInfo(&obj->cache, (void *)&slot, AIT_ECAcceleratorTablePointer);
    if (status != 0)
        return BE_ALGORITHM_NOT_SET;
    status = B_MemoryPoolAlloc(&obj->cache.pool, (void **)&table, 8);
    if (status != 0)
        return status;
    table->len = (unsigned int)header[1];
    *slot = table;
    status = B_MemoryPoolAllocAndCopy(&obj->cache.pool, (void **)&table->data, header[0], table->len);
    if (status != 0)
        return status;
    obj->flags &= ~4;
    return B_InfoCacheAddInfo(&obj->cache, key, table);
}
