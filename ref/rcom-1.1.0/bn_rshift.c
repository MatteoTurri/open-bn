/*
 * BN_rshift. RCOM hands a shift by one to BN_rshift1, expands the result by one more word and sets
 * the sign after the expansion.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

int BN_rshift(r, a, n)
BIGNUM *r;
BIGNUM *a;
int n;
	{
	int i,j,nw,lb,rb;
	BN_ULONG *t,*f;
	BN_ULONG l,tmp;

	if (n == 1) return(BN_rshift1(r,a));

	nw=n/BN_BITS2;
	rb=n%BN_BITS2;
	lb=BN_BITS2-rb;
	if (nw > a->top)
		{
		BN_zero(r);
		return(1);
		}
	if (r != a)
		{
		if (bn_wexpand(r,a->top-nw+2) == NULL) return(0);
		r->neg=a->neg;
		}

	f= &(a->d[nw]);
	t=r->d;
	j=a->top-nw;
	r->top=j;

	if (rb == 0)
		{
		i=j+1;
		while (i > 0)
			{
			*(t++)= *(f++);
			i--;
			}
		}
	else
		{
		l= *(f++);
		i=1;
		while (i<j)
			{
			tmp =(l>>rb)&BN_MASK2;
			l= *(f++);
			*(t++) =(tmp|(l<<lb))&BN_MASK2;
			i++;
			}
		*(t++) =(l>>rb)&BN_MASK2;
		*t=0;
		}
	bn_fix_top(r);
	return(1);
	}
