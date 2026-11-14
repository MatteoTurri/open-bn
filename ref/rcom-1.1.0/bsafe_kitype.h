/*
 * The key info types of BSAFE (B_KeyInfoType, the KIT_* objects): the two methods {AddInfo,
 * MakeInfo} in place, shared by kit_*.c and ki.c, and the key formats they convert.
 */
#ifndef BSAFE_KITYPE_H
#define BSAFE_KITYPE_H

#include "aglobal.h"

typedef struct B_KeyInfoType {
    int (*AddInfo)();
    int (*MakeInfo)();
} B_KeyInfoType;

/* An RSA key (KIT_RSAPublic, ALG_RSAInit): the modulus and one exponent. */
typedef struct {
    /* 0x00 */ ITEM modulus;
    /* 0x08 */ ITEM exponent;
} A_RSA_KEY; /* size = 0x10 */

/* A two-prime private key (BSAFE's A_PKCS_RSA_PRIVATE_KEY). */
typedef struct {
    ITEM modulus;            /* 0x00 */
    ITEM publicExponent;     /* 0x08 */
    ITEM privateExponent;    /* 0x10 */
    ITEM prime[2];           /* 0x18 */
    ITEM primeExponent[2];   /* 0x28 */
    ITEM coefficient;        /* 0x38 */
} KitRsaPrivateKey; /* size = 0x40 */

/* A private key with primeCount primes: primeCount primes and exponents, primeCount - 1 CRT
 * coefficients. */
typedef struct {
    ITEM n;              /* 0x00 */
    ITEM e;              /* 0x08 */
    ITEM d;              /* 0x10 */
    int primeCount;      /* 0x18 */
    ITEM *primes;        /* 0x1C */
    ITEM *exps;          /* 0x20 */
    ITEM *coeffs;        /* 0x24 */
} KitMpKey; /* size = 0x28 */

/* bsafe_keytype_error.c */
int B_KeyInfoTypeMakeError(void);

/* keyinfo_ber.c */
int DecodePublicKeyInfo(void *algorithmOid, void *algorithmParams, ITEM *out, ITEM *in);
int EncodePublicKeyInfoAlloc(void *out, void *algorithmOid, void *algorithmParams, ITEM *in);

/* ki.c */
int KITItemAddInfo(void *ctx, unsigned int *header);

/* kit_24byte.c */
int KIT_24ByteAddInfo(void *ctx, void *header);
int KIT_24ByteMakeInfo(unsigned char **out, void *key);

/* kit_8byte.c */
int KIT_8ByteAddInfo(void *ctx, void *header);
int KIT_8ByteMakeInfo(unsigned char **out, void *key);

/* kit_des8.c */
int KIT_DES8AddInfo(void *ctx, void *header);
int KIT_DES8MakeInfo(void **out, void *key);

/* kit_integer_items.c */
int AllocAndCopyIntegerItems(void *outBuf, void *header, ITEM *layoutBase, ITEM **items,
                             unsigned int count, void *ctx);

/* kit_misc.c */
int CheckDESParity(void *key);
void SetDESParity(void *key);

/* kit_pkcs_rsaprivate.c */
int KIT_PKCS_RSAMultiPrimeAddInfo(void *ctx, KitMpKey *header);
int KIT_PKCS_RSAMultiPrimeMakeInfo(void);
int KIT_PKCS_RSAPrivateAddInfo(void *ctx, KitRsaPrivateKey *header);
int KIT_PKCS_RSAPrivateMakeInfo(KitRsaPrivateKey **out, void *key);

/* kit_rsapublic.c */
int KIT_RSAPublicAddInfo(void *ctx, void *header);
int KIT_RSAPublicMakeInfo(void **out, void *key);

/* kit_rsapublic_ber.c */
int KIT_RSAPublicBERAddInfo(void *ctx, ITEM *in);
int KIT_RSAPublicBERMakeInfo(void **out, void *key);

/* The key info types, by defining unit (ki.c, kit_*.c). */
extern B_KeyInfoType KITItem;
extern B_KeyInfoType KIT_24Byte;
extern B_KeyInfoType KIT_8Byte;
extern B_KeyInfoType KIT_DES8;
extern B_KeyInfoType KIT_PKCS_RSAPrivate, KIT_PKCS_RSAMultiPrimePrivate;
extern B_KeyInfoType KIT_RSAPublic;
extern B_KeyInfoType KIT_RSAPublicBER;

#endif
