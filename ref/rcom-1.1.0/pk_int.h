/*
 * The PK layer of RCOM: the public key context (PK_CTX) and its method table (PK_METH).
 */
#ifndef PK_INT_H
#define PK_INT_H

#include "bsafe.h"

/* The padding methods of the RSA PK_METH (PK_METH.padding). */
typedef struct RsaPadMeth {
    /* 0x00 */ int (*signFn)(void *rng, void *outBuf, int outLen, void *hashBytes, int hashLen, int flags);
    /* 0x04 */ int (*unpadFn)(void *outBuf, int *outLenPtr, void *in, int inLen, int expectedLen, int flags);
    /* 0x08 */ int (*padFn)(void *rng, void *outBuf, int outLen, void *hashBytes, int hashLen, int flags);
    /* 0x0C */ int (*privUnpadFn)(void *outBuf, int *lenOut, void *in, int inLen, int expectedLen, int flags);
} RsaPadMeth; /* size = 0x10 */

/* The methods of a public key algorithm. Each algorithm defines them on its own context and
 * arguments, so the slots are unprototyped: initFn (ctx, a, b, c), destroyFn (ctx),
 * phase1Fn (ctx, op, a, b), opFn (ctx, op, the six arguments of op), finalFn (ctx, op, out, outLen),
 * getInfoFn (ctx, type, a, b). Unimplemented slots hold pk_unimplemented. */
typedef struct PK_METH {
    /* 0x00 */ int (*initFn)();
    /* 0x04 */ int (*destroyFn)();
    /* 0x08 */ int (*phase1Fn)();
    /* 0x0C */ int (*opFn)();
    /* 0x10 */ int (*finalFn)();
    /* 0x14 */ int (*unk_14)();  /* pk_unimplemented in every method */
    /* 0x18 */ int (*getInfoFn)();
    /* 0x1C */ RsaPadMeth *padding;  /* RSA only */
} PK_METH; /* size = 0x20 */

typedef struct PK_CTX {
    /* 0x00 */ PK_METH *methods;
    /* 0x04 */ void *eitems;
    /* 0x08 */ void *impl;  /* the RsaImpl or DhImpl of the method */
    /* 0x0C */ int flags;  /* RSA: 1 = load e by hand, 4 = multi-prime key */
} PK_CTX; /* size = 0x10 */

#include "r_com.h"

/* pk.c */
void PK_CTX_free(PK_CTX *ctxArg);
int PK_CTX_set(PK_CTX *ctxArg, PK_METH *methods, void *eitems, void *unused4, void *unused5);
int PK_phase1(PK_CTX *ctxArg, void *flags);
int PK_phase2(PK_CTX *ctxArg, unsigned char *out, int *outLen, unsigned int outMax, unsigned char *in,
              unsigned int inLen, void *unused);

/* pk_ctx_new.c */
void *PK_CTX_new(void);

/* pk_meth_dh.c */
PK_METH *PK_METH_dh(void);

/* pk_meth_rsa.c */
PK_METH *PK_METH_rsa_pkcs1_SSLv23(void);
int PK_encode_pkcs1_digest(void *outBuf, int *outLen, int maxOutLen, void *algOid,
                           void *digestData, int digestLen);

/* pk_rsa_key.c */
int PK_decode_rsa_private_key(void *eitems, unsigned char *data, int len, int *consumed);
int PK_decode_rsa_public_key(void *eitems, unsigned char *data, int len, int *consumed);
int PK_encode_rsa_public_key(void *eitems, unsigned char *buf, int *outLen, int bufLen);

/* pk_get_eitem.c */
int PK_get_eitem(PK_CTX *ctxArg, void *idx, void *buf, void *unused);

/* pk_get_info.c */
int PK_get_info(PK_CTX *ctxArg, int type, void *which, void *out);

/* pk_priv_enc.c */
int PK_priv_enc(PK_CTX *ctxArg, void *outBuf, void *outLenPtr, void *bufCap, void *in, void *inLen, void *padFlags);

/* pk_pub_dec.c */
int PK_pub_dec(PK_CTX *ctxArg, void *outBuf, void *outLenPtr, void *bufCap, void *in, void *inLen, void *padFlags);

/* pk_pub_enc.c */
int PK_pub_enc(PK_CTX *ctxArg, void *outBuf, void *outLenPtr, void *bufCap, void *in, void *inLen, void *padFlags);

/* pk_sign.c */
int PK_sign(PK_CTX *ctxArg, void *outBuf, void *outLenPtr, void *bufCap, void *in, void *inLen, void *padFlags);

/* pk_unimplemented.c */
int pk_unimplemented(void);

#endif
