/*
 * The algae layer of BSAFE: the RSA context (ALG_RSA*), the translation of
 * algorithm error codes and the helpers of the A_* layer.
 */

#ifndef RCOM_ALG_H
#define RCOM_ALG_H

#include "bsafe_kitype.h"
#include "cmp.h"

/* One modular operation of an algae context (a CMP_* function by default). */
typedef int (*AModOpFn)();

/* The context of an RSA operation (ALG_RSAInit): the key and the input of the current block. */
typedef struct ALG_RSAContext {
    /* 0x00 */ int bufSize;  /* bytes of the modulus: the block size */
    /* 0x04 */ void *buf;  /* input not processed yet */
    /* 0x08 */ int used;  /* bytes in buf */
    /* 0x0C */ CMPInt modulus;
    /* 0x18 */ CMPInt exponent;
    /* 0x24 */ AModOpFn modOps[7];  /* A_SetModularOperations; modOps[0] is the modular exponentiation */
} ALG_RSAContext; /* size = 0x40 */

/* a_misc.c */
unsigned int A_IntegerBits(unsigned char *value, unsigned int len);

/* a_modular_ops.c */
void A_SetModularOperations(AModOpFn *ops, AModOpFn modExp, AModOpFn modExpCRT,
                            AModOpFn modExpMultiPrimeCRT, AModOpFn modInvert,
                            AModOpFn modularReduce, AModOpFn computeGCD, AModOpFn modMultiply);

/* alg.c */
void ALG_RSAContextDestroy(struct ALG_RSAContext *ctx);
int ALG_RSAFinal(struct ALG_RSAContext *ctx);
int ALG_RSAInit(struct ALG_RSAContext *ctx, A_RSA_KEY *key);
int ALG_RSAUpdate(struct ALG_RSAContext *ctx, unsigned char *out, unsigned int *outlen, int outmax,
                  const unsigned char *in, unsigned int inlen, void *surrender);

/* alg_errcode.c */
int ALG_ErrorCode(int code);

#endif
