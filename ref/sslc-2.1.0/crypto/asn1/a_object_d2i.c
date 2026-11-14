/*
 * d2i_ASN1_OBJECT, from crypto/asn1/a_object.c of SSLeay.
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

#include <stdio.h>
#include "cryptlib.h"
#include "buffer.h"
#include "asn1.h"
#include "objects.h"

ASN1_OBJECT *d2i_ASN1_OBJECT(a, pp, length)
ASN1_OBJECT **a;
unsigned char **pp;
long length; 
	{
	ASN1_OBJECT *ret=NULL;
	unsigned char *p;
	long len;
	int tag,xclass;
	int inf,i;

	/* only the ASN1_OBJECTs from the 'table' will have values
	 * for ->sn or ->ln */
	if ((a == NULL) || ((*a) == NULL) ||
		!((*a)->flags & ASN1_OBJECT_FLAG_DYNAMIC))
		{
		if ((ret=ASN1_OBJECT_new()) == NULL) return(NULL);
		}
	else	ret=(*a);

	p= *pp;

	inf=ASN1_get_object(&p,&len,&tag,&xclass,length);
	if (inf & 0x80)
		{
		i=ASN1_R_BAD_OBJECT_HEADER;
		goto err;
		}

	if (tag != V_ASN1_OBJECT)
		{
		i=ASN1_R_EXPECTING_AN_OBJECT;
		goto err;
		}
	if ((ret->data == NULL) || (ret->length < len))
		{
		if (ret->data != NULL) Free((char *)ret->data);
		ret->data=(unsigned char *)Malloc((int)len);
		ret->flags|=ASN1_OBJECT_FLAG_DYNAMIC_DATA;
		if (ret->data == NULL)
			{ i=ERR_R_MALLOC_FAILURE; goto err; }
		}
	memcpy(ret->data,p,(int)len);
	ret->length=(int)len;
	ret->sn=NULL;
	ret->ln=NULL;
	/* ret->flags=ASN1_OBJECT_FLAG_DYNAMIC; we know it is dynamic */
	p+=len;

	if (a != NULL) (*a)=ret;
	*pp=p;
	return(ret);
err:
#line 364 /* the line number is part of the binary */
	ASN1err(ASN1_F_D2I_ASN1_OBJECT,i);
	if ((ret != NULL) && ((a == NULL) || (*a != ret)))
		ASN1_OBJECT_free(ret);
	return(NULL);
	}
