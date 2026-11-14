/*
 * BN_mod_mul, on the BN_CTX of RCOM (BIGNUMs by value).
 */
#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

/* slow but works */
int BN_mod_mul(ret, a, b, m, ctx)
BIGNUM *ret;
BIGNUM *a;
BIGNUM *b;
BIGNUM *m;
BN_CTX *ctx;
	{
	BIGNUM *t;
	int r=0;

	t= &(ctx->bn[ctx->tos++]);
	if (a == b)
		{ if (!BN_sqr(t,a,ctx)) goto err; }
	else
		{ if (!BN_mul(t,a,b,ctx)) goto err; }
	if (!BN_mod(ret,t,m,ctx)) goto err;
	r=1;
err:
	ctx->tos--;
	return(r);
	}
