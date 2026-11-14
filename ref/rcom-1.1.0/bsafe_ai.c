/*
 * AIT_FeedbackCipher: the algorithm info type of a block cipher in a feedback mode, with the tables
 * that name the ciphers, the feedback modes and the paddings.
 */
#include "bsafe_aitype.h"
#include "ah.h"
#include "bsafe.h"

typedef struct BsafeInfoCacheFwd BsafeInfoCacheFwd;


/* The parameters name the cipher, the feedback mode and the padding, each with its own parameters.
 * The AddInfo looks the names up in the attribute tables below; the cipher and the feedback mode add
 * their parameters through the AddInfo of their AIT, and the 40-byte entry goes into the cache under
 * key. Padding type 2 (stream) needs feedback type 6 or 7 (cfb, cfb_pipelined), otherwise BE_ALGORITHM_INFO. */

typedef struct FeedbackCipherInfo {
    B_BLK_CIPHER_W_FEEDBACK_PARAMS params;  /* 0, copied names; the cached cipher and feedback infos
                                               in place of their parameters, paddingParams unused */
    B_AlgorithmInfoType *encryptionAIT;     /* 24 */
    B_AlgorithmInfoType *feedbackAIT;       /* 28 */
    int paddingType;              /* 32 */
    int feedbackType;             /* 36 */
} FeedbackCipherInfo;



static AIAttrEntry AIE_aes = { "aes", 10, &AIT_AES_ECB };
static AIAttrEntry AIE_des = { "des", 1, &AIT_DES };
static AIAttrEntry AIE_desx = { "desx", 6, &AIT_DESX };
static AIAttrEntry AIE_des_ede = { "des_ede", 5, &AIT_DES_EDE };
static AIAttrEntry AIE_rc5 = { "rc5", 2, &AIT_RC5 };
static AIAttrEntry AIE_rc5_64 = { "rc5_64", 8, &AIT_RC5_64 };
static AIAttrEntry AIE_rc6 = { "rc6", 9, &AIT_RC6 };
static AIAttrEntry AIE_rc2 = { "rc2", 3, &AIT_RC2 };
static AIAttrEntry AIE_gencrypt = { "gencrypt", 7, &AIT_GenericCrypt };
static AIAttrEntry AIE_cbc = { "cbc", 1, &AIT_CBC };
static AIAttrEntry AIE_cbc_interleaved = { "cbc_interleaved", 2, &AIT_CBC_INTER_LEAVED };
static AIAttrEntry AIE_ecb = { "ecb", 3, &AIT_ECB };
static AIAttrEntry AIE_ofb = { "ofb", 4, &AIT_OFB };
static AIAttrEntry AIE_ofb_pipelined = { "ofb_pipelined", 5, &AIT_OFB_PIPELINED };
static AIAttrEntry AIE_cfb = { "cfb", 6, &AIT_CFB };
static AIAttrEntry AIE_cfb_pipelined = { "cfb_pipelined", 7, &AIT_CFB_PIPELINED };
static AIAttrEntry AIE_nopad = { "nopad", 0, 0 };
static AIAttrEntry AIE_pad = { "pad", 1, 0 };
static AIAttrEntry AIE_stream = { "stream", 2, 0 };

static AIAttrEntry *FEEDBACK_ENCRYPTION_TABLE[] = {
    &AIE_des, &AIE_des_ede, &AIE_desx, &AIE_rc5, &AIE_rc2, &AIE_rc5_64,
    &AIE_rc6, &AIE_aes, &AIE_gencrypt, 0
};
static AIAttrEntry *FEEDBACK_FEEDBACK_TABLE[] = {
    &AIE_cbc, &AIE_ecb, &AIE_cbc_interleaved, &AIE_cfb, &AIE_cfb_pipelined,
    &AIE_ofb, &AIE_ofb_pipelined, 0
};
static AIAttrEntry *FEEDBACK_PADDING_TABLE[] = {
    &AIE_nopad, &AIE_pad, &AIE_stream, 0
};

static int AITFeedbackCipherAddInfo();
static int AIFeedbackCipherNewHandler();

