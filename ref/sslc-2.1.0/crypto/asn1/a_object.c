/* crypto/asn1/a_object.c */
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

/* ASN1err(ASN1_F_ASN1_OBJECT_NEW,ASN1_R_EXPECTING_AN_OBJECT); 
 * ASN1err(ASN1_F_D2I_ASN1_OBJECT,ASN1_R_BAD_OBJECT_HEADER); 
 * ASN1err(ASN1_F_I2T_ASN1_OBJECT,ASN1_R_BAD_OBJECT_HEADER);
 */

int i2d_ASN1_OBJECT(a, pp)
ASN1_OBJECT *a;
unsigned char **pp;
	{
	unsigned char *p;

	if ((a == NULL) || (a->data == NULL)) return(0);

	if (pp == NULL)
		return(ASN1_object_size(0,a->length,V_ASN1_OBJECT));

	p= *pp;
	ASN1_put_object(&p,0,a->length,V_ASN1_OBJECT,V_ASN1_UNIVERSAL);
	memcpy(p,a->data,a->length);
	p+=a->length;

	*pp=p;
	return(a->length);
	}


int i2t_ASN1_OBJECT(buf,buf_len,a)
char *buf;
int buf_len;
ASN1_OBJECT *a;
	{
	int i,idx=0,n=0,len,nid;
	unsigned long l;
	unsigned char *p;
	char *s;
	char tbuf[32];

	if (buf_len <= 0) return(0);

	if ((a == NULL) || (a->data == NULL))
		{
		buf[0]='\0';
		return(0);
		}

	nid=OBJ_obj2nid(a);
	if (nid == NID_undef)
		{
		len=a->length;
		p=a->data;

		idx=0;
		l=0;
		while (idx < a->length)
			{
			l|=(p[idx]&0x7f);
			if (!(p[idx] & 0x80)) break;
			l<<=7L;
			idx++;
			}
		idx++;
		i=(int)(l/40);
		if (i > 2) i=2;
		l-=(long)(i*40);

		sprintf(tbuf,"%d.%ld",i,l);
		i=strlen(tbuf);
		strncpy(buf,tbuf,buf_len);
		buf_len-=i;
		buf+=i;
		n+=i;

		l=0;
		for (; idx<len; idx++)
			{
			l|=p[idx]&0x7f;
			if (!(p[idx] & 0x80))
				{
				sprintf(tbuf,".%ld",l);
				i=strlen(tbuf);
				if (buf_len > 0)
					strncpy(buf,tbuf,buf_len);
				buf_len-=i;
				buf+=i;
				n+=i;
				l=0;
				}
			l<<=7L;
			}
		}
	else
		{
		s=(char *)OBJ_nid2ln(nid);
		if (s == NULL)
			s=(char *)OBJ_nid2sn(nid);
		strncpy(buf,s,buf_len);
		n=strlen(s);
		}
	buf[buf_len-1]='\0';
	return(n);
	}

