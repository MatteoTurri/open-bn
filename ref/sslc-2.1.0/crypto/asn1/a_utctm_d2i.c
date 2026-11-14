/* crypto/asn1/a_utctm.c */
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
 */

#include <stdio.h>
#include <time.h>
#include "cryptlib.h"
#include "asn1.h"

#line 41 /* the line number is part of the binary */
ASN1_UTCTIME *d2i_ASN1_UTCTIME(a, pp, length)
ASN1_UTCTIME **a;
unsigned char **pp;
long length;
	{
	ASN1_UTCTIME *ret=NULL;

	ret=(ASN1_UTCTIME *)d2i_ASN1_bytes((ASN1_STRING **)a,pp,length,
		V_ASN1_UTCTIME,V_ASN1_UNIVERSAL);
	if (ret == NULL)
		{
		ASN1err(ASN1_F_D2I_ASN1_UTCTIME,ASN1_R_ERROR_STACK);
		return(NULL);
		}
	if (!ASN1_UTCTIME_check(ret))
		{
		ASN1err(ASN1_F_D2I_ASN1_UTCTIME,ASN1_R_INVALID_TIME_FORMAT);
		goto err;
		}

	return(ret);
err:
	if ((ret != NULL) && ((a == NULL) || (*a != ret)))
		ASN1_UTCTIME_free(ret);
	return(NULL);
	}
