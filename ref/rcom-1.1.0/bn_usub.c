/*
 * BN_usub: the unsigned subtract of RCOM (bn_qsub of SSLeay made public). It returns 0 when a is
 * shorter than b, expands the result itself, and copies the tail unrolled by 4.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

int BN_usub(r, a, b)
BIGNUM *r;
BIGNUM *a;
BIGNUM *b;
	{
	int max,min;
	register BN_ULONG t1,t2,*ap,*bp,*rp;
	int i,carry;

	if (a->top < b->top) return(0);

	max=a->top;
	min=b->top;
	if (bn_wexpand(r,max) == NULL) return(0);

	ap=a->d;
	bp=b->d;
	rp=r->d;

	carry=0;
	for (i=0; i<min; i++)
		{
		t1= *(ap++);
		t2= *(bp++);
		if (carry)
			{
			carry=(t1 <= t2);
			t1=(t1-t2-1)&BN_MASK2;
			}
		else
			{
			carry=(t1 < t2);
			t1=(t1-t2)&BN_MASK2;
			}
		*(rp++)=t1&BN_MASK2;
		}
	if (carry) /* subtracted */
		{
		while (i < max)
			{
			i++;
			t1= *(ap++);
			t2=(t1-1)&BN_MASK2;
			*(rp++)=t2;
			if (t1 > t2) break;
			}
		}

	if (rp != ap)
		for (;;)
			{
			if (i++ >= max) break;
			rp[0]=ap[0];
			if (i++ >= max) break;
			rp[1]=ap[1];
			if (i++ >= max) break;
			rp[2]=ap[2];
			if (i++ >= max) break;
			rp[3]=ap[3];
			rp+=4;
			ap+=4;
			}

	r->top=max;
	bn_fix_top(r);
	return(1);
	}
