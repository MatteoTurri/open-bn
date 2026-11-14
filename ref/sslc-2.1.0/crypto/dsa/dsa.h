/* crypto/dsa/dsa.h */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 * 
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from 
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

/*
 * The DSS routines are based on patches supplied by
 * Steven Schoch <schoch@sheba.arc.nasa.gov>.  He basically did the
 * work and I have just tweaked them a little to fit into my
 * stylistic vision for SSLeay :-) */

#ifndef HEADER_DSA_H
#define HEADER_DSA_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "bn.h"

typedef struct dsa_st
	{
	/* This first variable is used to pick up errors where
	 * a DSA is passed instead of of a EVP_PKEY */
	int pad;
	int version;
	int write_params;
	BIGNUM *p;
	BIGNUM *q;	/* == 20 */
	BIGNUM *g;

	BIGNUM *pub_key;  /* y public key */
	BIGNUM *priv_key; /* x private key */

	BIGNUM *kinv;	/* Signing pre-calc */
	BIGNUM *r;	/* Signing pre-calc */

	/* RCOM: the two fields of later OpenSSL (0.9.4): DSA_new_internal sets
	 * flags to 1, method_mont_p caches the Montgomery context of p. */
	int flags;
	char *method_mont_p;

	int references;
	} DSA;

/* RCOM/SSL-C: the DSA functions dispatch through this table, which
 * DSA_enable_default_method and the DSA_set_default_method_* hooks fill. */
typedef struct dsa_default_method_st {
    void *new_fn;
    void *free_fn;
    void *size_cb;
    void *setup_cb;
    void *sign_cb;
    void *verify_cb;
    void *p_dup_cb;
} DSA_DEFAULT_METHOD;

extern DSA_DEFAULT_METHOD default_dsa_method;

#define DSAparams_dup(x) (DSA *)ASN1_dup((int (*)())i2d_DSAparams, \
		(char *(*)())d2i_DSAparams,(char *)(x))
#define d2i_DSAparams_fp(fp,x) (DSA *)ASN1_d2i_fp((char *(*)())DSA_new, \
		(char *(*)())d2i_DSAparams,(fp),(unsigned char **)(x))
#define i2d_DSAparams_fp(fp,x) ASN1_i2d_fp(i2d_DSAparams,(fp), \
		(unsigned char *)(x))
#define d2i_DSAparams_bio(bp,x) (DSA *)ASN1_d2i_bio((char *(*)())DSA_new, \
		(char *(*)())d2i_DSAparams,(bp),(unsigned char **)(x))
#define i2d_DSAparams_bio(bp,x) ASN1_i2d_bio(i2d_DSAparams,(bp), \
		(unsigned char *)(x))

#ifndef NOPROTO

DSA *	DSA_new(void);
int	DSA_size(DSA *);
	/* next 4 return -1 on error */
int	DSA_sign_setup( DSA *dsa,BN_CTX *ctx_in,BIGNUM **kinvp,BIGNUM **rp);
int	DSA_sign(int type,unsigned char *dgst,int dlen,
		unsigned char *sig, unsigned int *siglen, DSA *dsa);
int	DSA_verify(int type,unsigned char *dgst,int dgst_len,
		unsigned char *sigbuf, int siglen, DSA *dsa);
void	DSA_free (DSA *r);
DSA *	DSA_params_dup(DSA *dsa);

/* RCOM/SSL-C: the default method and its callbacks. */
DSA *	DSA_new_internal(void);
void	DSA_free_internal(DSA *dsa);
int	DSA_enable_default_method(void);
void	DSA_set_default_method_size_cb(void *cb);
void	DSA_set_default_method_sign_cb(void *cb);
void	DSA_set_default_method_setup_cb(void *cb);
void	DSA_set_default_method_verify_cb(void *cb);
void	DSA_set_default_method_p_dup_cb(void *cb);
int	dsa_size_cb(DSA *dsa);
int	dsa_sign_setup_cb(DSA *dsa,BN_CTX *ctx_in,BIGNUM **kinvp,BIGNUM **rp);
int	dsa_sign_cb(int type,unsigned char *dgst,int dlen,
		unsigned char *sig, unsigned int *siglen, DSA *dsa);
