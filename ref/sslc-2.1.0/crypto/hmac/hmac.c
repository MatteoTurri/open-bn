/* crypto/hmac/hmac.c */
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
#include <stdlib.h>
#include <string.h>
#include "hmac.h"
#include "err.h"

/* SSL-C: HMAC_new, HMAC_free, HMAC_digest and HMAC_copy; md_ctx, i_ctx and o_ctx are allocated, and
 * HMAC_Init and HMAC_Final return an int. The allocator is R_malloc/R_free. */

HMAC_CTX *HMAC_new()
	{
	HMAC_CTX *ret;

	if ((ret=(HMAC_CTX *)R_malloc(sizeof(HMAC_CTX))) == NULL)
		return(NULL);
	memset(ret,0,sizeof(HMAC_CTX));

	if ((ret->md_ctx=EVP_MD_CTX_new()) == NULL)
		goto err;
	if ((ret->i_ctx=EVP_MD_CTX_new()) == NULL)
		goto err;
	if ((ret->o_ctx=EVP_MD_CTX_new()) == NULL)
		goto err;

	return(ret);
err:
	if (ret->md_ctx != NULL)
		EVP_MD_CTX_free(ret->md_ctx);
	if (ret->i_ctx != NULL)
		EVP_MD_CTX_free(ret->i_ctx);
	if (ret->o_ctx != NULL)
		EVP_MD_CTX_free(ret->o_ctx);
#line 38 /* the line number is part of the binary */
	HMACerr(HMAC_F_HMAC_NEW,ERR_R_MALLOC_FAILURE);
	return(NULL);
	}

void HMAC_free(ctx)
HMAC_CTX *ctx;
	{
	if (ctx->md_ctx != NULL)
		EVP_MD_CTX_free(ctx->md_ctx);
	if (ctx->i_ctx != NULL)
		EVP_MD_CTX_free(ctx->i_ctx);
	if (ctx->o_ctx != NULL)
		EVP_MD_CTX_free(ctx->o_ctx);
	memset(ctx,0,sizeof(HMAC_CTX));
	R_free(ctx);
	}

EVP_MD *HMAC_digest(ctx)
HMAC_CTX *ctx;
	{
	return(ctx->md);
	}

int HMAC_copy(dst,src)
HMAC_CTX *dst;
HMAC_CTX *src;
	{
	dst->md=src->md;
	if (EVP_MD_CTX_copy(dst->md_ctx,src->md_ctx) == 0 ||
	    EVP_MD_CTX_copy(dst->i_ctx,src->i_ctx) == 0 ||
	    EVP_MD_CTX_copy(dst->o_ctx,src->o_ctx) == 0)
		return 0;
	dst->reserved=src->reserved;
	dst->key_length=src->key_length;
	memcpy(dst->key,src->key,HMAC_MAX_MD_CBLOCK);
	return 1;
	}

int HMAC_Init(ctx,key,len,md)
HMAC_CTX *ctx;
unsigned char *key;
int len;
EVP_MD *md;
	{
	int i,j,reset=0;
	unsigned char pad[HMAC_MAX_MD_CBLOCK];

	if (md != NULL)
		{
		reset=1;
		ctx->md=md;
		}
	else
		md=ctx->md;

	if (key != NULL)
		{
		reset=1;
		j=EVP_MD_block_size(md);
		if (j < len)
			{
			if (!EVP_DigestInit(ctx->md_ctx,md))
				return 0;
			EVP_DigestUpdate(ctx->md_ctx,key,len);
			EVP_DigestFinal(ctx->md_ctx,ctx->key,
				&ctx->key_length);
			}
		else
			{
			memcpy(ctx->key,key,len);
			memset(&(ctx->key[len]),0,sizeof(ctx->key)-len);
			ctx->key_length=len;
			}
		}

	if (reset)
		{
		for (i=0; i<HMAC_MAX_MD_CBLOCK; i++)
			pad[i]=0x36^ctx->key[i];
		if (!EVP_DigestInit(ctx->i_ctx,md))
			return 0;
		EVP_DigestUpdate(ctx->i_ctx,pad,EVP_MD_block_size(md));

		for (i=0; i<HMAC_MAX_MD_CBLOCK; i++)
			pad[i]=0x5c^ctx->key[i];
		EVP_DigestInit(ctx->o_ctx,md);
		EVP_DigestUpdate(ctx->o_ctx,pad,EVP_MD_block_size(md));
		}

	if (!EVP_MD_CTX_copy(ctx->md_ctx,ctx->i_ctx))
		return 0;
	return 1;
	}

void HMAC_Update(ctx,data,len)
HMAC_CTX *ctx;
unsigned char *data;
int len;
	{
	EVP_DigestUpdate(ctx->md_ctx,data,len);
	}

int HMAC_Final(ctx,md,len)
HMAC_CTX *ctx;
unsigned char *md;
unsigned int *len;
	{
	int j;
	unsigned int i;
	unsigned char buf[EVP_MAX_MD_SIZE];

	j=EVP_MD_block_size(ctx->md);

	EVP_DigestFinal(ctx->md_ctx,buf,&i);
	if (!EVP_MD_CTX_copy(ctx->md_ctx,ctx->o_ctx))
		return 0;
	EVP_DigestUpdate(ctx->md_ctx,buf,i);
	EVP_DigestFinal(ctx->md_ctx,md,len);
	return 1;
	}

void HMAC_cleanup(ctx)
HMAC_CTX *ctx;
	{
	EVP_MD_CTX_init(ctx->md_ctx);
	EVP_MD_CTX_init(ctx->i_ctx);
	EVP_MD_CTX_init(ctx->o_ctx);
	memset(ctx->key,0,HMAC_MAX_MD_CBLOCK);
	}

/* SSL-C: HMAC() with an allocated HMAC_CTX. */
unsigned char *HMAC(evp_md,key,key_len,d,n,md,md_len)
EVP_MD *evp_md;
unsigned char *key;
int key_len;
unsigned char *d;
int n;
unsigned char *md;
unsigned int *md_len;
	{
	HMAC_CTX *ret=NULL;
	unsigned char *result=NULL;
	static unsigned char m[EVP_MAX_MD_SIZE];

	if (md == NULL) md=m;
	ret=HMAC_new();
	if (!HMAC_Init(ret,key,key_len,evp_md))
		goto end;
	HMAC_Update(ret,d,n);
	if (!HMAC_Final(ret,md,md_len))
		goto end;
	result=md;
end:
	if (ret != NULL)
		HMAC_free(ret);
	return(result);
	}

