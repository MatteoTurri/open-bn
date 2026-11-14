/*
 * BN_num_bits.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

int BN_num_bits(a)
BIGNUM *a;
	{
	BN_ULONG l;
	int i;

	if (a->top == 0) return(0);
	l=a->d[a->top-1];
	i=(a->top-1)*BN_BITS2;
	return(i+BN_num_bits_word(l));
	}
