/*
 * d2i_RSAPublicKey, d2i_RSAPrivateKey and i2d_RSAPublicKey of the RCOM fork: not the ASN.1 functions
 * of SSLeay but wrappers of PK_decode_rsa_*_key and PK_encode_rsa_public_key on the R_EITEMS list of
 * the key (rsa_int.h). i2d calls the encoder twice: for the size, then to write.
 */

#include <stdio.h>
#include "rsa.h"
#include "rsa_int.h"

#include "pk_int.h"

int i2d_RSAPublicKey(a,pp)
RSA *a;
unsigned char **pp;
	{
	int len;
	long ret= -1;

	if (PK_encode_rsa_public_key(a->eitems,0,&len,0) != 0)
		goto err;
	ret=len;
	if (pp == NULL)
		goto err;
	if (PK_encode_rsa_public_key(a->eitems,*pp,&len,len) != 0)
		{
		ret= -1;
		goto err;
		}
err:
	return((int)ret);
	}
