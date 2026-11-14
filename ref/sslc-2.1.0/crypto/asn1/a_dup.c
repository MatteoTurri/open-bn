/* crypto/asn1/a_dup.c */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 * See crypto/README for full SSLeay licensing terms.
 */




#include <stdio.h>
#include "cryptlib.h"
#include "asn1_mac.h"

#define READ_CHUNK   2048

char *ASN1_dup(i2d,d2i,x)
int (*i2d)();
char *(*d2i)();
char *x;
	{
	unsigned char *b,*p;
	long i;
	char *ret;

	if (x == NULL) return(NULL);

	i=(long)i2d(x,NULL);
	b=(unsigned char *)Malloc((unsigned int)i+10);
	if (b == NULL)
		{ ASN1err(ASN1_F_ASN1_DUP,ERR_R_MALLOC_FAILURE); return(NULL); }
	p= b;
	i=i2d(x,&p);
	p= b;
	ret=d2i(NULL,&p,i);
	Free((char *)b);
	return(ret);
	}
