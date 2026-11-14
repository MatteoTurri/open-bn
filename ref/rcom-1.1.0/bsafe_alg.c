/*
 * Algorithm objects of BSAFE: creating and destroying them, their information and checks.
 */
#include "bsafe.h"

/* The tag AlgorithmWrapCheck checks in an algorithm object. */
static void *bsafeAlgObjTag = 0;


int B_CreateAlgorithmObject(BsafeAlgorithmObj **outObj)
{
    BsafeAlgorithmObj *obj = T_malloc(sizeof(BsafeAlgorithmObj));
    *outObj = obj;
    if (obj == 0)
        return BE_ALLOC;
    B_AlgorithmConstructor(obj);
    obj->tag = &bsafeAlgObjTag;
    obj->selfCheck = obj;
    return 0;
}

void B_GetExtendedErrorInfo(BsafeAlgorithmObj *obj, int *errorData, void **errorCode)
{
    BsafeExtendedError *ee = &obj->extError;
    errorData[1] = ee->resizeCtx.size;
    errorData[0] = (int)ee->resizeCtx.dataPtr;
    *errorCode = (void *)obj->extError.errorCode;
}

void B_ExtendedErrorDestructor(BsafeExtendedError *ee)
{
    if (ee != 0) {
        ResizeContextDestructor(&ee->resizeCtx);
    }
}

void B_ExtendedErrorConstructor(BsafeExtendedError *ee)
{
    ResizeContextConstructor(&ee->resizeCtx);
    ee->errorCode = 0;
}

void B_DestroyAlgorithmObject(BsafeAlgorithmObj **pObj)
{
    BsafeAlgorithmObj *obj = *pObj;
    int status = AlgorithmWrapCheck(obj);
    if (status != 0) {
        *pObj = 0;
        return;
    }
    obj->selfCheck = 0;
    B_AlgorithmDestructor(obj);
    T_free(obj);
    *pObj = 0;
}

int B_SetAlgorithmInfo(BsafeAlgorithmObj *obj, BsafeInfoChooser infoType, void *info)
{
    void *typeId;
    int status = AlgorithmWrapCheck(obj);
    if (status != 0)
        return status;
    if (infoType(&typeId) != 0)
        return BE_KEY_OPERATION_UNKNOWN;
    return B_AlgorithmSetInfo(obj, typeId, info);
}

int B_GetAlgorithmInfo(void *info, BsafeAlgorithmObj *obj, BsafeInfoChooser infoType)
{
    void *typeId;
    int status = AlgorithmWrapCheck(obj);
    if (status != 0)
        return status;
    if (infoType(&typeId) != 0)
        return BE_KEY_OPERATION_UNKNOWN;
    return B_AlgorithmGetInfo(obj, info, typeId);
}

int AlgorithmWrapCheck(BsafeAlgorithmObj *obj)
{
    if (obj != 0) {
        if (obj->selfCheck == obj) {
            if (obj->tag == &bsafeAlgObjTag)
                goto success;
        }
    }
    return BE_ALGORITHM_OBJ;
success:
    return 0;
}

int RandomAlgorithmCheck(BsafeAlgorithmObj *randomAlg)
{
    int status;
    if (randomAlg == 0)
        return 0;
    status = AlgorithmWrapCheck(randomAlg);
    return status == 0 ? 0 : BE_RANDOM_OBJ;
}
