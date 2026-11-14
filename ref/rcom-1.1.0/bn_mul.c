/*
 * BN_mul of RCOM, in the later OpenSSL form: bn_mul_comba8 for two 8-word operands, the recursive
 * bn_mul_rec_words when both have the same power-of-two size of at least 16 words (the shorter
 * operand is padded with a zero word when the sizes differ by one), bn_mul_normal otherwise.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

int BN_mul(r,a,b,ctx)
BIGNUM *r;
BIGNUM *a;
BIGNUM *b;
BN_CTX *ctx;
	{
	int top,al,bl;
	int sign;
	BIGNUM *rr,*t;
	int i;
	int j,k;
	int nb;
	struct bn_rec_dtab dtab;

	al=a->top;
	bl=b->top;

	if ((al == 0) || (bl == 0))
		{
		BN_zero(r);
		return(1);
		}
	top=al+bl;

	sign=a->neg^b->neg;

	if ((r == a) || (r == b))
		rr= &(ctx->bn[ctx->tos+1]);
	else
		rr=r;

	if (bn_wexpand(rr,top) == NULL) return(0);
	rr->top=top;

	if (al == bl)
		{
		if (al == 8)
			{
			bn_mul_comba8(rr->d,a->d,b->d);
			goto end;
			}
		else if (al < 16)
			{
			bn_mul_normal(rr->d,a->d,al,b->d,bl);
			goto end;
			}
		goto symetric;
		}
	else if ((al < 16) || (bl < 16))
		{
		bn_mul_normal(rr->d,a->d,al,b->d,bl);
		goto end;
		}
	else
		{
		i=(al-bl);
		if ((i ==  1) && !(b->flags & BN_FLG_STATIC_DATA))
			{
			bn_wexpand(b,al);
			b->d[bl]=0;
			bl++;
			goto symetric;
			}
		else if ((i ==  -1) && !(a->flags & BN_FLG_STATIC_DATA))
			{
			bn_wexpand(a,bl);
			a->d[al]=0;
			al++;
			goto symetric;
			}
		}

asymetric:
	bn_mul_normal(rr->d,a->d,al,b->d,bl);

	if (0)
		{
symetric:
		nb=BN_num_bits_word((BN_ULONG)al);
		j=1<<(nb-1);
		k=j+j;
		t= &(ctx->bn[ctx->tos]);
		if (al == j)
			{
			dtab.depth=nb-5;
			dtab.size=j;
			dtab.mul8=(void (*)())bn_mul_comba8;
			dtab.sqr8=(void (*)())bn_sqr_comba8;
			bn_wexpand(t,k+k);
			bn_wexpand(rr,k);
			bn_mul_rec_words(rr->d,a->d,b->d,t->d,&dtab);
			}
		else
			goto asymetric;
		}
end:
	r->neg=sign;
	bn_fix_top(rr);
	if (r != rr) BN_copy(r,rr);
	return(1);
	}
