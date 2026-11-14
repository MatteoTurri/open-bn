/*
 * BN_CTX_init and BN_CTX_free: the BN_CTX of RCOM holds its BIGNUMs by value.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"


void BN_CTX_init(c)
BN_CTX *c;
	{
	memset(c,0,sizeof(BN_CTX));
	c->tos=0;
	c->flags=0;
	}

void BN_CTX_free(c)
BN_CTX *c;
	{
	int i;

	for (i=0; i<BN_CTX_NUM; i++)
		BN_clear_free(&(c->bn[i]));
	if (c->flags & BN_FLG_MALLOCED)
		Free(c);
	}
