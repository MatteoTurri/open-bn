/* crypto/evp/p_lib.c */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
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
#include "asn1_mac.h"
#include "x509.h"

/* EVPerr(EVP_F_D2I_PKEY,EVP_R_UNSUPPORTED_CIPHER); */
/* EVPerr(EVP_F_D2I_PKEY,EVP_R_IV_TOO_LARGE); */

#ifndef NOPROTO
static void EVP_PKEY_free_it(EVP_PKEY *x);
#else
static void EVP_PKEY_free_it();
#endif

int EVP_PKEY_bits(pkey)
EVP_PKEY *pkey;
	{
#ifndef NO_RSA
	if (pkey->type == EVP_PKEY_RSA)
		return(RSA_size(pkey->pkey.rsa)*8);
	else
#endif
#ifndef NO_DSA
		if (pkey->type == EVP_PKEY_DSA)
		return(BN_num_bits(pkey->pkey.dsa->p));
#endif
	return(0);
	}

int EVP_PKEY_size(pkey)
EVP_PKEY *pkey;
	{
	if (pkey == NULL) return(0);
#ifndef NO_RSA
	if (pkey->type == EVP_PKEY_RSA)
		return(RSA_size(pkey->pkey.rsa));
	else
#endif
#ifndef NO_DSA
		if (pkey->type == EVP_PKEY_DSA)
		return(DSA_size(pkey->pkey.dsa));
#endif
	return(0);
	}

int EVP_PKEY_save_parameters(pkey,mode)
EVP_PKEY *pkey;
int mode;
	{
#ifndef NO_DSA
	if (pkey->type == EVP_PKEY_DSA)
		{
		int ret=pkey->save_parameters;

		if (mode >= 0)
			pkey->save_parameters=mode;
		return(ret);
		}
#endif
	return(0);
	}

int EVP_PKEY_copy_parameters(to,from)
EVP_PKEY *to,*from;
	{
	if (to->type != from->type)
		{
		EVPerr(EVP_F_EVP_PKEY_COPY_PARAMETERS,EVP_R_DIFFERENT_KEY_TYPES);
		goto err;
		}

	if (EVP_PKEY_missing_parameters(from))
		{
		EVPerr(EVP_F_EVP_PKEY_COPY_PARAMETERS,EVP_R_MISSING_PARMATERS);
		goto err;
		}
#ifndef NO_DSA
	if (to->type == EVP_PKEY_DSA)
		{
		BIGNUM *a;

		if ((a=BN_dup(from->pkey.dsa->p)) == NULL) goto err;
		if (to->pkey.dsa->p != NULL) BN_free(to->pkey.dsa->p);
		to->pkey.dsa->p=a;

		if ((a=BN_dup(from->pkey.dsa->q)) == NULL) goto err;
		if (to->pkey.dsa->q != NULL) BN_free(to->pkey.dsa->q);
		to->pkey.dsa->q=a;

		if ((a=BN_dup(from->pkey.dsa->g)) == NULL) goto err;
		if (to->pkey.dsa->g != NULL) BN_free(to->pkey.dsa->g);
		to->pkey.dsa->g=a;
		}
#endif
	return(1);
err:
	return(0);
	}

int EVP_PKEY_missing_parameters(pkey)
EVP_PKEY *pkey;
	{
#ifndef NO_DSA
	if (pkey->type == EVP_PKEY_DSA)
		{
		DSA *dsa;

		dsa=pkey->pkey.dsa;
		if ((dsa->p == NULL) || (dsa->q == NULL) || (dsa->g == NULL))
			return(1);
		}
#endif
	return(0);
	}





int EVP_PKEY_cmp_parameters(a,b)
EVP_PKEY *a,*b;
	{
#ifndef NO_DSA
	if ((a->type == EVP_PKEY_DSA) && (b->type == EVP_PKEY_DSA))
		{
		if (	BN_cmp(a->pkey.dsa->p,b->pkey.dsa->p) ||
			BN_cmp(a->pkey.dsa->q,b->pkey.dsa->q) ||
			BN_cmp(a->pkey.dsa->g,b->pkey.dsa->g))
			return(1);
		else
			return(0);
		}
#endif
	return(-1);
	}

