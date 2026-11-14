/* ssl/ssl_lib.c */
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
 * ssl_cipher_list_to_bytes and ssl_bytes_to_cipher_list, from ssl/ssl_lib.c.
 */
#include <stdio.h>
#include "objects.h"
#include "lhash.h"
#include "ssl_locl.h"


/* SSL-C: struct ssl_rw_st (the two symmetric blocks of ssl_st) is in ssl.h: ssl3_change_cipher_state
 * uses it too. */

/* Used to change an SSL_CTXs default SSL method type */

int ssl_cipher_list_to_bytes(s,sk,p)
SSL *s;
STACK *sk;
unsigned char *p;
	{
	int i,j=0;
	SSL_CIPHER *c;
	unsigned char *q;

	if (sk == NULL) return(0);
	q=p;

	for (i=0; i<sk_num(sk); i++)
		{
		c=(SSL_CIPHER *)sk_value(sk,i);
		j=ssl_put_cipher_by_char(s,c,p);
		/* SSL-C: tests p (not j) before moving on: the function is also
		 * called with p == NULL, only to count. */
		if (p != NULL)
			p+=j;
		}
	return(p-q);
	}

STACK *ssl_bytes_to_cipher_list(s,p,num,skp)
SSL *s;
unsigned char *p;
int num;
STACK **skp;
	{
	SSL_CIPHER *c;
	STACK *sk;
	int i,n;

	n=ssl_put_cipher_by_char(s,NULL,NULL);
	if ((num%n) != 0)
		{
#line 1203 /* the line number is part of the binary */
		SSLerr(162,151);
		return(NULL);
		}
	if ((skp == NULL) || (*skp == NULL))
		sk=sk_new(NULL); /* change perhaps later */
	else
		{
		sk= *skp;
		sk_zero(sk);
		}

	for (i=0; i<num; i+=n)
		{
		c=ssl_get_cipher_by_char(s,p);
		p+=n;
		if (c != NULL)
			{
			if (!sk_push(sk,(char *)c))
				{
				SSLerr(162,ERR_R_MALLOC_FAILURE);
				goto err;
				}
			}
		}

	if (skp != NULL)
		*skp=sk;
	return(sk);
err:
	if ((skp == NULL) || (*skp == NULL))
		sk_free(sk);
	return(NULL);
	}
