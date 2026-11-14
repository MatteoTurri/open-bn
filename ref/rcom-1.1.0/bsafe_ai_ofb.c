/*
 * The algorithm info type OFB (AI_OFB).
 */
#include "bsafe_aitype.h"

static B_AlgorithmInfoTypeVTable V_TABLE = {
    AIT_CBCAddInfo, 0, B_AlgorithmInfoTypeMakeError
};
B_AlgorithmInfoType AIT_OFB = { &V_TABLE };

int AI_OFB(void **info)
{
    *info = &AIT_OFB;
    return 0;
}
