/*
 * BN_rshift1.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

int BN_rshift1(r, a)
BIGNUM *r;
BIGNUM *a;
	{
	BN_ULONG *ap,*rp,t,c;
	int i;

	if ((a->top == 0) || ((a->top == 1) && (a->d[0] == 0)))
		{
		BN_zero(r);
		return(1);
		}
	if (a != r)
		{
		if (bn_wexpand(r,a->top) == NULL) return(0);
		r->top=a->top;
		r->neg=a->neg;
		}
	ap=a->d;
	rp=r->d;
	c=0;
	for (i=a->top-1; i>=0; i--)
		{
		t=ap[i];
		rp[i]=((t>>1)&BN_MASK2)|c;
		c=(t&1)?BN_TBIT:0;
		}
	bn_fix_top(r);
	return(1);
	}
