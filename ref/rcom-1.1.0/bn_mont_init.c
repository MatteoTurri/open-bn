/*
 * BN_MONT_CTX_init and BN_MONT_CTX_free.
 */





#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"


void BN_MONT_CTX_init(mont)
BN_MONT_CTX *mont;
	{
	mont->ri=0;
	mont->nwords=0;
	BN_init(&(mont->RR));
	BN_init(&(mont->N));
	BN_init(&(mont->Ni));
	mont->flags=0;
	}

void BN_MONT_CTX_free(mont)
BN_MONT_CTX *mont;
	{
	BN_free(&(mont->RR));
	BN_free(&(mont->N));
	BN_free(&(mont->Ni));
	if (mont->flags & BN_FLG_MALLOCED)
		Free(mont);
	}
