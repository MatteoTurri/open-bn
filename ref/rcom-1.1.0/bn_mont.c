/*
 * BN_MONT_CTX_new. The BN_MONT_CTX of RCOM holds RR, N and Ni by value, so there is nothing to unwind
 * on failure.
 */




















#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"



BN_MONT_CTX *BN_MONT_CTX_new()
	{
	BN_MONT_CTX *ret;

	if ((ret=(BN_MONT_CTX *)Malloc(sizeof(BN_MONT_CTX))) == NULL) return(NULL);
	BN_MONT_CTX_init(ret);
	ret->flags=BN_FLG_MALLOCED;
	return(ret);
	}

