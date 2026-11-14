/*
 * X509_STORE_get_by_subject, from crypto/x509/x509_lu.c.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "lhash.h"
#include "x509.h"

int X509_STORE_CTX_get_error_depth(ctx)
X509_STORE_CTX *ctx;
	{
	return(ctx->error_depth);
	}

X509 *X509_STORE_CTX_get_current_cert(ctx)
X509_STORE_CTX *ctx;
	{
	return(ctx->current_cert);
	}

int X509_STORE_get_by_subject(vs,type,name,ret)
X509_STORE_CTX *vs;
int type;
X509_NAME *name;
X509_OBJECT *ret;
	{
	X509_STORE *ctx=vs->ctx;
	X509_LOOKUP *lu;
	X509_OBJECT stmp,*tmp;
	int i,j;

	tmp=X509_OBJECT_retrieve_by_subject(ctx->certs,type,name);

	if (tmp == NULL)
		{
		for (i=vs->current_method; i<sk_num(ctx->get_cert_methods); i++)
			{
			lu=(X509_LOOKUP *)sk_value(ctx->get_cert_methods,i);
			j=X509_LOOKUP_by_subject(lu,type,name,&stmp);
			if (j < 0)
				{
				vs->current_method=j;
				return(j);
				}
			else if (j)
				{
				tmp= &stmp;
				break;
				}
			}
		vs->current_method=0;
		if (tmp == NULL)
			return(0);
		}

	ret->type=tmp->type;
	ret->data.ptr=tmp->data.ptr;

	X509_OBJECT_up_ref_count(ret);

	return(1);
	}
