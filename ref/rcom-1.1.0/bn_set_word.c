/*
 * BN_set_word.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

int BN_set_word(a,w)
BIGNUM *a;
unsigned long w;
	{
	int i,n;

	if (((a->max > 0) ? a : bn_expand2(a,2)) == NULL) return(0);

	n=sizeof(unsigned long)/BN_BYTES;
	a->neg=0;
	a->top=0;
	a->d[0]=(BN_ULONG)w&BN_MASK2;
	if (a->d[0] != 0) a->top=1;
	i=1;
	while (i<n)
		{
#ifndef SIXTY_FOUR_BIT
		w>>=BN_BITS4;
		w>>=BN_BITS4;
#endif
		a->d[i]=(BN_ULONG)w&BN_MASK2;
		if (a->d[i] != 0) a->top=i+1;
		i++;
		}
	return(1);
	}
