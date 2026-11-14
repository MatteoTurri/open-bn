/* crypto/stack/stack.c */
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

/* Code for stacks
 * Author - Eric Young v 1.0
 * 1.2 eay 12-Mar-97 -	Modified sk_find so that it _DOES_ return the
 *			lowest index for the seached item.
 *
 * 1.1 eay - Take from netdb and added to SSLeay
 *
 * 1.0 eay - First version 29/07/92
 */
/*
 * sk_find, sk_move, sk_push, sk_unshift, sk_shift, sk_pop and sk_zero, from crypto/stack/stack.c.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "stack.h"


#ifndef NOPROTO
#define	FP_ICC	(int (*)(const void *,const void *))
#else
#define FP_ICC
#endif

#include <errno.h>

int sk_find(st,data)
STACK *st;
char *data;
	{
	char **r;
	int i;
	int (*comp_func)();

	if (st->comp == NULL)
		{
		for (i=0; i<st->num; i++)
			if (st->data[i] == data)
				return(i);
		return(-1);
		}
	comp_func=(int (*)())st->comp;
	if (!st->sorted)
		{
		qsort((char *)st->data,st->num,sizeof(char *),FP_ICC comp_func);
		st->sorted=1;
		}
	if (data == NULL) return(-1);
	r=(char **)bsearch(&data,(char *)st->data,
		st->num,sizeof(char *),FP_ICC comp_func);
	if (r == NULL) return(-1);
	i=(int)(r-st->data);
	for ( ; i>0; i--)
		if ((*st->comp)(&(st->data[i-1]),&data) < 0)
			break;
	return(i);
	}

/* SSL-C: moves the element at loc to the end of the stack. */
int sk_move(st,loc,to_end)
STACK *st;
int loc;
int to_end;
	{
	char *data;
	int ret;

	ret=0;
	if ((loc < 0) || (loc >= st->num))
		return(-1);
	data=st->data[loc];
	sk_delete(st,loc);
	if (to_end)
		ret=sk_push(st,data);
	else
		ret=sk_insert(st,data,0);
	return(ret);
	}

int sk_push(st,data)
STACK *st;
char *data;
	{
	return(sk_insert(st,data,st->num));
	}

int sk_unshift(st,data)
STACK *st;
char *data;
	{
	return(sk_insert(st,data,0));
	}

char *sk_shift(st)
STACK *st;
	{
	if (st == NULL) return(NULL);
	if (st->num <= 0) return(NULL);
	return(sk_delete(st,0));
	}

char *sk_pop(st)
STACK *st;
	{
	if (st == NULL) return(NULL);
	if (st->num <= 0) return(NULL);
	return(sk_delete(st,st->num-1));
	}

void sk_zero(st)
STACK *st;
	{
	if (st == NULL) return;
	if (st->num <= 0) return;
	memset((char *)st->data,0,sizeof(st->data)*st->num);
	st->num=0;
	}
