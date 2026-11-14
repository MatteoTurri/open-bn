/*
 * bn_sqr_rec_words: the recursive square of a power-of-two size. dt carries the remaining depth, the
 * current half size and the comba kernels of the base case, saved and restored around each level.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

void bn_sqr_rec_words(r,a,t,dt)
BN_ULONG *r;
BN_ULONG *a;
BN_ULONG *t;
struct bn_rec_dtab *dt;
	{
	int n2,n,c1;
	void (*sqr)();

	n2=dt->size;
	n=n2/2;
	if (dt->depth == 0)
		{
		sqr=dt->sqr8;
		(*sqr)(r,a,n);
		(*sqr)(&(r[n2]),&(a[n]),n);
		if (bn_sub_words(&(t[n2]),a,&(a[n]),n))
			bn_2s_comp(&(t[n2]),&(t[n2]),n);
		(*sqr)(t,&(t[n2]),n);
		}
	else
		{
		dt->depth--;
		dt->size=n;
		bn_sqr_rec_words(r,a,&(t[n2+n2]),dt);
		bn_sqr_rec_words(&(r[n2]),&(a[n]),&(t[n2+n2]),dt);
		if (bn_sub_words(&(t[n2]),a,&(a[n]),n))
			bn_2s_comp(&(t[n2]),&(t[n2]),n);
		bn_sqr_rec_words(t,&(t[n2]),&(t[n2+n2]),dt);
		dt->size=n2;
		dt->depth++;
		}
	c1=bn_add_words(&(t[n2]),r,&(r[n2]),n2);
	c1-=bn_sub_words(t,&(t[n2]),t,n2);
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
