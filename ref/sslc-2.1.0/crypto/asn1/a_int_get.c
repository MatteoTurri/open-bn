/*
 * ASN1_INTEGER_get, from crypto/asn1/a_int.c of SSLeay.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "asn1.h"

long ASN1_INTEGER_get(a)
ASN1_INTEGER *a;
	{
	int neg=0,i;
	long r=0;

	if (a == NULL) return(0L);
	i=a->type;
	if (i == V_ASN1_NEG_INTEGER)
		neg=1;
	else if (i != V_ASN1_INTEGER)
		return(0);
	
	if ((unsigned int)a->length > (unsigned int)sizeof(long))
		{
		/* hmm... a bit ugly */
		return(0xffffffffL);
		}
	if (a->data == NULL)
		return(0);

	for (i=0; (unsigned int)i<(unsigned int)a->length; i++)
		{
		r<<=8;
		r|=(unsigned char)a->data[i];
		}
	if (neg) r= -r;
	return(r);
	}
