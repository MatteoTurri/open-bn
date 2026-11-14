/*
 * BN_CTX_new.
 */



#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

/* RCOM: the BIGNUMs are part of the BN_CTX, so one allocation and BN_CTX_init replace the loop of
 * BN_new calls. */


BN_CTX *BN_CTX_new()
	{
	BN_CTX *ret;

	ret=(BN_CTX *)Malloc(sizeof(BN_CTX));
	if (ret == NULL) return(NULL);
	BN_CTX_init(ret);
	ret->flags=BN_FLG_MALLOCED;
	return(ret);
	}
