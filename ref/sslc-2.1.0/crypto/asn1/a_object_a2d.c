/*
 * a2d_ASN1_OBJECT, from crypto/asn1/a_object.c of SSLeay.
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

#line 56 /* the line number is part of the binary */
int a2d_ASN1_OBJECT(out,olen,buf,num)
unsigned char *out;
int olen;
char *buf;
int num;
	{
	int i,first,len=0,c;
	char tmp[24],*p;
	unsigned long l;

	if (num == 0)
		return(0);
	else if (num == -1)
		num=strlen(buf);

	p=buf;
	c= *(p++);
	num--;
	if ((c >= '0') && (c <= '2'))
		{
		first=(c-'0')*40;
		}
	else
		{
		ASN1err(ASN1_F_A2D_ASN1_OBJECT,ASN1_R_FIRST_NUM_TOO_LARGE);
		goto err;
		}

	if (num <= 0)
		{
		ASN1err(ASN1_F_A2D_ASN1_OBJECT,ASN1_R_MISSING_SECOND_NUMBER);
		goto err;
		}
	c= *(p++);
	num--;
	for (;;)
		{
		if (num <= 0) break;
		if ((c != '.') && (c != ' '))
			{
			ASN1err(ASN1_F_A2D_ASN1_OBJECT,ASN1_R_INVALID_SEPARATOR);
			goto err;
			}
		l=0;
		for (;;)
			{
			if (num <= 0) break;
			num--;
			c= *(p++);
			if ((c == ' ') || (c == '.'))
				break;
			if ((c < '0') || (c > '9'))
				{
				ASN1err(ASN1_F_A2D_ASN1_OBJECT,ASN1_R_INVALID_DIGIT);
				goto err;
				}
			l=l*10L+(long)(c-'0');
			}
		if (len == 0)
			{
			if ((first < 2) && (l >= 40))
				{
				ASN1err(ASN1_F_A2D_ASN1_OBJECT,ASN1_R_SECOND_NUMBER_TOO_LARGE);
				goto err;
				}
			l+=(long)first;
			}
		i=0;
		for (;;)
			{
			tmp[i++]=(unsigned char)l&0x7f;
			l>>=7L;
			if (l == 0L) break;
			}
		if (out != NULL)
			{
			if (len+i > olen)
				{
				/* SSL-C: the reason code "buffer too small" is 16. */
#line 134 /* the line number is part of the binary */
				ASN1err(ASN1_F_A2D_ASN1_OBJECT,16);
				goto err;
				}
			while (--i > 0)
				out[len++]=tmp[i]|0x80;
			out[len++]=tmp[0];
			}
		else
			len+=i;
		}
	return(len);
err:
	return(0);
	}
