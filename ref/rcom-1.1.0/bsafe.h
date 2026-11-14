/*
 * The core objects of BSAFE: memory pools, info caches, key and algorithm objects, and the AH object
 * an algorithm object dispatches to. Every function returns a status (0 = success) and its results
 * through out parameters.
 */
#ifndef BSAFE_H
#define BSAFE_H

#include "aglobal.h"

/* The BSAFE status codes (0 = success). */
#define BE_ALGORITHM_ALREADY_SET 0x0200
#define BE_ALGORITHM_INFO 0x0201
#define BE_ALGORITHM_NOT_INITIALIZED 0x0202
#define BE_ALGORITHM_NOT_SET 0x0203
#define BE_ALGORITHM_OBJ 0x0204
#define BE_ALG_OPERATION_UNKNOWN 0x0205
#define BE_ALLOC 0x0206
#define BE_CANCEL 0x0207
#define BE_DATA 0x0208
#define BE_EXPONENT_EVEN 0x0209
#define BE_EXPONENT_LEN 0x020a
#define BE_HARDWARE 0x020b
#define BE_INPUT_DATA 0x020c
#define BE_INPUT_LEN 0x020d
#define BE_KEY_ALREADY_SET 0x020e
#define BE_KEY_INFO 0x020f
#define BE_KEY_LEN 0x0210
#define BE_KEY_NOT_SET 0x0211
#define BE_KEY_OBJ 0x0212
#define BE_KEY_OPERATION_UNKNOWN 0x0213
#define BE_MEMORY_OBJ 0x0214
#define BE_MODULUS_LEN 0x0215
#define BE_NOT_INITIALIZED 0x0216
#define BE_NOT_SUPPORTED 0x0217
#define BE_OUTPUT_LEN 0x0218
#define BE_OVER_32K 0x0219
#define BE_RANDOM_NOT_INITIALIZED 0x021a
#define BE_RANDOM_OBJ 0x021b
#define BE_SIGNATURE 0x021c
#define BE_WRONG_ALGORITHM_INFO 0x021d
#define BE_WRONG_KEY_INFO 0x021e
#define BE_INPUT_COUNT 0x021f
#define BE_OUTPUT_COUNT 0x0220
#define BE_METHOD_NOT_IN_CHOOSER 0x0221
#define BE_KEY_WEAK 0x0222
#define BE_EXPONENT_ONE 0x0223

/* bsafe_t.c: the memory and string functions of the platform layer */
void T_memset(void *s, int c, unsigned int n);
void T_memcpy(void *dst, const void *src, unsigned int n);
void T_memmove(void *dst, const void *src, unsigned int n);
int T_memcmp(const void *a, const void *b, unsigned int n);
void *T_malloc(unsigned int size);
void *T_realloc(void *ptr, unsigned int size);
void T_free(void *p);
unsigned int T_strlen(const char *s);
char *T_strcpy(char *dst, const char *src);
int T_strcmp(const char *a, const char *b);

/* A memory pool: the blocks it owns, each with its destructor. */
typedef void (*BsafeDestructorFn)(void *);

typedef struct BsafePoolEntry {
    /* 0x00 */ void *dataPtr;
    /* 0x04 */ unsigned int size;
    /* 0x08 */ BsafeDestructorFn destructor;
} BsafePoolEntry; /* size = 0x0C */

typedef struct BsafeMemoryPool {
    /* 0x00 */ unsigned int count;
    /* 0x04 */ unsigned int capacity;
    /* 0x08 */ BsafePoolEntry *entries;
} BsafeMemoryPool; /* size = 0x0C */

/* An info cache: a memory pool and a {key, value} list searched linearly. */
typedef struct BsafeCacheItem {
    /* 0x00 */ void *key;
    /* 0x04 */ void *value;
} BsafeCacheItem; /* size = 0x08 */

typedef struct BsafeInfoCache {
    /* 0x00 */ BsafeMemoryPool pool;
    /* 0x0C */ unsigned int itemCount;
    /* 0x10 */ BsafeCacheItem *items;
} BsafeInfoCache; /* size = 0x14 */

/* The extended error of a key or algorithm object (B_GetExtendedErrorInfo). */
typedef struct BsafeExtendedError {
    /* 0x00 */ int errorCode;
    /* 0x04 */ int (*methodOverride)();  /* set during an Init to replace a method of the object */
    /* 0x08 */ BsafePoolEntry resizeCtx;
} BsafeExtendedError; /* size = 0x14 */

