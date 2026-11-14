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
 * SSL_CTX_new and SSL_CTX_free, from ssl/ssl_lib.c.
 */
#include <stdio.h>
#include "objects.h"
#include "lhash.h"
#include "ssl_locl.h"


/* SSL-C: struct ssl_rw_st (the two symmetric blocks of ssl_st) is in ssl.h: ssl3_change_cipher_state
 * uses it too. */

/* Used to change an SSL_CTXs default SSL method type */


SSL_CTX *SSL_CTX_new(meth)
SSL_METHOD *meth;
	{
	SSL_CTX *ret=NULL;

	if (meth == NULL)
		{
#line 1276 /* the line number is part of the binary */
		SSLerr(168,196);
		return(NULL);
		}
	ret=(SSL_CTX *)Malloc(sizeof(SSL_CTX));
	if (ret == NULL)
		goto err;

	memset(ret,0,sizeof(SSL_CTX));

	ret->method=meth;

	ret->cert_store=NULL;
	ret->session_cache_mode=SSL_SESS_CACHE_SERVER;
	ret->session_cache_size=SSL_SESSION_CACHE_MAX_SIZE_DEFAULT;
	ret->session_cache_head=NULL;
	ret->session_cache_tail=NULL;

	/* We take the system default */
	ret->session_timeout=meth->get_timeout();

	ret->new_session_cb=NULL;
	ret->remove_session_cb=NULL;
	ret->get_session_cb=NULL;

	memset(&ret->sess_connect,0,44);

	ret->references=1;
	ret->info_callback=NULL;
	ret->ctx_alert_info_cb=NULL;
	ret->ctx_alert_info_cb_arg=NULL;
	ret->ctx_app_data_cb=NULL;
	ret->ctx_app_data_cb_arg=NULL;
	ret->verify_cb=0;
	ret->verify_cb_arg=0;

	ret->app_verify_callback=NULL;
	ret->app_verify_arg=NULL;

	ret->default_read_ahead=0;
	ret->default_verify_mode=SSL_VERIFY_NONE;
	ret->default_verify_depth= -1;
	ret->default_verify_callback=NULL;
	if ((ret->default_cert=ssl_cert_new()) == NULL)
		goto err;

	ret->default_passwd_callback=NULL;
	ret->client_cert_cb=NULL;

	ret->sessions=lh_new(SSL_SESSION_hash,SSL_SESSION_cmp);
	if (ret->sessions == NULL) goto err;
	ret->cert_store=SSLCERT_STORE_new();
	if ((ret->cert_store != NULL) &&
		(SSL_get_ex_data_X509_STORE_CTX_idx() < 0))
		{
#line 1357 /* the line number is part of the binary */
		SSLerr(168,269);
		goto err;
		}

	ssl_create_cipher_list(ret->method,
		&ret->cipher_list,&ret->cipher_list_by_id,
		SSL_DEFAULT_CIPHER_LIST);
	if ((ret->cipher_list == NULL) || (sk_num(ret->cipher_list) <= 0))
		{
#line 1369 /* the line number is part of the binary */
		SSLerr(168,161);
		goto err2;
		}

	if ((ret->rsa_md5=EVP_OBJ_get_digestbyname("ssl2-md5")) == NULL)
		{
		SSLerr(168,241);
		goto err2;
		}
	if ((ret->md5=EVP_OBJ_get_digestbyname("ssl3-md5")) == NULL)
		{
		SSLerr(168,242);
		goto err2;
		}
	if ((ret->sha1=EVP_OBJ_get_digestbyname("ssl3-sha1")) == NULL)
		{
		SSLerr(168,243);
		goto err2;
		}
	if (SSL_feature_test(2))
		{
		if ((ret->dss1=EVP_OBJ_get_digestbyname("DSA-SHA1")) == NULL)
			{
#line 1393 /* the line number is part of the binary */
			SSLerr(168,1102);
			goto err2;
			}
		}
	else
		ret->dss1=NULL;

	if ((ret->client_CA=sk_new_null()) == NULL)
		goto err;

	CRYPTO_new_ex_data(ssl_ctx_meth,(char *)ret,&ret->ex_data);

	ret->extra_certs=NULL;
	ret->default_rbuf_size=4096;
	ret->default_wbuf_size=16384;

	return(ret);
err:
#line 1430 /* the line number is part of the binary */
	SSLerr(168,33);
err2:
	if (ret != NULL) SSL_CTX_free(ret);
	return(NULL);
	}

void SSL_CTX_free(a)
SSL_CTX *a;
	{
	int i;

	if (a == NULL) return;

#line 1443 /* the line number is part of the binary */
	i=CRYPTO_add(&a->references,-1,CRYPTO_LOCK_SSL_CTX);
#ifdef REF_PRINT
	REF_PRINT("SSL_CTX",a);
#endif
	if (i > 0) return;
#ifdef REF_CHECK
	if (i < 0)
		{
		fprintf(stderr,"SSL_CTX_free, bad reference count\n");
		abort(); /* ok */
		}
#endif
	CRYPTO_free_ex_data(ssl_ctx_meth,(char *)a,&a->ex_data);

	if (a->sessions != NULL)
		{
		SSL_CTX_flush_sessions(a,0);
		lh_free(a->sessions);
		}
	if (a->cert_store != NULL)
		SSLCERT_STORE_free(a->cert_store);
	if (a->cipher_list != NULL)
		sk_free(a->cipher_list);
	if (a->cipher_list_by_id != NULL)
		sk_free(a->cipher_list_by_id);
	if (a->default_cert != NULL)
		ssl_cert_free(a->default_cert);
	if (a->client_CA != NULL)
		sk_pop_free(a->client_CA,SSLCERT_NAME_free);
	/* SSL-C: a field SSLeay 0.9.0b does not have: a STACK of
	 * certificates freed with sk_pop_free/SSLCERT_free. */
	if (a->extra_certs != NULL)
		sk_pop_free(a->extra_certs,SSLCERT_free);
	Free((char *)a);
	}
