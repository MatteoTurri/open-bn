/*
 * The RSA implementation behind the RSA PK_METH (pk_rsa_sslc_*): its key and its contexts.
 */
#ifndef RCOM_PK_RSA_IMPL_H
#define RCOM_PK_RSA_IMPL_H

#include <stdio.h>
#include "bn.h"
#include "pk_int.h"

/* PK_CTX.impl of the RSA method. */
typedef struct RsaImpl {
    /* 0x00 */ int pad_00;  /* unused, but part of the layout */
    /* 0x04 */ BIGNUM n;  /* modulus */
    /* 0x18 */ BIGNUM e;  /* public exponent */
    /* 0x2C */ BIGNUM d;  /* private exponent */
    /* 0x40 */ int primeCount;  /* CRT factors */
    /* 0x44 */ BIGNUM *primes;  /* three per factor: the prime, its CRT exponent, its CRT coefficient */
    /* 0x48 */ void *rng;  /* first argument of the padding methods */
    /* 0x4C */ int surrender;  /* copied into BN_CTX.surrender: the operation context */
    /* 0x50 */ void **primeCtxs;  /* a BN_ME_CTX per factor */
    /* 0x54 */ void *modExpCtx;  /* BN_ME_CTX of the whole modulus */
    /* 0x58 */ RsaPadMeth *padMeth;  /* PK_METH.padding */
} RsaImpl; /* size = 0x5C */

/* pk_rsa_crt.c */
int pk_rsa_crt_sslc(BIGNUM *result, BIGNUM *base, RsaImpl *rsa);

/* pk_rsa_get.c */
int pk_rsa_get(PK_CTX *ctxArg, int type, int which, void *out);

/* pk_rsa_privdec.c */
int pk_rsa_private_decrypt_sslc(PK_CTX *ctxArg, int op, void *outBuf, int *outLenPtr, int bufCap,
                                void *in, int inLen, int padFlags);
int pk_rsa_public_encrypt_sslc(PK_CTX *ctxArg, int op, void *outBuf, int *outLenPtr, int bufCap,
                               void *in, int inLen, int padFlags);

/* pk_rsa_privenc.c */
int pk_rsa_private_encrypt_sslc(PK_CTX *ctxArg, int op, void *outBuf, int *outLenPtr, int bufCap,
                                void *in, int inLen, int padFlags);

/* pk_rsa_pubdec.c */
int pk_rsa_public_decrypt_sslc(PK_CTX *ctxArg, int op, void *outBuf, int *outLenPtr, int bufCap,
                               void *in, int inLen, int padFlags);

/* pk_rsa.c */
int pk_rsa_sslc(PK_CTX *ctxArg, int op, void *outBuf, int *outLenPtr, int bufCap, void *in, int inLen, int padFlags);

/* pk_rsa_cleanup.c */
int pk_rsa_sslc_cleanup(PK_CTX *ctxArg);

/* pk_rsa_init.c */
int pk_rsa_sslc_init(PK_CTX *ctxArg, void *eitems, int unused3, int unused4);

/* pk_rsa_verify.c */
int pk_rsa_verify(PK_CTX *ctxArg, int op, void *sigBytes, int *outMatchPtr, int bufCap,
                  unsigned char *hashBytes, int hashLen, int sigLen);

/* rsa_pk1_type2.c */
int rsa_padding_add_pkcs1_type_2(void *rng, unsigned char *to, int tlen, unsigned char *from,
                                 int flen, int flags);
int rsa_padding_check_pkcs1_type_2(unsigned char *to, int *outlen, unsigned char *from, int flen,
                                   int num, int flags);

#endif