/* A key object: its info cache, its extended error and the self-check of KeyWrapCheck. */
typedef struct BsafeKeyObj {
    /* 0x00 */ BsafeInfoCache cache;  /* itemCount != 0 once the key has its info */
    /* 0x14 */ BsafeExtendedError extError;
    /* 0x28 */ void *tag;  /* KeyWrapCheck tag */
    /* 0x2C */ struct BsafeKeyObj *selfCheck;  /* == the object itself */
} BsafeKeyObj; /* size = 0x30 */

/* The method table of a cipher AH object; the random generators use BsafeRandomMethods, with the
 * same slot 0. B_AlgorithmCheckType checks the class before every call. Each class fills it with its
 * own methods, which take the class object: the slots are unprototyped. Arguments after the object:
 * getBlockLen (unsigned int *outLen); encrypt/decryptInit (key, surrender, random);
 * encrypt/decryptUpdate (output, outLen, maxOutLen, input, inputLen, randomAlg, surrender);
 * encrypt/decryptFinal (output, outLen, maxOutLen, randomAlg, surrender). */
typedef struct BsafeAlgMethods {
    /* 0x00 */ void (*destroy)();
    /* 0x04 */ int (*getBlockLen)();
    /* 0x08 */ int (*encryptInit)();
    /* 0x0C */ int (*decryptInit)();
    /* 0x10 */ int (*encryptUpdate)();
    /* 0x14 */ int (*decryptUpdate)();
    /* 0x18 */ int (*encryptFinal)();
    /* 0x1C */ int (*decryptFinal)();
} BsafeAlgMethods; /* size = 0x20 */

/* The method table of a random generator AH object. */
typedef struct BsafeRandomMethods {
    /* 0x00 */ void (*destroy)(void *sub);
    /* 0x04 */ int (*randomInit)(void *sub, void *chooser, void *surrender);
    /* 0x08 */ int (*randomUpdate)(void *sub, void *data, unsigned int len, void *surrender);
    /* 0x0C */ int (*generateBytes)(void *sub, void *data, unsigned int len, void *surrender);
} BsafeRandomMethods; /* size = 0x10 */

struct AlgaChoice;

/* The header of every AH object. `destroy` is also the class tag that B_AlgorithmCheckType
 * compares: a static forwarder to slot 0 of the methods (0 in the embedded choosers). */
typedef struct AHObject {
    /* 0x00 */ void (*destroy)(void *self);
    /* 0x04 */ struct BsafeExtendedError *extError;  /* of the algorithm object */
    /* 0x08 */ struct AlgaChoice *chosen;  /* set by Init: the algorithm the object chose */
} AHObject; /* size = 0x0C */

/* The AH object of an algorithm object: the header and the method table. */
typedef struct BsafeAlgSub {
    /* 0x00 */ AHObject base;
    /* 0x0C */ BsafeAlgMethods *methods;  /* a BsafeRandomMethods in the random generators */
} BsafeAlgSub; /* size = 0x10 */

/* An algorithm object: its info cache, its extended error, the AH object built by its info type and
 * the self-check of AlgorithmWrapCheck. */
typedef struct BsafeAlgorithmObj {
    /* 0x00 */ BsafeInfoCache cache;
    /* 0x14 */ BsafeExtendedError extError;
    /* 0x28 */ BsafeAlgSub *sub;
    /* 0x2C */ int flags;  /* 1 initialized; 2, 4, 8 allow the run-time info types */
    /* 0x30 */ void *tag;  /* AlgorithmWrapCheck tag */
    /* 0x34 */ struct BsafeAlgorithmObj *selfCheck;  /* == the object itself */
} BsafeAlgorithmObj; /* size = 0x38 */

typedef int (*BsafeInfoChooser)(void **);

void B_MemoryPoolConstructor(BsafeMemoryPool *pool);
void B_MemoryPoolDestructor(BsafeMemoryPool *pool);
void B_MemoryPoolReset(BsafeMemoryPool *pool);
int B_MemoryPoolAlloc(BsafeMemoryPool *pool, void **outPtr, unsigned int size);
int B_MemoryPoolAllocAndCopy(BsafeMemoryPool *pool, void **outPtr, const void *src, unsigned int size);
int B_MemoryPoolAdoptData(BsafeMemoryPool *pool, void **pData, unsigned int size);
int B_MemoryPoolAdoptHelper(BsafeMemoryPool *pool, void *data, unsigned int size, BsafeDestructorFn destructor);
int B_MemoryPoolRealloc(BsafeMemoryPool *pool, void **pData, unsigned int newSize);
void B_MemoryPoolFree(BsafeMemoryPool *pool, void **pData);
BsafePoolEntry *B_MemoryPoolFindAllocedObject(BsafeMemoryPool *pool, void *dataPtr);

