/*
 * EVP_CIPHER_CTX_new and EVP_CipherInit, from crypto/evp/evp_enc.c.
 */
/* crypto/evp/evp_enc.c */
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

/* SSL-C: the EVP_CIPHER_CTX has an allocated cipher_data buffer instead of the union c, which
 * init, new, free and cleanup manage. */

#include <stdio.h>
#include "cryptlib.h"
#include "evp.h"

EVP_CIPHER_CTX *EVP_CIPHER_CTX_new()
	{
	EVP_CIPHER_CTX *ctx;

	ctx=NULL;
	ctx=(EVP_CIPHER_CTX *)R_malloc(sizeof(EVP_CIPHER_CTX));
	if (ctx == NULL)
		{
		ctx=NULL;
		goto err;
		}
	ctx->cipher=NULL;
	ctx->cipher_data=NULL;
	ctx->cipher_data_size=0;
	ctx->flags_unk7c=0;
	ctx->key_len=0;
	ctx->ex_long=0;
	if (!EVP_CIPHER_CTX_init(ctx))
		goto err;
	return(ctx);
err:
	if (ctx != NULL)
		R_free(ctx);
#line 173 /* the line number is part of the binary */
	EVPerr(EVP_F_EVP_CIPHER_CTX_NEW,ERR_R_MALLOC_FAILURE);
	return(NULL);
	}

int EVP_CipherInit(ctx,cipher,key,iv,enc)
EVP_CIPHER_CTX *ctx;
EVP_CIPHER *cipher;
unsigned char *key;
unsigned char *iv;
int enc;
	{
	int ret;

	if (enc < 0)
		enc=ctx->encrypt;
	if (enc)
		ret=EVP_EncryptInit(ctx,cipher,key,iv);
	else
		ret=EVP_DecryptInit(ctx,cipher,key,iv);
	return(ret);
	}
