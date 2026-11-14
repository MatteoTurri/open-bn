/*
 * BN_mod_exp2_mont: a1^p1 * a2^p2 mod m at once, with 2-bit windows on both exponents and a 4 x 4
 * table val[i][j] = a1^i * a2^j in Montgomery form.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

int BN_mod_exp2_mont(rr, a1, p1, a2, p2, m, ctx, in_mont)
BIGNUM *rr;
BIGNUM *a1;
BIGNUM *p1;
BIGNUM *a2;
BIGNUM *p2;
BIGNUM *m;
BN_CTX *ctx;
BN_MONT_CTX *in_mont;
	{
	int i;
	int j;
	int window;
	int bits;
	int bits1;
	int bits2;
	int ret;
	int wstart;
	int wvalue;
	int wbits;
	int wvalue1;
	int wvalue2;
	int start;
	int ts;
	int ii;
	int jj;
	BIGNUM *d;
	BIGNUM *aa1;
	BIGNUM *aa2;
	BIGNUM *r;
	BIGNUM val[4][4];
	BN_MONT_CTX *mont;

	ret=0;
	start=1;
	ts=0;
	mont=NULL;
	if (!(m->d[0] & 1)) return(0);
	d= &(ctx->bn[ctx->tos++]);
	r= &(ctx->bn[ctx->tos++]);
	bits1=BN_num_bits(p1);
	bits2=BN_num_bits(p2);
	if ((bits1 == 0) && (bits2 == 0))
		{
		BN_set_word(r,1);
		return(1);
		}
	bits=(bits1 > bits2)?bits1:bits2;

	if (in_mont != NULL)
		mont=in_mont;
	else
		{
		mont=BN_MONT_CTX_new();
		if (mont == NULL) goto err;
		if (!BN_MONT_CTX_set_word(mont,m,ctx)) goto err;
		}

	BN_init(&(val[0][0]));
	BN_init(&(val[1][1]));
	BN_init(&(val[0][1]));
	BN_init(&(val[1][0]));
	ts=1;
	if (BN_ucmp(a1,m) >= 0)
		{
		BN_mod(&(val[1][0]),a1,m,ctx);
		aa1= &(val[1][0]);
		}
	else
		aa1=a1;
	if (BN_ucmp(a2,m) >= 0)
		{
		BN_mod(&(val[0][1]),a2,m,ctx);
		aa2= &(val[0][1]);
		}
	else
		aa2=a2;
	if (!BN_mod_mul_montgomery(&(val[1][0]),aa1,&(mont->RR),mont,ctx)) goto err;
	if (!BN_mod_mul_montgomery(&(val[0][1]),aa2,&(mont->RR),mont,ctx)) goto err;
	if (!BN_mod_mul_montgomery(&(val[1][1]),&(val[1][0]),&(val[0][1]),mont,ctx))
		goto err;

	wbits=2;
	window=1<<wbits;
	for (ii=0; ii<window; ii++)
		{
		if (ii >= 2)
			{
			BN_init(&(val[ii][0]));
			BN_init(&(val[ii][1]));
			if (!BN_mod_mul_montgomery(&(val[ii][0]),&(val[1][0]),
						   &(val[ii-1][0]),mont,ctx)) goto err;
			if (!BN_mod_mul_montgomery(&(val[ii][1]),&(val[1][0]),
						   &(val[ii-1][1]),mont,ctx)) goto err;
			}
		jj=2;
		while (jj < window)
			{
			BN_init(&(val[ii][jj]));
			if (!BN_mod_mul_montgomery(&(val[ii][jj]),&(val[ii][jj-1]),
						   &(val[0][1]),mont,ctx)) goto err;
			jj++;
			}
		}
	ts=window;

	start=1;
	wvalue1=0;
	wvalue2=0;
	wstart=bits-1;
	wvalue=0;
	if (!BN_mod_mul_montgomery(r,BN_value_one(),&(mont->RR),mont,ctx)) goto err;
	for (;;)
		{
		wvalue1=BN_is_bit_set(p1,wstart);
		wvalue2=BN_is_bit_set(p2,wstart);
		if ((wvalue1 == 0) && (wvalue2 == 0))
			{
			if (!start)
				{
				if (!BN_mod_mul_montgomery(r,r,r,mont,ctx)) goto err;
				}
			wstart--;
			if (wstart < 0) break;
			continue;
			}
		j=wstart;
		wvalue=0;
		for (i=1; i<wbits; i++)
			{
			if (wstart-i < 0) break;
			wvalue1+=wvalue1;
			wvalue1|=BN_is_bit_set(p1,wstart-i);
			wvalue2+=wvalue2;
			wvalue2|=BN_is_bit_set(p2,wstart-i);
			}
		if (!start)
			for (j=0; j<i; j++)
				{
				if (!BN_mod_mul_montgomery(r,r,r,mont,ctx)) goto err;
				}
		if (!((wvalue1 == 0) && (wvalue2 == 0)))
			if (!BN_mod_mul_montgomery(r,r,&(val[wvalue1][wvalue2]),mont,ctx))
				goto err;
		wstart-=i;
		start=0;
		if (wstart < 0) break;
		}
	BN_from_montgomery(rr,r,mont,ctx);
	ret=1;
err:
	if ((in_mont == NULL) && (mont != NULL)) BN_MONT_CTX_free(mont);
	ctx->tos-=2;
	for (i=0; i<ts; i++)
		{
		j=0;
		while (j < ts)
			{
			BN_clear_free(&(val[i][j]));
			j++;
			}
		}
	return(ret);
	}
