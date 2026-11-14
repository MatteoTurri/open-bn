/*
 * BN_mod_inverse. RCOM has the later four-argument form (in, a, n, ctx), where in, when not 0,
 * receives the result, on the BN_CTX temporaries of SSLeay.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

/* solves ax == 1 (mod n) */
BIGNUM *BN_mod_inverse(in, a, n, ctx)
BIGNUM *in;
BIGNUM *a;
BIGNUM *n;
BN_CTX *ctx;
	{
	BIGNUM *A,*B,*X,*Y,*M,*D,*R,*T;
	BIGNUM *ret=NULL;
	int sign;

	A=&(ctx->bn[ctx->tos]);
	B=&(ctx->bn[ctx->tos+1]);
	X=&(ctx->bn[ctx->tos+2]);
	D=&(ctx->bn[ctx->tos+3]);
	M=&(ctx->bn[ctx->tos+4]);
	Y=&(ctx->bn[ctx->tos+5]);
	ctx->tos+=6;
	if (in == NULL)
		R=BN_new();
	else
		R=in;
	if (R == NULL) goto err;

	BN_zero(X);
	BN_one(Y);
	if (BN_copy(A,a) == NULL) goto err;
	if (BN_copy(B,n) == NULL) goto err;
	sign=1;

	while (!((B->top == 0) || (B->top == 1 && B->d[0] == 0)))
		{
		if (!BN_div(D,M,A,B,ctx)) goto err;
		T=A;
		A=B;
		B=M;
		/* T has a struct, M does not */

		if (!BN_mul(T,D,X,ctx)) goto err;
		if (!BN_add(T,T,Y)) goto err;
		M=Y;
		Y=X;
		X=T;
		sign= -sign;
		}
	if (sign < 0)
		{
		if (!BN_sub(Y,n,Y)) goto err;
		}

	if (BN_is_one(A))
		{ if (!BN_mod(R,Y,n,ctx)) goto err; }
	else
		{
		goto err;
		}
	ret=R;
err:
	if (ret == NULL)
		{
		if (in == NULL) BN_free(R);
		}
	ctx->tos-=6;
	return(ret);
	}
