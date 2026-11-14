/* crypto/evp/e_cbc_3d.c */
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

#include <stdio.h>
#include "cryptlib.h"
#include "evp.h"
#include "objects.h"

#ifndef NOPROTO
static void des_cbc_ede_init_key(EVP_CIPHER_CTX *ctx, unsigned char *key,
	unsigned char *iv,int enc);
static void des_cbc_ede3_init_key(EVP_CIPHER_CTX *ctx, unsigned char *key,
	unsigned char *iv,int enc);
static void des_cbc_ede_cipher(EVP_CIPHER_CTX *ctx, unsigned char *out,
	unsigned char *in, unsigned int inl);
#else
static void des_cbc_ede_init_key();
static void des_cbc_ede3_init_key();
static void des_cbc_ede_cipher();
#endif

/* SSL-C: the key schedule is in the cipher_data buffer of the EVP_CIPHER_CTX. */
typedef struct
	{
	des_key_schedule ks1;
	des_key_schedule ks2;
	des_key_schedule ks3;
	} DES_EDE_EX_KEY;

static EVP_CIPHER d_cbc_ede_cipher2=
	{
	NID_des_ede_cbc,
	8,16,8,
	des_cbc_ede_init_key,
	(int (*)())des_cbc_ede_cipher,
	NULL,
	sizeof(DES_EDE_EX_KEY),
	};

static EVP_CIPHER d_cbc_ede_cipher3=
	{
	NID_des_ede3_cbc,
	8,24,8,
	des_cbc_ede3_init_key,
	(int (*)())des_cbc_ede_cipher,
	NULL,
	sizeof(DES_EDE_EX_KEY),
	};

EVP_CIPHER *EVP_des_ede_cbc()
	{
	return(&d_cbc_ede_cipher2);
	}

EVP_CIPHER *EVP_des_ede3_cbc()
	{
	return(&d_cbc_ede_cipher3);
	}
	
static void des_cbc_ede_init_key(ctx,key,iv,enc)
EVP_CIPHER_CTX *ctx;
unsigned char *key;
unsigned char *iv;
int enc;
	{
	DES_EDE_EX_KEY *k=(DES_EDE_EX_KEY *)ctx->cipher_data;

	if (iv != NULL)
		memcpy(&(ctx->oiv[0]),iv,8);
	memcpy(&(ctx->iv[0]),&(ctx->oiv[0]),8);

	if (key != NULL)
		{
		des_set_key((des_cblock *)key,k->ks1);
		des_set_key((des_cblock *)&(key[8]),k->ks2);
		memcpy( (char *)k->ks3,
			(char *)k->ks1,
			sizeof(k->ks1));
		}
	}

static void des_cbc_ede3_init_key(ctx,key,iv,enc)
EVP_CIPHER_CTX *ctx;
unsigned char *key;
unsigned char *iv;
int enc;
	{
	DES_EDE_EX_KEY *k=(DES_EDE_EX_KEY *)ctx->cipher_data;

	if (iv != NULL)
		memcpy(&(ctx->oiv[0]),iv,8);
	memcpy(&(ctx->iv[0]),&(ctx->oiv[0]),8);

	if (key != NULL)
		{
		des_set_key((des_cblock *)key,k->ks1);
		des_set_key((des_cblock *)&(key[8]),k->ks2);
		des_set_key((des_cblock *)&(key[16]),k->ks3);
		}
	}

static void des_cbc_ede_cipher(ctx,out,in,inl)
EVP_CIPHER_CTX *ctx;
unsigned char *out;
unsigned char *in;
unsigned int inl;
	{
	DES_EDE_EX_KEY *k=(DES_EDE_EX_KEY *)ctx->cipher_data;

	des_ede3_cbc_encrypt(
		(des_cblock *)in,(des_cblock *)out,
		(long)inl, k->ks1,
		k->ks2,k->ks3,
		(des_cblock *)&(ctx->iv[0]),
		ctx->encrypt);
	}
