/*
 * The algorithm info type CBC_IV8 (AI_CBC_IV8, AIT_CBC_IV8AddInfo).
 */
#include "bsafe.h"
#include "bsafe_aitype.h"

static B_AlgorithmInfoTypeVTable V_TABLE = {
    AIT_CBC_IV8AddInfo, 0, B_AlgorithmInfoTypeMakeError
};
B_AlgorithmInfoType AIT_CBC_IV8 = { &V_TABLE };

int AI_CBC_IV8(void **info)
{
    *info = &AIT_CBC_IV8;
    return 0;
}

/* Needs flag 2 of the algorithm object; copies the 8-byte IV into the AH object at +52 and a pointer
 * back to the algorithm object at +48. */
int AIT_CBC_IV8AddInfo(void *unused, BsafeAlgorithmObj *obj, void *iv)
{
    if (!(obj->flags & 2))
        return BE_ALGORITHM_NOT_SET;
    T_memcpy((char *)obj->sub + 52, iv, 8);
    *(BsafeAlgorithmObj **)((char *)obj->sub + 48) = obj;
    return 0;
}
