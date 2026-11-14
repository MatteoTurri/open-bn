/*
 * The key object of BSAFE: creating, destroying and checking it, and B_SetKeyInfo/B_GetKeyInfo.
 */
#include "bsafe.h"
#include "bsafe_kitype.h"

/* The key info type of a generic {data, len} item; its address is the cache key. */

/* The tag KeyWrapCheck checks in a key object. */
static void *bsafeKeyObjTag = 0;

int B_CreateKeyObject(BsafeKeyObj **outKey)
{
    BsafeKeyObj *key = T_malloc(sizeof(BsafeKeyObj));
    *outKey = key;
    if (key == 0)
        return BE_ALLOC;
    B_InfoCacheConstructor(&key->cache);
    B_ExtendedErrorConstructor(&key->extError);
    key->tag = &bsafeKeyObjTag;
    key->selfCheck = key;
    return 0;
}

void B_GetKeyExtendedErrorInfo(BsafeKeyObj *key, int *errorData, void **errorCode)
{
    BsafeExtendedError *ee = &key->extError;
    errorData[1] = ee->resizeCtx.size;
    errorData[0] = (int)ee->resizeCtx.dataPtr;
    *errorCode = (void *)key->extError.errorCode;
}

void B_DestroyKeyObject(BsafeKeyObj **pKey)
{
    BsafeKeyObj *key = *pKey;
    if (key != 0) {
        int status = KeyWrapCheck(key);
        if (status != 0) {
            *pKey = 0;
            return;
        }
        key->selfCheck = 0;
        B_ExtendedErrorDestructor(&key->extError);
        B_MemoryPoolDestructor(&key->cache.pool);
        T_free(key);
        *pKey = 0;
    }
}

int B_SetKeyInfo(BsafeKeyObj *key, BsafeInfoChooser chooser, void *info)
{
    void *typeId;
    int status = KeyWrapCheck(key);
    if (status != 0)
        return status;
    if (chooser(&typeId) == 0)
        return BE_ALG_OPERATION_UNKNOWN;
    return B_KeySetInfo(key, typeId, info);
}

int B_GetKeyInfo(void *info, BsafeKeyObj *key, BsafeInfoChooser chooser)
{
    void *typeId;
    int status = KeyWrapCheck(key);
    if (status != 0)
        return status;
    if (chooser(&typeId) == 0)
        return BE_ALG_OPERATION_UNKNOWN;
    return B_KeyGetInfo(key, info, typeId);
}

int KeyWrapCheck(BsafeKeyObj *key)
{
    if (key == 0)
        return 0;
    if (key->selfCheck == key) {
        if (key->tag == &bsafeKeyObjTag)
            goto success;
    }
    return BE_KEY_OBJ;
success:
    return 0;
}
