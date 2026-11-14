/*
 * The RSA object of the RCOM fork of SSL-C (RSA_new/RSA_free and the d2i/i2d wrappers): the key is
 * an R_EITEMS list, the operations go through a PK_CTX. It is the body of the opaque `struct rsa_st`
 * of the SSL-C rsa.h.
 */
#ifndef RCOM_RSA_INT_H
#define RCOM_RSA_INT_H

typedef struct rsa_extra_st { int unk_00, unk_04, unk_08; } RSA_EXTRA;  /* copied field by field by RSA_dup */

struct rsa_st {
    /* 0x00 */ int pad;
    /* 0x04 */ void *eitems;  /* R_EITEMS: the components of the key */
    /* 0x08 */ void *meth;
    /* 0x0C */ void *pk_ctx;
    /* 0x10 */ int references;
    /* 0x14 */ int flags;
    /* 0x18 */ RSA_EXTRA *extra_buf;
}; /* size = 0x1C */

/* rsa_pk.c: the PK_CTX of the key, created on first use */
void *rsa_get_pk_ctx(struct rsa_st *rsa);

#endif
