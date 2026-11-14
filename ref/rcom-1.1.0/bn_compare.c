/*
 * BN_cmp, BN_set_bit and BN_is_bit_set of the RCOM big numbers.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

int BN_cmp(a, b)
BIGNUM *a;
BIGNUM *b;
	{
	int i;
	int gt,lt;
	BN_ULONG t1,t2;

	if ((a == NULL) || (b == NULL))
		{
		if (a != NULL)
			return(-1);
		else if (b != NULL)
			return(1);
		else
			return(0);
		}
	if (a->neg != b->neg)
		{
		if (a->neg)
			return(-1);
		else	return(1);
		}
	if (a->neg == 0)
		{ gt=1; lt= -1; }
	else	{ gt= -1; lt=1; }

	if (a->top > b->top) return(gt);
	if (a->top < b->top) return(lt);
	i=a->top-1;
	while (i>=0)
		{
		t1=a->d[i];
		t2=b->d[i];
		if (t1 > t2) return(gt);
		if (t1 < t2) return(lt);
		i--;
		}
	return(0);
	}

/* RCOM: the words between the old top and the new one are zeroed. */
int BN_set_bit(a, n)
BIGNUM *a;
int n;
	{
	int i,j,k;

	i=n/BN_BITS2;
	j=n%BN_BITS2;
	if (a->top <= i)
		{
		if (bn_wexpand(a,i+1) == NULL) return(0);

		k=a->top;
		while (k<i+1)
			{
			a->d[k]=0;
			k++;
			}
		a->top=i+1;
		}

	a->d[i]|=(((BN_ULONG)1)<<j);
	return(1);
	}

int BN_is_bit_set(a, n)
BIGNUM *a;
int n;
	{
	int i,j;

	if (n < 0) return(0);
	i=n/BN_BITS2;
	j=n%BN_BITS2;
	if (a->top <= i) return(0);
	return((a->d[i]&(((BN_ULONG)1)<<j))?1:0);
	}
