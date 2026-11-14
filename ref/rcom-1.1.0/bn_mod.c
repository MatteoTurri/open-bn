/*
 * BN_mod: BN_div without the quotient.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

int BN_mod(rem, m, d,ctx)
BIGNUM *rem;
BIGNUM *m;
BIGNUM *d;
BN_CTX *ctx;
	{
	return(BN_div(NULL,rem,m,d,ctx));
	}
