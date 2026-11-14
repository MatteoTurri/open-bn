/*
 * ASN1_UTCTIME_check and its range tables, from crypto/asn1/a_utctm.c of SSLeay.
 */
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

int ASN1_UTCTIME_check(d)
ASN1_UTCTIME *d;
	{
	static int min[8]={ 0, 1, 1, 0, 0, 0, 0, 0};
	static int max[8]={99,12,31,23,59,59,12,59};
	char *a;
	int n,i,l,o;

	if (d->type != V_ASN1_UTCTIME) return(0);
	l=d->length;
	a=(char *)d->data;
	o=0;

	if (l < 11) goto err;
	for (i=0; i<6; i++)
		{
		if ((i == 5) && ((a[o] == 'Z') ||
			(a[o] == '+') || (a[o] == '-')))
			{ i++; break; }
		if ((a[o] < '0') || (a[o] > '9')) goto err;
		n= a[o]-'0';
		if (++o > l) goto err;

		if ((a[o] < '0') || (a[o] > '9')) goto err;
		n=(n*10)+ a[o]-'0';
		if (++o > l) goto err;

		if ((n < min[i]) || (n > max[i])) goto err;
		}
	if (a[o] == 'Z')
		o++;
	else if ((a[o] == '+') || (a[o] == '-'))
		{
		o++;
		if (o+4 > l) goto err;
		for (i=6; i<8; i++)
			{
			if ((a[o] < '0') || (a[o] > '9')) goto err;
			n= a[o]-'0';
			o++;
			if ((a[o] < '0') || (a[o] > '9')) goto err;
			n=(n*10)+ a[o]-'0';
			if ((n < min[i]) || (n > max[i])) goto err;
			o++;
			}
		}
	return(o == l);
err:
	return(0);
	}
