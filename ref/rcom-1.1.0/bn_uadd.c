/*
 * BN_uadd: the unsigned add of RCOM (bn_qadd of SSLeay made public). It puts the longer operand
 * first, expands the result itself and returns 0 when the expansion fails, 1 otherwise.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

int BN_uadd(r,a,b)
BIGNUM *r;
BIGNUM *a;
BIGNUM *b;
	{
	register int i;
	int max,min;
	BN_ULONG *ap,*bp,*rp,carry,t1;
	BIGNUM *tmp;

	if (a->top < b->top)
		{ tmp=a; a=b; b=tmp; }

	max=a->top;
	min=b->top;
	if (bn_wexpand(r,max+1) == NULL) return(0);
	r->top=max;

	ap=a->d;
	bp=b->d;
	rp=r->d;
	carry=0;

	carry=bn_add_words(rp,ap,bp,min);
	rp+=min;
	ap+=min;
	bp+=min;
	i=min;

	if (carry)
		{
		while (i < max)
			{
			i++;
			t1= *(ap++);
			if ((*(rp++)=(t1+1)&BN_MASK2) >= t1)
				{
				carry=0;
				break;
				}
			}
		if ((i >= max) && carry)
			{
			*(rp++)=1;
			r->top++;
			}
		}
	if (rp != ap)
		for (; i<max; i++)
			*(rp++)= *(ap++);
	return(1);
	}
