/*
 * BN_mask_bits.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

/* RCOM: the trim of the zero top words is a separate step, done in both cases, walking a pointer
 * down from the top word. */
int BN_mask_bits(a,n)
BIGNUM *a;
int n;
	{
	int b,w;
	BN_ULONG *p;

	w=n/BN_BITS2;
	b=n%BN_BITS2;
	if (w >= a->top) return(0);
	if (b == 0)
		a->top=w;
	else
		{
		a->top=w+1;
		a->d[w]&= ~(BN_MASK2<<b);
		}

	if (a->top > 0)
		{
		p= &(a->d[a->top-1]);
		while (a->top > 0)
			{
			if (*(p--) != 0)
				break;
			a->top--;
			}
		}
	return(1);
	}
