/*
 * BN_add. In RCOM BN_uadd and BN_usub grow the result themselves, so BN_add only picks the operation
 * and the operand order and checks their return value.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

/*  a +  b	a+b
 *  a + -b	a-b
 * -a +  b	b-a
 * -a + -b	-(a+b)
 */
int BN_add(r, a, b)
BIGNUM *r;
BIGNUM *a;
BIGNUM *b;
	{
	BIGNUM *tmp;

	if (a->neg != b->neg)
		{
		/* only one is negative */
		if (a->neg)
			{ tmp=a; a=b; b=tmp; }

		/* we are now a - b */

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

	if (a->neg)
		r->neg=1;
	else
		r->neg=0;

	if (!BN_uadd(r,a,b)) return(0);
	return(1);
	}
