/* crypto/asn1/asn1_lib.c */
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
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from 
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

/*
 * ASN1_get_object and asn1_get_length, from crypto/asn1/asn1_lib.c of SSLeay.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "asn1.h"
#include "asn1_mac.h"


#ifndef NOPROTO
static int asn1_get_length(unsigned char **pp,int *inf,long *rl,int max);
#else
static int asn1_get_length();
#endif

#line 73 /* the line number is part of the binary */
int ASN1_get_object(pp, plength, ptag, pclass, omax)
unsigned char **pp;
long *plength;
int *ptag;
int *pclass;
long omax;
	{
	int i,ret;
	long l;
	unsigned char *p= *pp;
	int tag,xclass,inf;
	long max=omax;

	if (!max) goto err;
	ret=(*p&V_ASN1_CONSTRUCTED);
	xclass=(*p&V_ASN1_PRIVATE);
	i= *p&V_ASN1_PRIMATIVE_TAG;
	if (i == V_ASN1_PRIMATIVE_TAG)
		{		/* high-tag */
		p++;
		if (--max == 0) goto err;
		l=0;
		while (*p&0x80)
			{
			l<<=7L;
			l|= *(p++)&0x7f;
			if (--max == 0) goto err;
			}
		l<<=7L;
		l|= *(p++)&0x7f;
		tag=(int)l;
		}
	else
		{ 
		tag=i;
		p++;
		if (--max == 0) goto err;
		}
	*ptag=tag;
	*pclass=xclass;
	if (!asn1_get_length(&p,&inf,plength,(int)max)) goto err;

#ifdef undef
	fprintf(stderr,"p=%d + *plength=%d > omax=%d + *pp=%d  (%d > %d)\n",
		p,*plength,omax,*pp,(p+ *plength),omax+ *pp);

#endif
	*pp=p;
	return(ret|inf);
err:
	ASN1err(ASN1_F_ASN1_GET_OBJECT,ASN1_R_HEADER_TOO_LONG);
	return(0x80);
	}

static int asn1_get_length(pp,inf,rl,max)
unsigned char **pp;
int *inf;
long *rl;
int max;
	{
	unsigned char *p= *pp;
	long ret=0;
	int i;

	if (max-- < 1) return(0);
	if (*p == 0x80)
		{
		*inf=1;
		ret=0;
		p++;
		}
	else
		{
		*inf=0;
		i= *p&0x7f;
		if (*(p++) & 0x80)
			{
			if (max-- == 0) return(0);
			while (i-- > 0)
				{
				ret<<=8L;
				ret|= *(p++);
				if (max-- == 0) return(0);
				}
			}
		else
			ret=i;
		}
	*pp=p;
	*rl=ret;
	return(1);
	}
