#ifndef RCOM_PK_DH_IMPL_H
#define RCOM_PK_DH_IMPL_H

/*
 * The DH implementation behind the DH PK_METH (pk_dh_sslc_*): its context, installed in PK_CTX.impl.
 */

#include <stdio.h>
#include "bn.h"
#include "pk_int.h"

#include "r_com.h"
extern int pk_dh_sslc_cleanup(PK_CTX *ctxArg);

/* PK_CTX.impl of the DH method. */
typedef struct DhImpl {
    /* 0x00 */ int pad_00;  /* unused, but part of the layout */
    /* 0x04 */ int flags;  /* 1 initialized, 2 keys generated, 4 private key present */
    /* 0x08 */ int privBits;  /* bits of the private key that generate_key draws */
    /* 0x0C */ BIGNUM p;
    /* 0x20 */ BIGNUM g;
    /* 0x34 */ BIGNUM pubKey;
    /* 0x48 */ BIGNUM privKey;
    /* 0x5C */ void *rng;  /* random object for BN_rand */
    /* 0x60 */ void *surrender;
    /* 0x64 */ void *modExpCtx;
} DhImpl; /* size = 0x68 */

#include <string.h>

#include "r_eitems.h"

#include "bn_me.h"


/* pk_dh_compute.c */
int pk_dh_sslc_compute_key(PK_CTX *ctxArg, int op, void *outBuf, int *outLenPtr, unsigned int bufCap,
                           unsigned char *peerBytes, int peerLen);
int pk_dh_sslc_final(PK_CTX *ctxArg, int op, void *outBuf, int *outLenPtr);
int pk_dh_sslc_get(PK_CTX *ctxArg, int type, void *which, void *out);

/* pk_dh_generate.c */
int pk_dh_sslc_generate_key(PK_CTX *ctxArg, int op, void *unusedA, int flags);

/* pk_dh.c */
int pk_dh_sslc_init(PK_CTX *ctxArg, void *eitems, int unused3, int unused4);

#endif
