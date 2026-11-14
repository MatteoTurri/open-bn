/*
 * EVP_MD_CTX_new, EVP_MD_CTX_free and EVP_MD_CTX_copy (evp_md.c of SSL-C).
 */
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

EVP_MD_CTX *EVP_MD_CTX_new()
	{
	EVP_MD_CTX *ret;

	ret=NULL;
	if ((ret=(EVP_MD_CTX *)R_malloc(sizeof(EVP_MD_CTX))) == NULL)
		goto err;
	ret->digest=NULL;
	ret->md_data=NULL;
	ret->md_data_size=0;
	if (!EVP_MD_CTX_init(ret))
		goto err;
	return(ret);
err:
	if (ret != NULL)
		R_free(ret);
#line 101 /* the line number is part of the binary */
	EVPerr(EVP_F_EVP_MD_CTX_NEW,ERR_R_MALLOC_FAILURE);
	return(NULL);
	}

void EVP_MD_CTX_free(ctx)
EVP_MD_CTX *ctx;
	{
	if (ctx->md_data != NULL)
		{
		memset(ctx->md_data,0,ctx->md_data_size);
		R_free(ctx->md_data);
		}
	R_free(ctx);
	}

int EVP_MD_CTX_copy(out,in)
EVP_MD_CTX *out;
EVP_MD_CTX *in;
	{
	if (!EVP_MD_CTX_set_digest(out,in->digest))
		return(0);
	out->flags=in->flags;
	if (in->md_data != NULL)
		memcpy(out->md_data,in->md_data,in->md_data_size);
	return(1);
	}
