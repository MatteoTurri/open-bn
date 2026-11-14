/*
 * The algorithm info type RSAPublic (AI_RSAPublic).
 */
#include "bsafe_aitype.h"

B_AlgorithmInfoType AIT_RSAPublic = { &AITChooseEncryptNull_V_TABLE };

int AI_RSAPublic(void **info)
{
    *info = &AIT_RSAPublic;
    return 0;
}
