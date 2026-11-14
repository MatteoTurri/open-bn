/*
 * BN_mod_mul_montgomery of RCOM: calls the word kernels directly (bn_sqr_comba8 or bn_sqr_normal to
 * square, bn_mul_comba8 or bn_mul_normal to multiply, comba when the operands have 8 words), zeroes
 * the product up to 2 * nl words and reduces it with bn_from_montgomery_words.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

int BN_mod_mul_montgomery(r,a,b,mont,ctx)
BIGNUM *r,*a,*b;
BN_MONT_CTX *mont;
BN_CTX *ctx;
	{
	BIGNUM *tmp,*tmp2;
	int retn=0;
	int nl;
	int i;

	nl=mont->nwords;

	tmp=&(ctx->bn[ctx->tos++]);
	tmp2=&(ctx->bn[ctx->tos++]);

	if (bn_wexpand(tmp,nl+nl) == NULL) goto err;
	if (bn_wexpand(r,nl+nl) == NULL) goto err;

	if (a == b)
		{
		if (bn_wexpand(tmp2,nl+nl) == NULL) goto err;
		if ((a->top == 8) && (a->top == b->top))
			bn_sqr_comba8(tmp->d,a->d);
		else
			bn_sqr_normal(tmp->d,a->d,a->top,tmp2->d);
		}
	else
		{
		if (a->top == 8)
			bn_mul_comba8(tmp->d,a->d,b->d);
		else
			bn_mul_normal(tmp->d,a->d,a->top,b->d,b->top);
		}

	i=a->top+b->top;
	while (i < nl+nl)
		{
		tmp->d[i]=0;
		i++;
		}
	tmp->top=nl+nl;
	tmp->neg=0;

	bn_from_montgomery_words(r->d,tmp->d,mont->N.d,nl,mont->n0);
	r->top=nl;
	r->neg=0;
	bn_fix_top(r);

	retn=1;
err:
	ctx->tos-=2;
	return(retn);
	}