void B_InfoCacheConstructor(BsafeInfoCache *cache);
int B_InfoCacheAddInfo(BsafeInfoCache *cache, void *key, void *value);
int B_InfoCacheFindInfo(BsafeInfoCache *cache, void *outValue, void *key);
void B_InfoCacheReset(BsafeInfoCache *cache);

void B_ExtendedErrorConstructor(BsafeExtendedError *ee);
void B_ExtendedErrorDestructor(BsafeExtendedError *ee);
void ResizeContextConstructor(BsafePoolEntry *ctx);
void ResizeContextDestructor(BsafePoolEntry *ctx);

int AlgorithmWrapCheck(BsafeAlgorithmObj *obj);
int KeyWrapCheck(BsafeKeyObj *key);
int RandomAlgorithmCheck(BsafeAlgorithmObj *randomAlg);

struct B_AlgorithmInfoType;
int B_AlgorithmSetInfo(BsafeAlgorithmObj *obj, struct B_AlgorithmInfoType *typeId, void *info);
int B_AlgorithmGetInfo(BsafeAlgorithmObj *obj, void **info, struct B_AlgorithmInfoType *typeId);
int B_AlgorithmCheckType(BsafeAlgorithmObj *obj, void *expectedTag);
int B_AlgorithmCheckTypeAndInitFlag(BsafeAlgorithmObj *obj, void *expectedTag);

int B_AlgorithmEncryptInit(BsafeAlgorithmObj *obj, BsafeKeyObj *key, void *chooser, BsafeAlgorithmObj *surrender);
int B_AlgorithmDecryptInit(BsafeAlgorithmObj *obj, BsafeKeyObj *key, void *chooser, BsafeAlgorithmObj *surrender);
int B_AlgorithmEncryptUpdate(BsafeAlgorithmObj *obj, void *output, unsigned int *outLen, unsigned int maxOutLen,
                              void *input, unsigned int inputLen, BsafeAlgorithmObj *randomAlg, void *surrender);
int B_AlgorithmDecryptUpdate(BsafeAlgorithmObj *obj, void *output, unsigned int *outLen, unsigned int maxOutLen,
                              void *input, unsigned int inputLen, BsafeAlgorithmObj *randomAlg, void *surrender);
int B_AlgorithmEncryptFinal(BsafeAlgorithmObj *obj, void *output, unsigned int *outLen, unsigned int maxOutLen,
                             BsafeAlgorithmObj *randomAlg, void *surrender);
int B_AlgorithmDecryptFinal(BsafeAlgorithmObj *obj, void *output, unsigned int *outLen, unsigned int maxOutLen,
                             BsafeAlgorithmObj *randomAlg, void *surrender);

struct HwAlgorithmMethod;
int SetDeviceName(BsafeAlgorithmObj *algorithm, struct B_AlgorithmInfoType *infoType, int encryptFlag,
                  void *alga, struct HwAlgorithmMethod **array);

struct B_KeyInfoType;
int B_KeySetInfo(BsafeKeyObj *key, struct B_KeyInfoType *infoType, void *info);
int B_KeyGetInfo(BsafeKeyObj *key, void **info, struct B_KeyInfoType *infoType);
int B_SetKeyInfo(BsafeKeyObj *key, BsafeInfoChooser chooser, void *info);
int B_GetKeyInfo(void *info, BsafeKeyObj *key, BsafeInfoChooser chooser);

void B_AlgorithmConstructor(BsafeAlgorithmObj *obj);
void B_AlgorithmDestructor(BsafeAlgorithmObj *obj);
int B_CreateAlgorithmObject(BsafeAlgorithmObj **outObj);
void B_DestroyAlgorithmObject(BsafeAlgorithmObj **pObj);
void B_GetExtendedErrorInfo(BsafeAlgorithmObj *obj, int *errorData, void **errorCode);
int B_GetAlgorithmInfo(void *info, BsafeAlgorithmObj *obj, BsafeInfoChooser infoType);

