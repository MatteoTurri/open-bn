/*
 * BN_sub_word.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

/* RCOM: as BN_add_word, w == 0 and a BIGNUM of zero or one word are handled here. */
int BN_sub_word(a, w)
BIGNUM *a;
unsigned long w;
	{
	int i;

	if (w == 0) return(1);
	if (a->neg)
		{
		a->neg=0;
		i=BN_add_word(a,w);
		a->neg=1;
		return(i);
		}

	if (a->top < 2)
		{
		if (a->top == 0)
			{
			if (((a->max > 0) ? a : bn_expand2(a,1)) == NULL) return(0);
			a->d[0]=w;
			a->neg=1;
			a->top=1;
			return(1);
			}
		/* fallthrough: a->top==1 (outer guard ensures a->top<2) */
		{
		BN_ULONG l;

		l=a->d[0];
		if (l == w)
			{
			a->top=0;
			}
		else if (l > w)
			{
			a->d[0]=l-w;
			}
		else
			{
			a->neg=1;
			a->d[0]=w-l;
			}
		return(1);
		}
		}
	i=0;
	for (;;)
		{
		if (a->d[i] >= w)
			{
			a->d[i]-=w;
			break;
			}
		else
			{
			a->d[i]=(a->d[i]-w)&BN_MASK2;
			i++;
			w=1;
			}
		}
	if ((a->d[i] == 0) && (i == (a->top-1)))
		a->top--;
	return(1);
	}