EVP_PKEY *EVP_PKEY_new()
	{
	EVP_PKEY *ret;

	ret=(EVP_PKEY *)Malloc(sizeof(EVP_PKEY));
	if (ret == NULL)
		{
		EVPerr(EVP_F_EVP_PKEY_NEW,ERR_R_MALLOC_FAILURE);
		return(NULL);
		}
	ret->type=EVP_PKEY_NONE;
	ret->references=1;
	ret->pkey.ptr=NULL;
	ret->attributes=NULL;
	ret->save_parameters=1;
	return(ret);
	}

/* SSL-C: accessors, as in evp_lib.c. */
void EVP_PKEY_reference_inc(pkey)
EVP_PKEY *pkey;
	{
#line 174 /* the line number is part of the binary */
	CRYPTO_add(&pkey->references,1,CRYPTO_LOCK_EVP_PKEY);
	}

int EVP_PKEY_assign(pkey,type,key)
EVP_PKEY *pkey;
int type;
char *key;
	{
	if (pkey == NULL) return(0);
	if (pkey->pkey.ptr != NULL)
		EVP_PKEY_free_it(pkey);
	pkey->type=EVP_PKEY_type(type);
	pkey->save_type=type;
	pkey->pkey.ptr=key;
	return(1);
	}

int EVP_PKEY_key_type(pkey)
EVP_PKEY *pkey;
	{
	return(EVP_PKEY_type(pkey->type));
	}

int EVP_PKEY_get_num_primes(pkey)
EVP_PKEY *pkey;
	{
	int num;

	if (EVP_PKEY_type(pkey->type) != EVP_PKEY_RSA)
		return(0);
	RSA_get(pkey->pkey.rsa,1,NULL,&num);
	return(num);
	}

int EVP_PKEY_type(type)
int type;
	{
	switch (type)
		{
	case EVP_PKEY_RSA:
	case EVP_PKEY_RSA2:
		return(EVP_PKEY_RSA);
	case EVP_PKEY_DSA:
	case EVP_PKEY_DSA1:
	case EVP_PKEY_DSA2:
	case EVP_PKEY_DSA3:
	case EVP_PKEY_DSA4:
		return(EVP_PKEY_DSA);
	case EVP_PKEY_DH:
		return(EVP_PKEY_DH);
	default:
		return(NID_undef);
		}
	}

int EVP_PKEY_save_type(pkey)
EVP_PKEY *pkey;
	{
	return(pkey->save_type);
	}

void EVP_PKEY_free(x)
EVP_PKEY *x;
	{
	int i;

	if (x == NULL) return;

	i=CRYPTO_add(&x->references,-1,CRYPTO_LOCK_EVP_PKEY);
#ifdef REF_PRINT
	REF_PRINT("EVP_PKEY",x);
#endif
	if (i > 0) return;
#ifdef REF_CHECK
	if (i < 0)
		{
		fprintf(stderr,"EVP_PKEY_free, bad reference count\n");
		abort();
		}
#endif
	EVP_PKEY_free_it(x);
	Free((char *)x);
	}

static void EVP_PKEY_free_it(x)
EVP_PKEY *x;
	{
	switch (x->type)
		{
#ifndef NO_RSA
	case EVP_PKEY_RSA:
	case EVP_PKEY_RSA2:
		RSA_free(x->pkey.rsa);
		break;
#endif
#ifndef NO_DSA
	case EVP_PKEY_DSA:
	case EVP_PKEY_DSA2:
	case EVP_PKEY_DSA3:
	case EVP_PKEY_DSA4:
		DSA_free(x->pkey.dsa);
		break;
#endif
#ifndef NO_DH
	case EVP_PKEY_DH:
		DH_free(x->pkey.dh);
		break;
#endif
		}
	}

RSA *EVP_PKEY_get_RSA(pkey)
EVP_PKEY *pkey;
	{
	if (pkey != NULL)
		return(pkey->pkey.rsa);
	else
		return(NULL);
	}




























DSA *EVP_PKEY_get_DSA(pkey)
EVP_PKEY *pkey;
	{
	if (pkey != NULL)
		return(pkey->pkey.dsa);
	else
		return(NULL);
	}

DH *EVP_PKEY_get_DH(pkey)
EVP_PKEY *pkey;
	{
	if (pkey != NULL)
		return(pkey->pkey.dh);
	else
		return(NULL);
	}
