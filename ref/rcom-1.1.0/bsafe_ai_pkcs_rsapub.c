/*
 * The algorithm info type PKCS_RSAPublic (AI_PKCS_RSAPublic, AIT_PKCS_RSAPublicNewHandler).
 */
#include "bsafe.h"
#include "bsafe_aitype.h"
#include "ah.h"

static B_AlgorithmInfoTypeVTable V_TABLE = {
    AITNullAddInfo, (int (*)())AIT_PKCS_RSAPublicNewHandler, B_AlgorithmInfoTypeMakeError
};
B_AlgorithmInfoType AIT_PKCS_RSAPublic = { &V_TABLE };

int AI_PKCS_RSAPublic(void **info)
{
    *info = &AIT_PKCS_RSAPublic;
    return 0;
}

/* Builds an RSA public encryptor directly, without the info cache, and sets its initArg to 241. */
void *AIT_PKCS_RSAPublicNewHandler(void *unused, BsafeAlgorithmObj *algorithm)
{
    AH_RSAEncryption *obj = AH_RSAEncrypPublicConstructor(0, &algorithm->extError);
    if (obj == 0)
        return 0;
    obj->base.initArg = 241;
    return obj;
}
