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
 * ASN1_object_size, asn1_Finish, asn1_GetSequence and ASN1_STRING_set, from crypto/asn1/asn1_lib.c
 * of SSLeay.
 */





#include <stdio.h>
#include "cryptlib.h"
#include "asn1.h"
#include "asn1_mac.h"

/* SSL-C: Realloc is R_realloc, which takes (ptr, new size, old size). */







#undef Realloc
#define Realloc(a,n) R_realloc((char *)(a),(n),0)

int ASN1_object_size(constructed, length, tag)
int constructed;
int length;
int tag;
	{
	int ret;

	ret=length;
	ret++;
	if (tag >= 31)
		{
		while (tag > 0)
			{
			tag>>=7;
			ret++;
			}
		}
	if ((length == 0) && (constructed == 2))
		ret+=2;
	ret++;
	if (length > 127)
		{
		while (length > 0)
			{
			length>>=8;
			ret++;
			}
		}
	return(ret);
	}

int asn1_Finish(c)
ASN1_CTX *c;
	{
	if ((c->inf == (1|V_ASN1_CONSTRUCTED)) && (!c->eos))
		{
		if (!ASN1_check_infinite_end(&c->p,c->slen))
			{
			c->error=ASN1_R_MISSING_EOS;
			return(0);
			}
		}
	if (	((c->slen != 0) && !(c->inf & 1)) ||
		((c->slen < 0) && (c->inf & 1)))
		{
		c->error=ASN1_R_LENGTH_MISMATCH;
		return(0);
		}
	return(1);
	}

int asn1_GetSequence(c,length)
ASN1_CTX *c;
long *length;
	{
	unsigned char *q;

	q=c->p;
	c->inf=ASN1_get_object(&(c->p),&(c->slen),&(c->tag),&(c->xclass),
		*length);
	if (c->inf & 0x80)
		{
		c->error=ASN1_R_BAD_GET_OBJECT;
		return(0);
		}
	if (c->tag != V_ASN1_SEQUENCE)
		{
		c->error=ASN1_R_EXPECTING_A_SEQUENCE;
		return(0);
		}
	(*length)-=(c->p-q);
	if (c->max && (*length < 0))
		{
		c->error=ASN1_R_LENGTH_MISMATCH;
		return(0);
		}
	if (c->inf == (1|V_ASN1_CONSTRUCTED))
		c->slen= *length+ *(c->pp)-c->p;
	c->eos=0;
	return(1);
	}

int ASN1_STRING_set(str,data,len)
ASN1_STRING *str;
unsigned char *data;
int len;
	{
	char *c;

	if (len < 0)
		{
		if (data == NULL)
			return(0);
		else
			len=strlen((char *)data);
		}
	if ((str->length < len) || (str->data == NULL))
		{
		c=(char *)str->data;
		if (c == NULL)
			str->data=(unsigned char *)Malloc(len+1);
		else
			str->data=(unsigned char *)Realloc(c,len+1);

		if (str->data == NULL)
			{
			str->data=(unsigned char *)c;
			return(0);
			}
		}
	str->length=len;
	if (data != NULL)
		{
		memcpy(str->data,data,len);
		/* an alowance for strings :-) */
		str->data[len]='\0';
		}
	str->flags=0;
	return(1);
	}

