/*
 * d2i_RSAPublicKey, d2i_RSAPrivateKey and i2d_RSAPublicKey of the RCOM fork: not the ASN.1 functions
 * of SSLeay but wrappers of PK_decode_rsa_*_key and PK_encode_rsa_public_key on the R_EITEMS list of
 * the key (rsa_int.h). i2d calls the encoder twice: for the size, then to write.
 */

#include <stdio.h>
#include "rsa.h"
#include "rsa_int.h"
#include "pk_int.h"
#include "r_eitem.h"
#include "r_eitems.h"

RSA *d2i_RSAPrivateKey(a,pp,length)
RSA **a;
unsigned char **pp;
long length;
	{
	int consumed;
	RSA *ret=NULL;

	if (a != NULL)
		ret=(*a);
	if (ret == NULL)
		{
		ret=RSA_new();
		if (ret == NULL) goto err;
		}

	if (PK_decode_rsa_private_key(ret->eitems,*pp,length,&consumed) != 0)
		{
		if ((a == NULL) || (*a != ret)) RSA_free(ret);
		ret=NULL;
		goto err;
		}

	if (R_EITEMS_compact(ret->eitems,0) != 0)
		{
		if ((a == NULL) || (*a != ret)) RSA_free(ret);
		ret=NULL;
		goto err;
		}
err:
	if ((ret != NULL) && (a != NULL)) (*a)=ret;
	return(ret);
	}
