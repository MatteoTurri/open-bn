/* crypto/asn1/a_octet.c */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 * See crypto/README for full SSLeay licensing terms.
 */




#include <stdio.h>
#include "cryptlib.h"
#include "asn1.h"

/* ASN1err(ASN1_F_D2I_ASN1_OCTET_STRING,ASN1_R_EXPECTING_AN_OCTET_STRING);
 */

int i2d_ASN1_OCTET_STRING(a, pp)
ASN1_OCTET_STRING *a;
unsigned char **pp;
	{
	return(i2d_ASN1_bytes((ASN1_STRING *)a,pp,
		V_ASN1_OCTET_STRING,V_ASN1_UNIVERSAL));
	}

ASN1_OCTET_STRING *d2i_ASN1_OCTET_STRING(a, pp, length)
ASN1_OCTET_STRING **a;
unsigned char **pp;
long length;
	{
	ASN1_OCTET_STRING *ret=NULL;

	ret=(ASN1_OCTET_STRING *)d2i_ASN1_bytes((ASN1_STRING **)a,
		pp,length,V_ASN1_OCTET_STRING,V_ASN1_UNIVERSAL);
	if (ret == NULL)
		{
		ASN1err(ASN1_F_D2I_ASN1_OCTET_STRING,ASN1_R_ERROR_STACK);
		return(NULL);
		}
	return(ret);
	}

