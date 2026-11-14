/*
 * The algorithm object of BSAFE: constructor, destructor, type checks, SetInfo and GetInfo.
 */
#include "bsafe.h"
#include "bsafe_aitype.h"


/* Looks name up, without case, in a table of attributes closed by 0 whose entries start with their
 * name; the entry goes to the out parameter. BE_NOT_SUPPORTED: unknown attribute. */




void B_AlgorithmConstructor(BsafeAlgorithmObj *obj)
{
    B_InfoCacheConstructor(&obj->cache);
    B_ExtendedErrorConstructor(&obj->extError);
    T_memset(&obj->sub, 0, 8);
}

void B_AlgorithmDestructor(BsafeAlgorithmObj *obj)
{
    BsafeAlgSub *sub;
    B_ExtendedErrorDestructor(&obj->extError);
    sub = obj->sub;
    if (sub != 0) {
        sub->base.destroy(sub);
        T_free(obj->sub);
    }
    B_MemoryPoolDestructor(&obj->cache.pool);
}

int B_AlgorithmCheckType(BsafeAlgorithmObj *obj, void *expectedTag)
{
    BsafeAlgSub *sub = obj->sub;
    if (sub != 0)
        return ((void *)sub->base.destroy == expectedTag) ? 0 : BE_ALG_OPERATION_UNKNOWN;
    return BE_ALGORITHM_NOT_SET;
}

int B_AlgorithmCheckTypeAndInitFlag(BsafeAlgorithmObj *obj, void *expectedTag)
{
    int status = B_AlgorithmCheckType(obj, expectedTag);
    if (status != 0)
        return status;
    return (obj->flags & 1) ? 0 : BE_ALGORITHM_NOT_INITIALIZED;
}

/* Once the AH object exists, SetInfo is allowed only for the three run-time info types (the IV of
 * CBC/CFB, the EC accelerator table, RESET_IV), each gated by its flag (2, 4, 8). Then the AddInfo
 * of the info type, and its NewHandler, if any, which builds the AH object. */
int B_AlgorithmSetInfo(BsafeAlgorithmObj *obj, B_AlgorithmInfoType *typeId, void *info)
{
    B_AlgorithmInfoTypeVTable *vtbl;
    int status;
    (void)info;

    if (obj->cache.itemCount != 0) {
        int flags = obj->flags;
        if (flags & 2) {
            if (typeId == &AIT_CBC_IV8)
                goto directSet;
        }
        if (flags & 4) {
            if (typeId == &AIT_ECAcceleratorTable)
                goto directSet;
        }
        if (!(flags & 8) || typeId != &AIT_RESET_IV)
            return BE_ALGORITHM_ALREADY_SET;
    }
directSet:
    status = (*typeId->vTable->AddInfo)(typeId, obj);
    if (status != 0)
        return status;
    vtbl = typeId->vTable;
    {
        int (*newHandler)() = vtbl->NewHandler;
        if (newHandler != 0) {
            BsafeAlgSub *newSub = (BsafeAlgSub *)(*newHandler)(typeId, obj);
            obj->sub = newSub;
            if (newSub == 0)
                return BE_ALLOC;
        }
    }
    return 0;
}

int B_AlgorithmGetInfo(BsafeAlgorithmObj *obj, void **info, B_AlgorithmInfoType *typeId)
{
    int status;
    B_AlgorithmInfoTypeVTable *vtbl;
    if (obj->cache.itemCount == 0)
        return BE_ALGORITHM_NOT_SET;
    status = B_InfoCacheFindInfo(&obj->cache, info, typeId);
    if (status == 0)
        return 0;
    vtbl = typeId->vTable;
    status = (*vtbl->MakeInfo)(typeId, info, obj);
    if (status != 0)
        return status;
    return B_InfoCacheAddInfo(&obj->cache, typeId, *info);
}
