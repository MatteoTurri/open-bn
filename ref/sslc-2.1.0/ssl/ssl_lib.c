/* ssl/ssl_lib.c */
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
#include "objects.h"
#include "lhash.h"
#include "ssl_locl.h"


/* SSL-C: struct ssl_rw_st (the two symmetric blocks of ssl_st) is in ssl.h: ssl3_change_cipher_state
 * uses it too. */



#line 234 /* the line number is part of the binary */
SSL *SSL_new(ctx)
SSL_CTX *ctx;
	{
	SSL *s;

	if (ctx == NULL)
		{
		SSLerr(186,195);
		return(NULL);
		}
	if (ctx->method == NULL)
		{
		SSLerr(186,228);
		return(NULL);
		}

	s=(SSL *)Malloc(sizeof(SSL));
	if (s == NULL) goto err;
	memset(s,0,sizeof(SSL));

	if (ctx->default_cert != NULL)
		{
		CRYPTO_add(&ctx->default_cert->references,1,
			CRYPTO_LOCK_SSL_CERT);
		s->cert=ctx->default_cert;
		}
	else
		s->cert=NULL;
	s->sid_ctx_length=ctx->sid_ctx_length;
	memcpy(s->sid_ctx,ctx->sid_ctx,32);
	s->verify_mode=ctx->default_verify_mode;
	s->verify_depth=ctx->default_verify_depth;
	s->verify_callback=ctx->default_verify_callback;

	CRYPTO_add(&ctx->references,1,CRYPTO_LOCK_SSL_CTX);
	s->ctx=ctx;

	s->verify_result=X509_V_OK;

	s->method=ctx->method;

	if (((s->md_ctx=EVP_MD_CTX_new()) == NULL) ||
		!s->method->ssl_new(s))
		{
		if (s->md_ctx != NULL)
			EVP_MD_CTX_free(s->md_ctx);
		SSL_CTX_free(ctx);
		Free(s);
		goto err;
		}

	s->references=1;
	s->new_session=(ctx->method->ssl_accept != ssl_undefined_function);
	s->options=ctx->options;
	s->rbuf_size=s->ctx->default_rbuf_size;
	s->wbuf_size=s->ctx->default_wbuf_size;
	s->extra_certs=NULL;
	ssl_clear_internal(s,1);

	CRYPTO_new_ex_data(ssl_meth,(char *)s,&s->ex_data);

	return(s);
err:
#line 303 /* the line number is part of the binary */
	SSLerr(186,33);
	return(NULL);
	}




#if defined(_WINDLL) && defined(WIN16)
#include "../crypto/bio/bss_file.c"
#endif

