/*
 * X509_STORE_add_crl (crypto/x509/x509_st.c of SSL-C).
 */
#include <stdio.h>
#include "cryptlib.h"
#include "lhash.h"
#include "x509.h"

int X509_STORE_add_crl(ctx,x)
X509_STORE *ctx;
X509_CRL *x;
	{
	X509_OBJECT *obj,*r;
	int ret=1;

	if (x == NULL) return(0);
	obj=(X509_OBJECT *)R_malloc(sizeof(X509_OBJECT));
	if (obj == NULL)
		{
#line 105 /* the line number is part of the binary */
		X509err(233,33);
		return(0);
		}

	R_lock_ctrl(9,11,ERR_file_name,113);

	X509_OBJECT_up_ref_count(obj);

	r=(X509_OBJECT *)lh_insert(ctx->certs,(char *)obj);
	if (r != NULL)
		{ /* oops, put it back */
		lh_delete(ctx->certs,(char *)obj);
		X509_OBJECT_free_contents(obj);
		R_free(obj);
		lh_insert(ctx->certs,(char *)r);
#line 124 /* the line number is part of the binary */
		X509err(233,155);
		ret=0;
		}

	R_lock_ctrl(10,11,ERR_file_name,128);

	return(ret);
	}
