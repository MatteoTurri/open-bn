/*
 * X509_STORE_add_cert (crypto/x509/x509_st.c of SSL-C).
 */
#include <stdio.h>
#include "cryptlib.h"
#include "lhash.h"
#include "x509.h"

int X509_STORE_add_cert(ctx,x)
X509_STORE *ctx;
X509 *x;
	{
	X509_OBJECT *obj,*r;
	int ret=1;

	if (x == NULL) return(0);
	obj=(X509_OBJECT *)R_malloc(sizeof(X509_OBJECT));
	if (obj == NULL)
		{
#line 66 /* the line number is part of the binary */
		X509err(232,33);
		return(0);
		}
	obj->type=X509_LU_X509;
	obj->data.x509=x;

	R_lock_ctrl(9,11,ERR_file_name,72);

	X509_OBJECT_up_ref_count(obj);

	r=(X509_OBJECT *)lh_insert(ctx->certs,(char *)obj);
	if (r != NULL)
		{ /* oops, put it back */
		lh_delete(ctx->certs,(char *)obj);
		X509_OBJECT_free_contents(obj);
		R_free(obj);
		lh_insert(ctx->certs,(char *)r);
		X509err(232,155);
		ret=0;
		}

	R_lock_ctrl(10,11,ERR_file_name,87);

	return(ret);
	}
