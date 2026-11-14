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

/* ASN1err(ASN1_F_ASN1_UTCTIME_NEW,ASN1_R_UTCTIME_TOO_LONG);
 * ASN1err(ASN1_F_D2I_ASN1_UTCTIME,ASN1_R_EXPECTING_A_UTCTIME);
 */

int i2d_ASN1_UTCTIME(a,pp)
ASN1_UTCTIME *a;
unsigned char **pp;
	{
	return(i2d_ASN1_bytes((ASN1_STRING *)a,pp,
		V_ASN1_UTCTIME,V_ASN1_UNIVERSAL));
	}
