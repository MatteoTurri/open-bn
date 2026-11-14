/*
 * BN_clear_free, BN_free and BN_init.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

void BN_clear_free(a)
BIGNUM *a;
	{
	int i;

	if (a == NULL) return;
	if (a->d != NULL)
		{
		memset(a->d,0,a->max*sizeof(a->d[0]));
		if (!(a->flags & BN_FLG_STATIC_DATA))
			Free(a->d);
		}
	i=a->flags & BN_FLG_MALLOCED;
	memset(a,0,sizeof(BIGNUM));
	if (i)
		Free(a);
	}

void BN_free(a)
BIGNUM *a;
	{
	if (a == NULL) return;
	if ((a->d != NULL) && !(a->flags & BN_FLG_STATIC_DATA))
		Free(a->d);
	a->flags|=BN_FLG_FREE;
	if (a->flags & BN_FLG_MALLOCED)
		Free(a);
	}


/* BN_init, an RCOM addition: clears a BIGNUM that lives on the stack or inside another structure. */
void BN_init(a)
BIGNUM *a;
	{
	memset(a,0,sizeof(BIGNUM));
	}