int	dsa_verify_cb(int type,unsigned char *dgst,int dgst_len,
		unsigned char *sigbuf, int siglen, DSA *dsa);

void	ERR_load_DSA_strings(void );

DSA *	d2i_DSAPublicKey(DSA **a, unsigned char **pp, long length);
DSA *	d2i_DSAPrivateKey(DSA **a, unsigned char **pp, long length);
DSA * 	d2i_DSAparams(DSA **a, unsigned char **pp, long length);
DSA *	DSA_generate_parameters(int bits, unsigned char *seed,int seed_len,
		int *counter_ret, unsigned long *h_ret,void
		(*callback)(),char *cb_arg);
int	DSA_generate_key(DSA *a);
int	i2d_DSAPublicKey(DSA *a, unsigned char **pp);
int 	i2d_DSAPrivateKey(DSA *a, unsigned char **pp);
int	i2d_DSAparams(DSA *a,unsigned char **pp);

#ifdef HEADER_BIO_H
int	DSAparams_print(BIO *bp, DSA *x);
int	DSA_print(BIO *bp, DSA *x, int off);
#endif
#ifndef NO_FP_API
int	DSAparams_print_fp(FILE *fp, DSA *x);
int	DSA_print_fp(FILE *bp, DSA *x, int off);
#endif

int DSA_is_prime(BIGNUM *q,void (*callback)(),char *cb_arg);

#else

DSA *	DSA_new();
int	DSA_size();
int	DSA_sign_setup();
int	DSA_sign();
int	DSA_verify();
void	DSA_free ();
DSA *	DSA_params_dup();

DSA *	DSA_new_internal();
void	DSA_free_internal();
int	DSA_enable_default_method();
void	DSA_set_default_method_size_cb();
void	DSA_set_default_method_sign_cb();
void	DSA_set_default_method_setup_cb();
void	DSA_set_default_method_verify_cb();
void	DSA_set_default_method_p_dup_cb();
int	dsa_size_cb();
int	dsa_sign_setup_cb();
int	dsa_sign_cb();
int	dsa_verify_cb();

void	ERR_load_DSA_strings();

DSA *	d2i_DSAPublicKey();
DSA *	d2i_DSAPrivateKey();
DSA * 	d2i_DSAparams();
DSA *	DSA_generate_parameters();
int	DSA_generate_key();
int	i2d_DSAPublicKey();
int 	i2d_DSAPrivateKey();
int	i2d_DSAparams();

int	DSA_is_prime();

int	DSAparams_print();
int	DSA_print();

#ifndef NO_FP_API
int	DSAparams_print_fp();
int	DSA_print_fp();
#endif

#endif

/* BEGIN ERROR CODES */
/* Error codes for the DSA functions. */

/* Function codes. */
/* SSL-C: the function codes of the fork (102..113). */
#define DSA_F_DSAPARAMS_PRINT				 110
#define DSA_F_DSAPARAMS_PRINT_FP			 111
#define DSA_F_DSA_FREE					 103
#define DSA_F_DSA_IS_PRIME				 102
#define DSA_F_DSA_NEW					 104
#define DSA_F_DSA_PARAMS_DUP				 109
#define DSA_F_DSA_PRINT					 112
#define DSA_F_DSA_PRINT_FP				 113
#define DSA_F_DSA_SIGN					 105
#define DSA_F_DSA_SIGN_SETUP				 106
#define DSA_F_DSA_SIZE					 107
#define DSA_F_DSA_VERIFY				 108

/* Reason codes. */
#define DSA_R_DATA_TOO_LARGE_FOR_KEY_SIZE		 100
 
#ifdef  __cplusplus
}
#endif
#endif

