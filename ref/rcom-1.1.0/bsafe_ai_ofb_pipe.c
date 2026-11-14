/*
 * The algorithm info type OFB_PIPELINED (AI_OFB_PIPELINED).
 */
#include "bsafe_aitype.h"

static B_AlgorithmInfoTypeVTable V_TABLE = {
    AIT_CBCAddInfo, 0, B_AlgorithmInfoTypeMakeError
};
B_AlgorithmInfoType AIT_OFB_PIPELINED = { &V_TABLE };

int AI_OFB_PIPELINED(void **info)
{
    *info = &AIT_OFB_PIPELINED;
    return 0;
}
