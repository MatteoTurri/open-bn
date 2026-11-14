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

/*
 * ssl_set_cert_masks and ssl_update_cache, from ssl/ssl_lib.c.
 */
#include <stdio.h>
#include "objects.h"
#include "lhash.h"
#include "ssl_locl.h"


/* SSL-C: struct ssl_rw_st (the two symmetric blocks of ssl_st) is in ssl.h: ssl3_change_cipher_state
 * uses it too. */

/* Used to change an SSL_CTXs default SSL method type */

void ssl_set_cert_masks(c)
CERT *c;
	{
	CERT_PKEY *cpk;
	int rsa_enc,rsa_tmp,rsa_sign,dh_tmp,dh_rsa,dh_dsa,dsa_sign;
	int rsa_enc_export,rsa_enc_export1024,dh_rsa_export,dh_dsa_export;
	int rsa_tmp_b,rsa_tmp_c,dh_tmp_export;
	unsigned long mask,emask;

	if ((c == NULL) || (c->valid)) return;

	rsa_tmp=((c->rsa_tmp != NULL) || (c->rsa_tmp_cb != NULL))?1:0;
	rsa_tmp_b=((c->rsa_tmp_b != NULL) || (c->rsa_tmp_cb != NULL))?1:0;
	rsa_tmp_c=((c->rsa_tmp_c != NULL) || (c->rsa_tmp_cb != NULL))?1:0;

	if (!SSL_feature_test(1))
		dh_tmp=dh_tmp_export=0;
	else
		{
		dh_tmp=((c->dh_tmp != NULL) || (c->dh_tmp_cb != NULL))?1:0;
		dh_tmp_export=((c->dh_tmp_b != NULL) || (c->dh_tmp_cb != NULL))?1:0;
		}

	cpk= &(c->pkeys[SSL_PKEY_RSA_ENC]);
	rsa_enc= ((cpk->x509 != NULL) && (cpk->privatekey != NULL))?1:0;
	rsa_enc_export=(rsa_enc &&
		(SSL_EVP_PKEY_size(cpk->privatekey)*8 <= 512))?1:0;
	rsa_enc_export1024=(rsa_enc &&
		(SSL_EVP_PKEY_size(cpk->privatekey)*8 <= 1024))?1:0;
	cpk= &(c->pkeys[SSL_PKEY_RSA_SIGN]);
	rsa_sign=((cpk->x509 != NULL) && (cpk->privatekey != NULL))?1:0;
	cpk= &(c->pkeys[SSL_PKEY_DSA_SIGN]);
	dsa_sign=((cpk->x509 != NULL) && (cpk->privatekey != NULL))?1:0;
	cpk= &(c->pkeys[SSL_PKEY_DH_RSA]);
	dh_rsa=  ((cpk->x509 != NULL) && (cpk->privatekey != NULL))?1:0;
	dh_rsa_export=(dh_rsa &&
		(SSL_EVP_PKEY_size(cpk->privatekey)*8 <= 512))?1:0;
	cpk= &(c->pkeys[SSL_PKEY_DH_DSA]);
	dh_dsa=  ((cpk->x509 != NULL) && (cpk->privatekey != NULL))?1:0;
	dh_dsa_export=(dh_dsa &&
		(SSL_EVP_PKEY_size(cpk->privatekey)*8 <= 512))?1:0;

	mask=0;
	emask=0;

	if (rsa_enc || (rsa_tmp && rsa_sign))
		mask|=SSL_kRSA;
	if (rsa_enc_export || rsa_enc_export1024 ||
		(rsa_tmp && (rsa_sign || rsa_enc)))
		emask|=SSL_kRSA;

	if (dh_tmp_export)
		emask|=SSL_kEDH;

	if (dh_tmp)
		mask|=SSL_kEDH;

	if (dh_rsa) mask|=SSL_kDHr;
	if (dh_rsa_export) emask|=SSL_kDHr;

	if (dh_dsa) mask|=SSL_kDHd;
	if (dh_dsa_export) emask|=SSL_kDHd;

	if (rsa_enc || rsa_sign)
		{
		mask|=SSL_aRSA;
		emask|=SSL_aRSA;
		}

	if (dsa_sign)
		{
		mask|=SSL_aDSS;
		emask|=SSL_aDSS;
		}

	if (SSL_feature_test(8))
		{
		mask|=SSL_aNULL;
		emask|=SSL_aNULL;
		}

	c->mask=mask;
	c->export_mask=emask;
	c->valid=1;
	}

void ssl_update_cache(s,mode)
SSL *s;
int mode;
	{
	int i;

	/* If the session_id_length is 0, we are not supposed to cache it,
	 * and it would be rather hard to do anyway :-) */
	if (s->session->session_id_length == 0) return;

	if ((s->ctx->session_cache_mode & mode)
		&& (!s->hit)
		&& SSL_CTX_add_session(s->ctx,s->session)
		&& (s->ctx->new_session_cb != NULL))
		{
		#line 1773 /* the line number is part of the binary */
		CRYPTO_add(&s->session->references,1,CRYPTO_LOCK_SSL_SESSION);
		if (!s->ctx->new_session_cb(s,s->session))
			SSL_SESSION_free(s->session);
		}

	/* auto flush every 255 connections */
	i=s->ctx->session_cache_mode;
	if ((!(i & SSL_SESS_CACHE_NO_AUTO_CLEAR)) &&
		((i & mode) == mode))
		{
		if (  (((mode & SSL_SESS_CACHE_CLIENT)
			?s->ctx->sess_connect_good
			:s->ctx->sess_accept_good) & 0xff) == 0xff)
			{
			SSL_CTX_flush_sessions(s->ctx,1);
			}
		}
	}
