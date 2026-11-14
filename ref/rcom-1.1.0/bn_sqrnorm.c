/*
 * bn_sqr_normal: the schoolbook square of OpenSSL (the off-diagonal products once, doubled, plus the
 * squares of the words).
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

void bn_sqr_normal(r, a, n, tmp)
BN_ULONG *r,*a,*tmp;
int n;
	{
	int i,j,max;
	BN_ULONG *ap,*rp;
	BN_ULONG w;

	max=n*2;
	ap=a;
	rp=r;
	rp[0]=rp[max-1]=0;
	rp++;
	j=n;

	if (--j > 0)
		{
		w=*(ap++);
		rp[j]=bn_mul_words(rp,ap,j,w);
		rp+=2;
		}

	i=n-2;
	while (i>0)
		{
		j--;
		w=*(ap++);
		rp[j]=bn_mul_add_words(rp,ap,j,w);
		rp+=2;
		i--;
		}

	bn_add_words(r,r,r,max);

	/* There will not be a carry */

	bn_sqr_words(tmp,a,n);

	bn_add_words(r,r,tmp,max);
	}
