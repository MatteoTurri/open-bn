/*
 * The algorithm info type CBC_INTER_LEAVED (AI_CBC_INTER_LEAVED).
 */
#include "bsafe_aitype.h"

static B_AlgorithmInfoTypeVTable V_TABLE = {
    AIT_CBCAddInfo, 0, B_AlgorithmInfoTypeMakeError
};
B_AlgorithmInfoType AIT_CBC_INTER_LEAVED = { &V_TABLE };

int AI_CBC_INTER_LEAVED(void **info)
{
    *info = &AIT_CBC_INTER_LEAVED;
    return 0;
}
