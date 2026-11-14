/* crypto/evp/digest.c */
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
#include "objects.h"
#include "evp.h"

/* SSL-C: the EVP_MD_CTX has an allocated md_data buffer (ctx_size bytes) instead of the md union;
 * EVP_DigestInit returns an int and calls the optional init_ex hook of the EVP_MD before init. */
int EVP_DigestInit(ctx,type)
EVP_MD_CTX *ctx;
EVP_MD *type;
	{
	if (type == NULL)
		type=ctx->digest;
	else
		ctx->digest=type;

	if (type == NULL)
		{
#line 23 /* the line number is part of the binary */
		EVPerr(EVP_F_EVP_DIGESTINIT,EVP_R_NO_DIGEST_SPECIFIED);
		return(0);
		}

	if ((ctx->md_data == NULL) || (ctx->md_data_size < type->ctx_size))
		{
		if (ctx->md_data != NULL)
			R_free(ctx->md_data);
		if ((ctx->md_data=R_malloc(type->ctx_size)) == NULL)
			{
#line 31 /* the line number is part of the binary */
			EVPerr(EVP_F_EVP_DIGESTINIT,ERR_R_MALLOC_FAILURE);
			return(0);
			}
		memset(ctx->md_data,0,type->ctx_size);
		}
	ctx->md_data_size=type->ctx_size;
	if (type->init_ex != NULL)
		type->init_ex(ctx->md_data,type->init_ex_arg);
	type->init(ctx->md_data);
	return(1);
	}

void EVP_DigestUpdate(ctx,data,count)
EVP_MD_CTX *ctx;
unsigned char *data;
unsigned int count;
	{
	ctx->digest->update(ctx->md_data,data,count);
	}

void EVP_DigestFinal(ctx,md,size)
EVP_MD_CTX *ctx;
unsigned char *md;
unsigned int *size;
	{
	ctx->digest->final(md,ctx->md_data);
	if (size != NULL)
		*size=ctx->digest->md_size;
	memset(ctx->md_data,0,ctx->md_data_size);
	}


