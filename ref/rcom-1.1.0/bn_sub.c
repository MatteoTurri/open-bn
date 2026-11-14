/*
 * BN_sub of RCOM: built on BN_uadd and BN_usub; when both operands are negative it swaps them and
 * takes the same path as two nonnegative operands.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

int BN_sub(r, a, b)
BIGNUM *r;
BIGNUM *a;
BIGNUM *b;
	{
	int max;
	int use_add,r_neg;
	BIGNUM *tmp;

	use_add=0;
	r_neg=0;

	if (a->neg)
		{
		if (b->neg != 0)
			{
			/* both negative: swap, then subtract as in the nonnegative case */
			tmp=a; a=b; b=tmp;
			}
		else
			{
			use_add=1;
			r_neg=1;
			}
		}
	else
		{
		if (b->neg != 0)
			{
			use_add=1;
			r_neg=0;
			}
		}

	if (use_add)
		{
		if (!BN_uadd(r,a,b)) return(0);
		r->neg=r_neg;
		return(1);
		}

	max = (b->top < a->top) ? a->top : b->top;
	if (bn_wexpand(r,max) == NULL) return(0);

	if (BN_ucmp(a,b) < 0)
		{
		if (!BN_usub(r,b,a)) return(0);
		r->neg=1;
		}
	else
		{
		if (!BN_usub(r,a,b)) return(0);
		r->neg=0;
		}
	return(1);
	}