static B_AlgorithmInfoTypeVTable V_TABLE = {
    AITFeedbackCipherAddInfo, AIFeedbackCipherNewHandler, B_AlgorithmInfoTypeMakeError
};
B_AlgorithmInfoType AIT_FeedbackCipher = { &V_TABLE };

int AI_FeedbackCipher(void **info)
{
    *info = &AIT_FeedbackCipher;
    return 0;
}

static int AITFeedbackCipherAddInfo(void *key, BsafeAlgorithmObj *algorithm,
                                    B_BLK_CIPHER_W_FEEDBACK_PARAMS *params)
{
    AIAttrEntry *encryption;
    AIAttrEntry *feedback;
    AIAttrEntry *padding;
    FeedbackCipherInfo *info;
    int status;

    if ((status = GetInternalAttributes(&encryption, FEEDBACK_ENCRYPTION_TABLE,
                                        params->encryptionMethodName)) != 0)
        return status;
    if ((status = GetInternalAttributes(&feedback, FEEDBACK_FEEDBACK_TABLE,
                                        params->feedbackMethodName)) != 0)
        return status;
    if ((status = GetInternalAttributes(&padding, FEEDBACK_PADDING_TABLE,
                                        params->paddingMethodName)) != 0)
        return status;
    if (padding->type == 2 && feedback->type != 6 && feedback->type != 7)
        return BE_ALGORITHM_INFO;

    if ((status = B_MemoryPoolAlloc(&algorithm->cache.pool, (void **)&info, 40)) != 0)
        return status;
    info->paddingType = padding->type;

    if ((status = B_MemoryPoolAllocAndCopy
         (&algorithm->cache.pool, (void **)&info->params.encryptionMethodName, params->encryptionMethodName,
          T_strlen(params->encryptionMethodName) + 1)) != 0)
        return status;
    info->encryptionAIT = encryption->ait;
    if ((status = (*encryption->ait->vTable->AddInfo)
         (encryption->ait, algorithm, params->encryptionParams)) != 0)
        return status;
    if ((status = B_InfoCacheFindInfo(&algorithm->cache, &info->params.encryptionParams,
                                      encryption->ait)) != 0)
        return status;

    if ((status = B_MemoryPoolAllocAndCopy
         (&algorithm->cache.pool, (void **)&info->params.feedbackMethodName, params->feedbackMethodName,
          T_strlen(params->feedbackMethodName) + 1)) != 0)
        return status;
    info->feedbackType = feedback->type;
    info->feedbackAIT = feedback->ait;
    if ((status = (*feedback->ait->vTable->AddInfo)
         (feedback->ait, algorithm, params->feedbackParams)) != 0)
        return status;
    if ((status = B_InfoCacheFindInfo(&algorithm->cache, &info->params.feedbackParams,
                                      feedback->ait)) != 0)
        return status;

    if ((status = B_MemoryPoolAllocAndCopy
         (&algorithm->cache.pool, (void **)&info->params.paddingMethodName, params->paddingMethodName,
          T_strlen(params->paddingMethodName) + 1)) != 0)
        return status;

    algorithm->flags |= 8;
    return B_InfoCacheAddInfo(&algorithm->cache, key, info);
}

/* Builds the AHFeedbackCipher from the entry the AddInfo above stored under key. */
static int AIFeedbackCipherNewHandler(void *key, BsafeAlgorithmObj *algorithm)
{
    FeedbackCipherInfo *info;
    void *encryptionInfo, *encryptionAIT, *feedbackAIT, *feedbackInfo;

    if (B_InfoCacheFindInfo(&algorithm->cache, &info, key) != 0)
        return 0;
    encryptionInfo = info->params.encryptionParams;
    encryptionAIT = info->encryptionAIT;
    feedbackAIT = info->feedbackAIT;
    feedbackInfo = info->params.feedbackParams;
    return (int)AHFeedbackCipherConstructor(0, &algorithm->extError, encryptionAIT, encryptionInfo,
                                            feedbackAIT, feedbackInfo,
                                            info->paddingType,
                                            info->feedbackType);
}
