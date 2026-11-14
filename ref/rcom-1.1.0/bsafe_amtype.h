/*
 * The algorithm methods of BSAFE (B_AlgorithmMethod, the AM_* objects): {algorithm info type, 1 to
 * encrypt and 0 to decrypt, key info type (0 for CBC and ECB), method table}, and the three kinds of
 * method table (alga) they point to.
 */
#ifndef BSAFE_AMTYPE_H
#define BSAFE_AMTYPE_H

#include "bsafe_aitype.h"
#include "bsafe_kitype.h"

typedef struct B_AlgorithmMethod {
    B_AlgorithmInfoType *algorithmInfoType;
    int encryptFlag;
    B_KeyInfoType *keyInfoType;
    void *alga;
} B_AlgorithmMethod;

/* A block cipher (DES, DES-EDE, RC6): CBC and ECB call Encrypt (+8) and Decrypt (+12) on one block;
 * the direction a method does not support has a function that returns 9. */
typedef struct A_BlockCipherAlga {
    int (*Query)();
    int (*Init)();
    int (*Encrypt)();
    int (*Decrypt)();
    int (*GetBlockLen)();
    int (*SetContextDestructor)();
    int cipherType;
} A_BlockCipherAlga;

/* A feedback mode (CBC, ECB): ResetIV returns 9 where there is no IV. */
typedef struct A_FeedbackAlga {
    int (*Query)();
    int (*Init)();
    int (*ResetIV)();
    int (*Update)();
    int (*Final)();
    int (*GetBlockLen)();
    int (*SetContextDestructor)();
} A_FeedbackAlga;

/* Public key encryption (PKCS RSA): the last method is missing (0). */
typedef struct A_PublicKeyAlga {
    int (*Query)();
    int (*Init)();
    int (*Update)();
    int (*Final)();
    int (*GetOutputLen)();
    int (*GetBlockLen)();
    int (*SetContextDestructor)();
    int (*Reserved)();
} A_PublicKeyAlga;

#endif
