/*
 * BN_lshift1. In RCOM the result is expanded before its sign and length are set.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

int BN_lshift1(r, a)
BIGNUM *r;
BIGNUM *a;
	{
	register BN_ULONG *ap,*rp,t,c;
	int i;

	if (r != a)
		{
		if (bn_wexpand(r,a->top+1) == NULL) return(0);
		r->neg=a->neg;
		r->top=a->top;
		}
	else
		{
		if (bn_wexpand(r,a->top+1) == NULL) return(0);
		}
	ap=a->d;
	rp=r->d;
	c=0;
	for (i=0; i<a->top; i++)
		{
		t= *(ap++);
		*(rp++)=((t<<1)|c)&BN_MASK2;
		c=(t & BN_TBIT)?1:0;
		}
	if (c)
		{
		*rp=1;
		r->top++;
		}
	return(1);
	}
