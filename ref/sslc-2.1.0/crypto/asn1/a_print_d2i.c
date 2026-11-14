/*
 * d2i_ASN1_PRINTABLE, from crypto/asn1/a_print.c of SSLeay.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "asn1.h"

ASN1_STRING *d2i_ASN1_PRINTABLE(a,pp,l)
ASN1_STRING **a;
unsigned char **pp;
long l;
	{ return(M_d2i_ASN1_PRINTABLE(a,pp,l)); }
