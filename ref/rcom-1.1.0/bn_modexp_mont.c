/*
 * BN_mod_exp_mont of RCOM: not the SSLeay one but the word-level algorithm of bn_mod_exp_word (an
 * exponent program from BN_gen_exp_bits run with bn_mul_normal, bn_sqr_normal and
 * bn_from_montgomery_words), with its own Montgomery context and the trivial cases a == 0, p == 0,
 * p == 1.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

#include "bn_me.h"

int BN_mod_exp_mont(rr, a, p, m, ctx, in_mont)
BIGNUM *rr;
BIGNUM *a;
BIGNUM *p;
BIGNUM *m;
BN_CTX *ctx;
BN_MONT_CTX *in_mont;
	{
	int i;
	int j;
	int wnl;
	int pbits;
	int ret;
	int tos;
	int nl;
	int idx;
	int nsq;
	int wsize;
	BIGNUM *t1bn;
	BIGNUM *valbn;
	BIGNUM *accbn;
	BIGNUM *aa;
	BIGNUM *prodbn;
	BN_ULONG *val[16];
	BN_ULONG *t1;
	BN_ULONG *acc;
	BN_ULONG n0;
	BN_ULONG *rrd;
	BN_ULONG *prod;
	BN_ULONG *np;
	BN_MONT_CTX *mont;
	unsigned char *prog;

	ret=0;
	mont=NULL;
	prog=NULL;
	if (!(m->d[0] & 1)) return(0);
	tos=ctx->tos;

	if ((a->top == 0) || ((a->top == 1) && (a->d[0] == 0)))
		{
		BN_set_word(rr,0);
		return(1);
		}
	if ((p->top == 0) || ((p->top == 1) && (p->d[0] == 0)))
		{
		BN_set_word(rr,1);
		return(1);
		}
	if ((p->top == 1) && (p->d[0] == 1))
		{
		BN_copy(rr,a);
		return(1);
		}

	if (in_mont != NULL)
		mont=in_mont;
	else
		{
		mont=BN_MONT_CTX_new();
		if (mont == NULL) goto err;
		if (!BN_MONT_CTX_set_word(mont,m,ctx)) goto err;
		}

	BN_gen_exp_bits(p,&prog,0,ctx);
	ctx->tos++;
	wsize=prog[3];
	i=(prog[2]+63)/prog[2];
	prog+=4;
	nl=mont->N.top;

	if ((a->top == nl) && (*(a->d+nl-1) < *(m->d+nl-1)))
		aa=a;
	else if (a->top < nl)
		{
		aa= &(ctx->bn[ctx->tos++]);
		aa->top=a->top;
		if (aa->top < nl)
			{
			int k;

			bn_wexpand(aa,nl);
			for (k=aa->top; k<nl; k++)
				aa->d[k]=0;
			}
		j=0;
		while (j < a->top)
			{
			aa->d[j]=a->d[j];
			j++;
			}
		}
	else
		{
		aa= &(ctx->bn[ctx->tos++]);
		if (!BN_mod(aa,a,m,ctx)) goto err;
		if (aa->top < nl)
			{
			int k;

			bn_wexpand(aa,nl);
			for (k=aa->top; k<nl; k++)
				aa->d[k]=0;
			}
		}

	pbits=p->top*32;
	wnl=wsize*nl;
	j=wnl+((p->top*i+3)/2);
	valbn= &(ctx->bn[ctx->tos++]);
	t1bn= &(ctx->bn[ctx->tos++]);
	accbn= &(ctx->bn[ctx->tos++]);
	prodbn= &(ctx->bn[ctx->tos++]);
	bn_wexpand(rr,nl);
	bn_wexpand(t1bn,nl*4);
	bn_wexpand(accbn,nl*2);
	bn_wexpand(valbn,j);
	bn_wexpand(prodbn,nl*2);
	t1=t1bn->d;
	acc=accbn->d;
	prod=prodbn->d;
	rrd=mont->RR.d;
	n0=mont->n0;
	np=mont->N.d;
	val[0]=valbn->d;

	bn_mul_normal(prod,aa->d,nl,rrd,nl);
	bn_from_montgomery_words(val[0],prod,np,nl,n0);
	if (wsize >= 2)
		{
		bn_sqr_normal(prod,val[0],nl,t1);
		bn_from_montgomery_words(t1,prod,np,nl,n0);
		for (i=1; i<wsize; i++)
			{
			val[i]=val[i-1]+nl;
			bn_mul_normal(prod,val[i-1],nl,t1,nl);
			bn_from_montgomery_words(val[i],prod,np,nl,n0);
			}
		}

	idx= *prog++;
	nsq= *prog++;
	memcpy(acc,val[idx>>1],nl*4);
	while (nsq != 0)
		{
		i=0;
		while (i < nsq)
			{
			bn_sqr_normal(prod,acc,nl,t1);
			bn_from_montgomery_words(acc,prod,np,nl,n0);
			i++;
			}
		idx= *prog++;
		nsq= *prog++;
		if ((idx == 0) && (nsq == 0)) break;
		if (!((nsq == 0) && (idx == 1)))
			{
			bn_mul_normal(prod,acc,nl,val[idx>>1],nl);
			bn_from_montgomery_words(acc,prod,np,nl,n0);
			}
		else
			{
			bn_mul_normal(prod,acc,nl,aa->d,nl);
			bn_from_montgomery_words(rr->d,prod,np,nl,n0);
			goto done;
			}
		if (ctx->flags & 0x4000) goto err;
		}
	i=nl;
	while (i < nl+nl)
		{
		acc[i]=0;
		i++;
		}
	bn_from_montgomery_words(rr->d,acc,np,nl,n0);
done:
	rr->top=nl;
	if (rr->top > 0)
		{
		BN_ULONG *q;

		q=rr->d+(rr->top-1);
		while (rr->top > 0)
			{
			if (*q-- != 0) break;
			rr->top--;
			}
		}
	ret=1;
err:
	if ((in_mont == NULL) && (mont != NULL)) BN_MONT_CTX_free(mont);
	ctx->tos=tos;
	return(ret);
	}
