/*
 * The algorithm info type DES_EDE (AI_DES_EDE).
 */
#include "bsafe_aitype.h"

B_AlgorithmInfoType AIT_DES_EDE = { &AITChooseEncryptNull_V_TABLE };

int AI_DES_EDE(void **info)
{
    *info = &AIT_DES_EDE;
    return 0;
}
