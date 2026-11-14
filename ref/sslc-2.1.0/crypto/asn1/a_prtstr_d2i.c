/*
 * d2i_ASN1_PRINTABLESTRING, from crypto/asn1/a_print.c of SSLeay.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "asn1.h"

ASN1_PRINTABLESTRING *d2i_ASN1_PRINTABLESTRING(a,pp,l)
ASN1_PRINTABLESTRING **a;
unsigned char **pp;
long l;
	{ return(M_d2i_ASN1_PRINTABLESTRING(a,pp,l)); }
