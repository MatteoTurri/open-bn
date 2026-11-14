/*
 * BN_sqr of RCOM, in the later OpenSSL form: bn_sqr_comba4 and bn_sqr_comba8 for 4 and 8 words,
 * bn_sqr_rec_words for a power-of-two size of at least 16 words, bn_sqr_normal otherwise (with a
 * 32-word scratch on the stack below 16 words). The result loses at most one leading zero word.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

int BN_sqr(r,a,ctx)
BIGNUM *r;
BIGNUM *a;
BN_CTX *ctx;
	{
	int top,al;
	BIGNUM *t,*rr;
	BN_ULONG tmp[32];
	int j,nb,k;
	struct bn_rec_dtab dtab;

	t=&(ctx->bn[ctx->tos]);
	rr=(a == r) ? &(ctx->bn[ctx->tos+1]) : r;

	al=a->top;
	if (al <= 0)
		{
		r->top=0;
		return(1);
		}
	top=al+al;
	if (bn_wexpand(rr,top) == NULL) return(0);
	rr->top=top;
	rr->neg=0;

	if (al == 4)
		{
		bn_sqr_comba4(rr->d,a->d);
		goto end;
		}
	if (al == 8)
		{
		bn_sqr_comba8(rr->d,a->d);
		goto end;
		}
	if (al < 16)
		{
		bn_sqr_normal(rr->d,a->d,al,tmp);
		goto end;
		}

	nb=BN_num_bits_word((BN_ULONG)al);
	j=1<<(nb-1);
	k=j+j;
	if ((al == j) && !(a->flags & BN_FLG_STATIC_DATA))
		{
		if (bn_wexpand(t,k*2) == NULL) return(0);
		dtab.depth=nb-5;
		dtab.size=j;
		dtab.mul8=(void(*)())bn_mul_comba8;
		dtab.sqr8=(void(*)())bn_sqr_comba8;
		bn_sqr_rec_words(rr->d,a->d,t->d,&dtab);
		goto end;
		}
	if (bn_wexpand(t,top) == NULL) return(0);
	bn_sqr_normal(rr->d,a->d,al,t->d);
end:
	if (top>0 && rr->d[top-1]==0) rr->top--;
	if (rr != r) BN_copy(r,rr);
	return(1);
	}
