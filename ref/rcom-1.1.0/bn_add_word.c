/*
 * BN_add_word.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

/* RCOM: a one-word negative a (top < 2) is handled here instead of going through BN_sub_word, and
 * the new top word is cleared before the add. */
int BN_add_word(a, w)
BIGNUM *a;
unsigned long w;
	{
	BN_ULONG l;
	int i;

	if (w == 0) return(1);
	if (a->neg)
		{
		if (a->top >= 2)
			{
			a->neg=0;
			i=BN_sub_word(a,w);
			a->neg=1;
			return(i);
			}
		else
			{
			l=a->d[0];
			if (l > w)
				{
				a->d[0]=l-w;
				}
			else if (l < w)
				{
				a->neg=0;
				a->d[0]=w-l;
				}
			else
				{
				a->neg=0;
				a->top=0;
				}
			return(1);
			}
		}
	if (bn_wexpand(a,a->top+1) == NULL) return(0);
	a->d[a->top]=0;
	i=0;
	for (;;)
		{
		l=(a->d[i]+(BN_ULONG)w)&BN_MASK2;
		a->d[i]=l;
		if (w > l)
			w=1;
		else
			break;
		i++;
		}
	if (i >= a->top)
		a->top++;
	return(1);
	}
