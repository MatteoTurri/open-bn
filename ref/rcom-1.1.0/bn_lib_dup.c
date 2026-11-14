/*
 * BN_dup.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"


BIGNUM *BN_dup(a)
BIGNUM *a;
	{
	BIGNUM *r;

	r=BN_new();
	if (r == NULL) return(NULL);
	return((BIGNUM *)BN_copy(r,a));
	}
