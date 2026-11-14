/*
 * bn_mul_rec_words: the recursive (Karatsuba) product of two equal power-of-two sizes. dt carries
 * the remaining depth, the current half size and the comba kernels of the base case; neg tracks the
 * sign of the middle term |a0 - a1| * |b1 - b0|, which is added or subtracted at the merge.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

void bn_mul_rec_words(r,a,b,t,dt)
BN_ULONG *r;
BN_ULONG *a;
BN_ULONG *b;
BN_ULONG *t;
struct bn_rec_dtab *dt;
	{
	int n2,n,c1,neg;
	void (*mul)();

	n2=dt->size;
	n=n2/2;
	neg=0;
	if (dt->depth == 0)
		{
		mul=dt->mul8;
		(*mul)(r,a,b,n);
		(*mul)(&(r[n2]),&(a[n]),&(b[n]),n);
		if (bn_sub_words(&(t[n2]),a,&(a[n]),n))
			{
			bn_2s_comp(&(t[n2]),&(t[n2]),n);
			neg=1;
			}
		else
			neg=0;
		if (bn_sub_words(&(t[n2+n]),&(b[n]),b,n))
			{
			neg^=1;
			bn_2s_comp(&(t[n2+n]),&(t[n2+n]),n);
			}
		(*mul)(t,&(t[n2]),&(t[n2+n]),n);
		}
	else
		{
		dt->depth--;
		dt->size=n;
		bn_mul_rec_words(r,a,b,&(t[n2+n2]),dt);
		bn_mul_rec_words(&(r[n2]),&(a[n]),&(b[n]),&(t[n2+n2]),dt);
		neg=0;
		if (bn_sub_words(&(t[n2]),a,&(a[n]),n))
			{
			bn_2s_comp(&(t[n2]),&(t[n2]),n);
			neg=1;
			}
		else
			neg=0;
		if (bn_sub_words(&(t[n2+n]),&(b[n]),b,n))
			{
			neg^=1;
			bn_2s_comp(&(t[n2+n]),&(t[n2+n]),n);
			}
		bn_mul_rec_words(t,&(t[n2]),&(t[n2+n]),&(t[n2+n2]),dt);
		dt->size=n2;
		dt->depth++;
		}
	c1=bn_add_words(&(t[n2]),r,&(r[n2]),n2);
	if (neg)
		c1-=bn_sub_words(t,&(t[n2]),t,n2);
	else
		c1+=bn_add_words(t,&(t[n2]),t,n2);
	t[n2]=c1;
	c1=bn_add_words(&(r[n]),&(r[n]),t,n2+1);
	if (c1)
		{
		t= &(r[n2+n+1]);
		for (;;)
			{
			if (++*(t++)) break;
			}
		}
	}
