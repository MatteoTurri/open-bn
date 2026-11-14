/*
 * ssl_cert_free, from ssl/ssl_cert.c.
 */

/* ssl/ssl_cert.c */
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
#include "bio.h"
#include "pem.h"
#include "ssl_locl.h"

void ssl_cert_free(c)
CERT *c;
	{
	int i;

#line 79 /* the line number is part of the binary */
	i=CRYPTO_add(&c->references,-1,CRYPTO_LOCK_SSL_CERT);
#ifdef REF_PRINT
	REF_PRINT("CERT",c);
#endif
	if (i > 0) return;
#ifdef REF_CHECK
	if (i < 0)
		{
		fprintf(stderr,"ssl_cert_free, bad reference count\n");
		abort(); /* ok */
		}
#endif

#ifndef NO_RSA
	if (c->rsa_tmp) RSA_free(c->rsa_tmp);
	if (c->rsa_tmp_b) RSA_free(c->rsa_tmp_b);
	if (c->rsa_tmp_c) RSA_free(c->rsa_tmp_c);
#endif
#ifndef NO_DH
	if (c->dh_tmp) DH_free(c->dh_tmp);
	if (c->dh_tmp_b) DH_free(c->dh_tmp_b);
#endif

	for (i=0; i<SSL_PKEY_NUM; i++)
		{
		if (c->pkeys[i].x509 != NULL)
			SSLCERT_free(c->pkeys[i].x509);
		if (c->pkeys[i].privatekey != NULL)
			SSLCERT_PKEY_free(c->pkeys[i].privatekey);
#if 0
		if (c->pkeys[i].publickey != NULL)
			SSLCERT_PKEY_free(c->pkeys[i].publickey);
#endif
		}
	if (c->cert_chain != NULL)
		sk_pop_free(c->cert_chain,SSLCERT_free);
	Free(c);
	}

