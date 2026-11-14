/*
 * The algorithm info type CFB_PIPELINED (AI_CFB_PIPELINED).
 */
#include "bsafe_aitype.h"

static B_AlgorithmInfoTypeVTable V_TABLE = {
    AIT_CFBAddInfo, 0, B_AlgorithmInfoTypeMakeError
};
B_AlgorithmInfoType AIT_CFB_PIPELINED = { &V_TABLE };

int AI_CFB_PIPELINED(void **info)
{
    *info = &AIT_CFB_PIPELINED;
    return 0;
}