int B_AlgorithmRandomInit(BsafeAlgorithmObj *obj, void *chooser, void *surrender);
int B_AlgorithmRandomUpdate(BsafeAlgorithmObj *obj, void *data, unsigned int len, void *surrender);
int B_AlgorithmGenerateRandomBytes(BsafeAlgorithmObj *obj, void *data, unsigned int len, void *surrender);
int B_GetDevice(ITEM *outInfo, BsafeInfoCache *cache);

int B_CreateHardwareChooser(struct HwAlgorithmMethod **templateList, struct HwAlgorithmMethod ***pOut,
                            struct HwAlgorithmMethod **softwareList, int (*chooser)(void **), void *chooserArg);
void B_DestroyHardwareChooser(struct HwAlgorithmMethod ***pObj);

/* bsafe_alg.c */
int B_SetAlgorithmInfo(BsafeAlgorithmObj *obj, BsafeInfoChooser infoType, void *info);

/* bsafe_encdec.c */
int B_DecryptFinal(BsafeAlgorithmObj *obj, void *output, unsigned int *outLen,
                   unsigned int maxOutLen, BsafeAlgorithmObj *randomAlg, void *surrender);
int B_DecryptInit(BsafeAlgorithmObj *obj, BsafeKeyObj *key, void *chooser,
                  BsafeAlgorithmObj *surrender);
int B_DecryptUpdate(BsafeAlgorithmObj *obj, void *output, unsigned int *outLen,
                    unsigned int maxOutLen, void *input, unsigned int inputLen,
                    BsafeAlgorithmObj *randomAlg, void *surrender);
int B_EncryptFinal(BsafeAlgorithmObj *obj, void *output, unsigned int *outLen,
                   unsigned int maxOutLen, BsafeAlgorithmObj *randomAlg, void *surrender);
int B_EncryptInit(BsafeAlgorithmObj *obj, BsafeKeyObj *key, void *chooser,
                  BsafeAlgorithmObj *surrender);
int B_EncryptUpdate(BsafeAlgorithmObj *obj, void *output, unsigned int *outLen,
                    unsigned int maxOutLen, void *input, unsigned int inputLen,
                    BsafeAlgorithmObj *randomAlg, void *surrender);

/* bsafe_item_equal.c */
int B_ItemEqual(ITEM *a, ITEM *b);

/* bsafe_key.c */
int B_CreateKeyObject(BsafeKeyObj **outKey);
void B_DestroyKeyObject(BsafeKeyObj **pKey);

/* bsafe_keyinfo.c */
int B_KeyAddItemInfo(BsafeKeyObj *key, unsigned char *data, unsigned int len);

/* bsafe_resize.c */
int ResizeContextMakeNewContext(BsafePoolEntry *ctx, unsigned int newSize);

/* bsafe_ai*.c: the algorithm info types (B_SetAlgorithmInfo) */
int AI_AES128_ECB(void **info);
int AI_AES192_ECB(void **info);
int AI_AES256_ECB(void **info);
int AI_AES_ECB(void **info);
int AI_CBC(void **info);
int AI_CBC_INTER_LEAVED(void **info);
int AI_CBC_IV8(void **info);
int AI_CFB(void **info);
int AI_CFB_PIPELINED(void **info);
int AI_DES(void **info);
int AI_DES_EDE(void **info);
int AI_DESX(void **info);
int AI_ECAcceleratorTable(void **info);
int AI_ECB(void **info);
int AI_FeedbackCipher(void **info);
int AI_GenericCrypt(void **info);
int AI_OFB(void **info);
int AI_OFB_PIPELINED(void **info);
int AI_PKCS_RSAPublic(void **info);
int AI_RC2(void **info);
int AI_RC5(void **info);
int AI_RC5_64(void **info);
int AI_RC6(void **info);
int AI_RESET_IV(void **info);
int AI_RSAPublic(void **info);

/* kit_*.c, ki.c: the key info types (B_SetKeyInfo) */
int KI_24Byte(void **info);
int KI_8Byte(void **info);
int KI_DES8(void **info);
int KI_Item(void **info);
int KI_PKCS_RSAMultiPrimePrivate(void **info);
int KI_PKCS_RSAPrivate(void **info);
int KI_RSAPublic(void **info);
int KI_RSAPublicBER(void **info);

#endif
