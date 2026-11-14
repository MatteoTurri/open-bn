/*
 * i2d_ASN1_PRINTABLE, from crypto/asn1/a_print.c of SSLeay.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "asn1.h"

int i2d_ASN1_PRINTABLE(a,pp)
ASN1_STRING *a;
unsigned char **pp;
	{ return(M_i2d_ASN1_PRINTABLE(a,pp)); }
