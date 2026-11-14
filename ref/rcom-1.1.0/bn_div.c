/*
 * BN_div: the fast division of SSLeay, with the BIGNUMs of the BN_CTX taken by address (RCOM holds
 * them by value), BN_usub for bn_qsub and bn_div_words for bn_div64.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"


int BN_div(dv, rm, num, divisor, ctx)
BIGNUM *dv;
BIGNUM *rm;
BIGNUM *num;
BIGNUM *divisor;
BN_CTX *ctx;
	{
	int norm_shift,i,j,loop;
	BIGNUM *tmp,wnum,*snum,*sdiv,*res;
	BN_ULONG *resp,*wnump;
	BN_ULONG d0,d1;
	int num_n,div_n;

	if ((divisor->top == 0) || (divisor->top == 1 && divisor->d[0] == 0))
		{
		return(0);
		}

	if (BN_ucmp(num,divisor) < 0)
		{
		if (rm != NULL)
			{ if (BN_copy(rm,num) == NULL) return(0); }
		if (dv != NULL) BN_zero(dv);
		return(1);
		}

	tmp=&(ctx->bn[ctx->tos]);
	tmp->neg=0;
	snum=&(ctx->bn[ctx->tos+1]);
	sdiv=&(ctx->bn[ctx->tos+2]);
	if (dv == NULL)
		res=&(ctx->bn[ctx->tos+3]);
	else	res=dv;

	/* First we normalise the numbers */
	norm_shift=BN_BITS2-((BN_num_bits(divisor))%BN_BITS2);
	BN_lshift(sdiv,divisor,norm_shift);
	sdiv->neg=0;
	norm_shift+=BN_BITS2;
	BN_lshift(snum,num,norm_shift);
	snum->neg=0;
	div_n=sdiv->top;
	num_n=snum->top;
	loop=num_n-div_n;

	/* Lets setup a 'window' into snum
	 * This is the part that corresponds to the current
	 * 'area' being divided */
	BN_init(&wnum);
	wnum.d=	 &(snum->d[loop]);
	wnum.top= div_n;
	wnum.max= snum->max+1; /* a bit of a lie */

	/* Get the top 2 words of sdiv */
	/* i=sdiv->top; */
	d0=sdiv->d[div_n-1];
	d1=(div_n == 1)?0:sdiv->d[div_n-2];

	/* pointer to the 'top' of snum */
	wnump= &(snum->d[num_n-1]);

	/* Setup to 'res' */
	if (!bn_wexpand(res,(loop+1))) goto err;
	res->neg= (num->neg^divisor->neg);
	res->top=loop;
	resp= &(res->d[loop-1]);

	/* space for temp */
	if (!bn_wexpand(tmp,(div_n+1))) goto err;

	if (BN_ucmp(&wnum,sdiv) >= 0)
		{
		if (!BN_usub(&wnum,&wnum,sdiv)) goto err;
		*resp=1;
		res->d[res->top-1]=1;
		}
	else
		res->top--;
	resp--;

	for (i=0; i<loop-1; i++)
		{
		BN_ULONG q,n0,n1;
		BN_ULONG l0;

		wnum.d--; wnum.top++;
		n0=wnump[0];
		n1=wnump[-1];
		if (n0 == d0)
			q=BN_MASK2;
		else
			q=bn_div_words(n0,n1,d0);
		{
		BN_ULLONG t1,t2,rem;
		t1=((BN_ULLONG)n0<<BN_BITS2)|n1;
		for (;;)
			{
			t2=(BN_ULLONG)d1*q;
			rem=t1-(BN_ULLONG)q*d0;
			if ((rem>>BN_BITS2) ||
				(t2 <= ((BN_ULLONG)(rem<<BN_BITS2)+wnump[-2])))
				break;
			q--;
			}
		}
		l0=bn_mul_words(tmp->d,sdiv->d,div_n,q);
		tmp->d[div_n]=l0;
		for (j=div_n+1; j>0; j--)
			if (tmp->d[j-1]) break;
		tmp->top=j;

		j=wnum.top;
		BN_sub(&wnum,&wnum,tmp);

		snum->top=snum->top+wnum.top-j;

		if (wnum.neg)
			{
			q--;
			j=wnum.top;
			BN_add(&wnum,&wnum,sdiv);
			snum->top+=wnum.top-j;
			}
		*(resp--)=q;
		wnump--;
		}
	bn_fix_top(snum);
	if (rm != NULL)
		{
		i=num->neg;
		BN_rshift(rm,snum,norm_shift);
		rm->neg=i;
		}
	return(1);
err:
	return(0);
	}
