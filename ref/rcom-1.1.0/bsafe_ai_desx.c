/*
 * The algorithm info type DESX (AI_DESX).
 */
#include "bsafe_aitype.h"

B_AlgorithmInfoType AIT_DESX = { &AITChooseEncryptNull_V_TABLE };

int AI_DESX(void **info)
{
    *info = &AIT_DESX;
    return 0;
}
