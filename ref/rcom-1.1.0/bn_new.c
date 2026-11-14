/*
 * BN_new.
 */

















#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"


BIGNUM *BN_new()
	{
	BIGNUM *ret;

	if ((ret=(BIGNUM *)Malloc(sizeof(BIGNUM))) == NULL) return(NULL);
	ret->flags=BN_FLG_MALLOCED;
	ret->top=0;
	ret->neg=0;
	ret->max=0;
	ret->d=NULL;
	return(ret);
	}
