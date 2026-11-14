/*
 * BN_lshift. RCOM hands a shift by one to BN_lshift1 and sets the sign after the expansion.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

int BN_lshift(r, a, n)
BIGNUM *r;
BIGNUM *a;
int n;
	{
	int i,nw,lb,rb;
	BN_ULONG *t,*f;
	BN_ULONG l;

	if (n == 1) return(BN_lshift1(r,a));

	if (bn_wexpand(r,a->top+(n/BN_BITS2)+1) == NULL) return(0);
	r->neg=a->neg;
	nw=n/BN_BITS2;
	lb=n%BN_BITS2;
	rb=BN_BITS2-lb;
	f=a->d;
	t=r->d;
	t[a->top+nw]=0;
	if (lb == 0)
		{
		i=a->top-1;
		while (i>=0)
			{
			t[nw+i]=f[i];
			i--;
			}
		}
	else
		{
		i=a->top-1;
		while (i>=0)
			{
			l=f[i];
			t[nw+i+1]|=(l>>rb)&BN_MASK2;
			t[nw+i]=(l<<lb)&BN_MASK2;
			i--;
			}
		}
	memset(t,0,nw*sizeof(t[0]));
/*	for (i=0; i<nw; i++)
		t[i]=0;*/
	r->top=a->top+nw+1;
	bn_fix_top(r);
	return(1);
	}
