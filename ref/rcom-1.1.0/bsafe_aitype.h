/*
 * The algorithm info types of BSAFE (B_AlgorithmInfoType): a pointer to a method table {AddInfo,
 * NewHandler, MakeInfo} (no NewHandler: 0), shared by the bsafe_ai_*.c units.
 */
#ifndef BSAFE_AITYPE_H
#define BSAFE_AITYPE_H

#include "aglobal.h"
#include "bsafe.h"

typedef struct B_AlgorithmInfoTypeVTable {
    int (*AddInfo)();
    int (*NewHandler)();
    int (*MakeInfo)();
} B_AlgorithmInfoTypeVTable;

typedef struct B_AlgorithmInfoType {
    B_AlgorithmInfoTypeVTable *vTable;
    void *pad_04[2];  /* unused, but part of the layout */
} B_AlgorithmInfoType;

/* An entry of the method tables GetInternalAttributes searches by name: the method name, its
 * type code and its algorithm info type. */
typedef struct AIAttrEntry {
    /* 0x00 */ const char *name;
    /* 0x04 */ int type;
    /* 0x08 */ B_AlgorithmInfoType *ait;
} AIAttrEntry; /* size = 0x0C */

/* The algorithm info types, by defining unit (bsafe_ai*.c). */
extern B_AlgorithmInfoType AIT_FeedbackCipher;
extern B_AlgorithmInfoType AIT_AES_ECB, AIT_AES128_ECB, AIT_AES192_ECB, AIT_AES256_ECB;
extern B_AlgorithmInfoType AIT_CBC;
extern B_AlgorithmInfoType AIT_CBC_INTER_LEAVED;
extern B_AlgorithmInfoType AIT_CBC_IV8;
extern B_AlgorithmInfoType AIT_CFB;
extern B_AlgorithmInfoType AIT_CFB_PIPELINED;
extern B_AlgorithmInfoType AIT_DES;
extern B_AlgorithmInfoType AIT_DES_EDE;
extern B_AlgorithmInfoType AIT_DESX;
extern B_AlgorithmInfoType AIT_ECAcceleratorTable;
extern B_AlgorithmInfoType AIT_ECB;
extern B_AlgorithmInfoType AIT_GenericCrypt;
extern B_AlgorithmInfoType AIT_OFB;
extern B_AlgorithmInfoType AIT_OFB_PIPELINED;
extern B_AlgorithmInfoType AIT_PKCS_RSAPublic;
extern B_AlgorithmInfoType AIT_RC2;
extern B_AlgorithmInfoType AIT_RC5;
extern B_AlgorithmInfoType AIT_RC5_64;
extern B_AlgorithmInfoType AIT_RC6;
extern unsigned char AIT_INIT_VECTOR[12];
extern B_AlgorithmInfoType AIT_RESET_IV;
extern B_AlgorithmInfoType AIT_RSAPublic;

extern int B_AlgorithmInfoTypeMakeError();
extern B_AlgorithmInfoTypeVTable AITChooseEncryptNull_V_TABLE;

/* Entry that the feedback modes (CBC, CFB) register under AIT_INIT_VECTOR: their parameters
 * and the function RESET_IV calls with a new IV (AHFeedbackCipherReInitIV). */
typedef struct AitInitVectorEntry {
    /* 0x00 */ void *params;
    /* 0x04 */ int (*reInitIV)();
} AitInitVectorEntry; /* size = 0x08 */

/* Info of AI_FeedbackCipher: the names of the cipher, the feedback mode and the padding, each with
 * its own parameters (the IV of the feedback mode is an ITEM). */
typedef struct {
    /* 0x00 */ unsigned char *encryptionMethodName;
    /* 0x04 */ void *encryptionParams;
    /* 0x08 */ unsigned char *feedbackMethodName;
    /* 0x0C */ void *feedbackParams;
    /* 0x10 */ unsigned char *paddingMethodName;
    /* 0x14 */ void *paddingParams;
} B_BLK_CIPHER_W_FEEDBACK_PARAMS; /* size = 0x18 */

/* bsafe_ai_aes.c */
int AIT_AESAddInfo(void *key, BsafeInfoCache *cache);

/* bsafe_ai_cbc.c */
int AIT_CBCAddInfo(void *key, BsafeInfoCache *cache, void **header);

/* bsafe_ai_cbc_iv8.c */
int AIT_CBC_IV8AddInfo(void *unused, BsafeAlgorithmObj *obj, void *iv);

/* bsafe_ai_cfb.c */
int AIT_CFBAddInfo(void *key, BsafeInfoCache *cache, unsigned int *header);

/* bsafe_ai_choose_null.c */
int AITChooseEncryptNewHandler(void *key, BsafeAlgorithmObj *algorithm);
int AITNullAddInfo(void *key, BsafeInfoCache *cache, void **header);

/* bsafe_ai_ecaccel.c */
int AIT_ECAcceleratorTableAddInfo(void *key, BsafeAlgorithmObj *obj, void **header);

/* bsafe_ai_gencrypt.c */
int AIT_GenericCryptAddInfo(void *key, BsafeInfoCache *cache, void **header);

/* bsafe_ai_pkcs_rsapub.c */
void *AIT_PKCS_RSAPublicNewHandler(void *unused, BsafeAlgorithmObj *algorithm);

/* bsafe_ai_rc2.c */
int AIT_RC2AddInfo(void *key, BsafeInfoCache *cache, unsigned int *header);

/* bsafe_ai_rc5.c */
int AIT_RC5AddInfo(void *key, BsafeInfoCache *cache, unsigned int *header);

/* bsafe_ai_rc5_64.c */
int AIT_RC5_64AddInfo(void *key, BsafeInfoCache *cache, unsigned int *header);

/* bsafe_ai_rc6.c */
int AIT_RC6AddInfo(void *key, BsafeInfoCache *cache, unsigned int *header);

/* bsafe_ai_reset_iv.c */
int AIT_RESET_IVAddInfo(void *unused, BsafeAlgorithmObj *obj, void *iv);

/* bsafe_attributes.c */
int GetInternalAttributes(AIAttrEntry **out, AIAttrEntry **table, unsigned char *name);

#endif
