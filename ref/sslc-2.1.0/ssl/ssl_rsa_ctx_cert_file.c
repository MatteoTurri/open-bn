/*
 * SSL_CTX_use_certificate_file, from ssl/ssl_rsa.c.
 */

/* ssl/ssl_rsa.c */
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
#include "bio.h"
#include "objects.h"
#include "evp.h"
#include "x509.h"
#include "pem.h"
#include "ssl_locl.h"



#ifndef NO_STDIO
#line 589 /* the line number is part of the binary */
int SSL_CTX_use_certificate_file(ctx, file, type)
SSL_CTX *ctx;
char *file;
int type;
	{
	int j;
	BIO *in=NULL;
	int ret=0;
	X509 *x=NULL;

	if (!SSLCERT_get_flag(1))
		{
		SSLerr(173,14);
		goto end;
		}

	in=BIO_new(BIO_s_file_internal());
	if (in == NULL)
		{
		SSLerr(173,ERR_R_BUF_LIB);
		goto end;
		}

	if (BIO_read_filename(in,file) <= 0)
		{
		SSLerr(173,ERR_R_SYS_LIB);
		goto end;
		}
	if (type == SSL_FILETYPE_ASN1)
		{
		j=ERR_R_ASN1_LIB;
		x=SSLCERT_from_binary_bio(in,NULL);
		}
	else if (type == SSL_FILETYPE_PEM)
		{
		j=ERR_R_PEM_LIB;
		x=SSLCERT_PEM_read_bio_SSLCERT(in,NULL,ctx->default_passwd_callback);
		}
	else
		{
		SSLerr(173,SSL_R_BAD_SSL_FILETYPE);
		goto end;
		}

	if (x == NULL)
		{
		SSLerr(173,j);
		goto end;
		}

	ret=SSL_CTX_use_certificate(ctx,x);
end:
	if (x != NULL) SSLCERT_free(x);
	if (in != NULL) BIO_free(in);
	return(ret);
	}
#endif

