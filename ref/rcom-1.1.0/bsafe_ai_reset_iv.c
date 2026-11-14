/*
 * The algorithm info type RESET_IV (AI_RESET_IV, AIT_RESET_IVAddInfo).
 */
#include "bsafe.h"
#include "bsafe_aitype.h"


unsigned char AIT_INIT_VECTOR[12];

static B_AlgorithmInfoTypeVTable V_TABLE = {
    AIT_RESET_IVAddInfo, 0, B_AlgorithmInfoTypeMakeError
};
B_AlgorithmInfoType AIT_RESET_IV = { &V_TABLE };

int AI_RESET_IV(void **info)
{
    *info = &AIT_RESET_IV;
    return 0;
}

/* Needs flag 8 of the algorithm object (CBC or CFB active); finds the entry registered under
 * AIT_INIT_VECTOR and calls its reset function with the AH object, the data of the entry and the new
 * IV. */
int AIT_RESET_IVAddInfo(void *unused, BsafeAlgorithmObj *obj, void *iv)
{
    AitInitVectorEntry *found;
    int status;
    if (!(obj->flags & 8))
        return BE_ALGORITHM_NOT_SET;
    status = B_InfoCacheFindInfo(&obj->cache, &found, AIT_INIT_VECTOR);
    if (status != 0)
        return status;
    return found->reInitIV(obj->sub, found->params, iv);
}
