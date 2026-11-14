/* crypto/x509/x509_lu.c */
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
#include "lhash.h"
#include "x509.h"


static STACK *x509_store_meth=NULL;
































/* SSL-C: compares the key type with EVP_PKEY_key_type, and RSA keys with RSA_get and memcmp. */










static unsigned long x509_object_hash(a)
X509_OBJECT *a;
	{
	unsigned long h;

	/* SSL-C: no X509_LU_CRL case and no abort(). */



	switch (a->type)
		{
	case X509_LU_X509:
		h=X509_NAME_hash(X509_get_subject_name(a->data.x509));
		break;


	default:
		h=0;
		}
	return(h);
	}

static int x509_object_cmp(a,b)
X509_OBJECT *a,*b;
	{
	int ret;

	ret=(a->type - b->type);
	if (ret) return(ret);
	/* SSL-C: as above, only X509_LU_X509, default 0, no abort(). */

	switch (a->type)
		{
	case X509_LU_X509:
		ret=X509_subject_name_cmp(a->data.x509,b->data.x509);
		break;
	default:
		ret=0;
		break;
		}
	return(ret);
	}

X509_STORE *X509_STORE_new()
	{
	X509_STORE *ret;

	if ((ret=(X509_STORE *)Malloc(sizeof(X509_STORE))) == NULL)
		return(NULL);
	ret->certs=lh_new(x509_object_hash,x509_object_cmp);
	ret->cache=1;
	ret->get_cert_methods=sk_new_null();
	ret->verify=NULL;
	ret->verify_cb=NULL;
	memset(&ret->ex_data,0,sizeof(CRYPTO_EX_DATA));
	ret->references=1;
	CRYPTO_new_ex_data(x509_store_meth,(char *)ret,&(ret->ex_data));
	return(ret);
	}

static void cleanup(a)
X509_OBJECT *a;
	{
	/* SSL-C: no CRL branch. */
	if (a->type == X509_LU_X509)
		{
		X509_free(a->data.x509);
		}
	else
		{
		/* abort(); */
		}

	Free(a);
	}

void X509_STORE_free(vfy)
X509_STORE *vfy;
	{
	int i;
	STACK *sk;
	X509_LOOKUP *lu;

	sk=vfy->get_cert_methods;
	for (i=0; i<sk_num(sk); i++)
		{
		lu=(X509_LOOKUP *)sk_value(sk,i);
		X509_LOOKUP_shutdown(lu);
		X509_LOOKUP_free(lu);
		}
	sk_free(sk);

	CRYPTO_free_ex_data(x509_store_meth,(char *)vfy,&vfy->ex_data);
	lh_doall(vfy->certs,cleanup);
	lh_free(vfy->certs);
	Free(vfy);
	}

X509_LOOKUP *X509_STORE_add_lookup(v,m)
X509_STORE *v;
X509_LOOKUP_METHOD *m;
	{
	int i;
	STACK *sk;
	X509_LOOKUP *lu;

	sk=v->get_cert_methods;
	for (i=0; i<sk_num(sk); i++)
		{
		lu=(X509_LOOKUP *)sk_value(sk,i);
		if (m == lu->method)
			{
			return(lu);
			}
		}
	/* a new one */
	lu=X509_LOOKUP_new(m);
	if (lu == NULL)
		return(NULL);
	else
		{
		lu->store_ctx=v;
		if (sk_push(v->get_cert_methods,(char *)lu))
			return(lu);
		else
			{
			X509_LOOKUP_free(lu);
			return(NULL);
			}
		}
	}

/* SSL-C: only X509_LU_X509, freed with X509_free. */
void X509_OBJECT_free_contents(a)
X509_OBJECT *a;
	{
	switch (a->type)
		{
	case X509_LU_X509:
		X509_free(a->data.x509);
		break;
		}
	}

int X509_check_private_key(x,k)
X509 *x;
EVP_PKEY *k;
	{
	EVP_PKEY *xk=NULL;
	int ok=0;
	int xktype;
	unsigned char *na,*nb;
	int la,lb;

	xk=X509_get_pubkey(x);
	xktype=EVP_PKEY_key_type(xk);
	if (xktype != EVP_PKEY_key_type(k)) goto err;
	switch (xktype)
		{
	case EVP_PKEY_RSA:
		if (RSA_get(xk->pkey.rsa,16,&na,&la)) goto err;
		if (RSA_get(k->pkey.rsa,16,&nb,&lb)) goto err;
		if (la != lb || memcmp(na,nb,la) != 0) goto err;
		if (RSA_get(xk->pkey.rsa,17,&na,&la)) goto err;
		if (RSA_get(k->pkey.rsa,17,&nb,&lb)) goto err;
		if (la != lb || memcmp(na,nb,la) != 0) goto err;
		break;
	case EVP_PKEY_DSA:
		if (BN_cmp(xk->pkey.dsa->pub_key,k->pkey.dsa->pub_key) != 0)
			goto err;
		break;
	case EVP_PKEY_DH:
		goto err;
	default:
		goto err;
		}

	ok=1;
err:
	return(ok);
	}
