/*
 * The algorithm info type DES (AI_DES).
 */
#include "bsafe_aitype.h"

B_AlgorithmInfoType AIT_DES = { &AITChooseEncryptNull_V_TABLE };

int AI_DES(void **info)
{
    *info = &AIT_DES;
    return 0;
}
