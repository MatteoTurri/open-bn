/*
 * ASN1_UTCTIME_set, from crypto/asn1/a_utctm.c of SSLeay.
 */
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

ASN1_UTCTIME *ASN1_UTCTIME_set(s, t)
ASN1_UTCTIME *s;
void *t;
	{
	int i;
	/* SSL-C: t is an R_TIME_OBJ, formatted by R_time_export into a 17-byte buffer. */
	if (t == 0)
		goto err2;

	if (s == NULL)
		s=(ASN1_UTCTIME *)ASN1_STRING_type_new(V_ASN1_UTCTIME);
	if (s == NULL)
		goto err2;

	if ((s->data == NULL) || ((unsigned int)s->length < 17))
		{
		if (s->data != NULL)
			R_free(s->data);
		s->data=(unsigned char *)R_malloc(17);
		if (s->data == NULL)
			goto err2;
		}

	i=R_time_export((void *)t,1,s->data,14);
	if (i != 0)
		goto err;
	else
		s->type=V_ASN1_UTCTIME;

	s->length=strlen((char *)s->data);
	return(s);
err:
#line 197 /* the line number is part of the binary */
	ASN1err(224,123);
err2:
	return(NULL);
	}
