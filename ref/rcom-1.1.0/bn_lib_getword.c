/*
 * BN_get_word.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

unsigned long BN_get_word(a)
BIGNUM *a;
	{
	int i,n;
	unsigned long ret=0;

	n=BN_num_bytes(a);
	if (n > 4)
#ifdef SIXTY_FOUR_BIT_LONG
		return(BN_MASK2);
#else
		return(0xFFFFFFFFL);
#endif
	i=a->top-1;
	while (i>=0)
		{
#ifndef SIXTY_FOUR_BIT
		ret<<=BN_BITS4;
		ret<<=BN_BITS4;
#endif
		ret|=a->d[i];
		i--;
		}
	return(ret);
	}
