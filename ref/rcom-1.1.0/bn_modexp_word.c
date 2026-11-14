/*
 * bn_mod_exp_word: the modular exponentiation of the word method, run on word arrays. The exponent
 * program (BN_gen_exp_bits) is a 4-byte header, whose byte 3 is the size of the window table, then
 * (idx, nsq) byte pairs: nsq squarings, then a multiplication by val[idx >> 1]; (0, 0) ends it and
 * (1, 0) multiplies by the base into the result. BN_CTX.surrender, when set, is polled with code 0xFF
 * and a counter after each multiplication, and with -1 at the end.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"
#include "bn_me.h"

/* The object BN_CTX.surrender points to: a table with one method. */
typedef struct bn_op_st {
    int (*poll)();
} BN_OP;


int bn_mod_exp_word(mectx, r, a, p, ctx)
BN_ME_WORD *mectx;
BIGNUM *r;
BIGNUM *a;
BIGNUM *p;
BN_CTX *ctx;
	{
	int i;
	int ret;
	int tos;
	int nl;
	int nlsave;
	int idx;
	int nsq;
	int wsize;
	BIGNUM *aa;
	BIGNUM *tmp;
	BIGNUM *Np;
	BN_ULONG *val[16];
	BN_ULONG *t1;
	BN_ULONG *acc;
	BN_ULONG n0;
	BN_ULONG *rr;
	BN_ULONG *prod;
	BN_ULONG *np;
	BN_MONT_CTX *mont;
	unsigned char *prog;
	BN_OP *op;
	int pollcnt;

	ret=0;
	op=(BN_OP *)ctx->surrender;
	pollcnt=0;
	mont=mectx->mont;
	Np= &(mont->N);
	tos=ctx->tos;
	nlsave=nl=mont->N.top;

	if ((a->top == nl) && (*(a->d+nl-1) < *(Np->d+nl-1)))
		aa=a;
	else if (a->top >= nl)
		{
		aa= &(ctx->bn[ctx->tos++]);
		if (!BN_mod(aa,a,Np,ctx)) goto err;
		}
	else
		{
		aa= &(ctx->bn[ctx->tos++]);
		BN_copy(aa,a);
		}

	if (aa->top < nl)
		{
		int j;

		bn_wexpand(aa,nl);
		for (j=aa->top; j<nl; j++)
			aa->d[j]=0;
		}

	if (p != NULL)
		{
		i=BN_gen_exp_bits(p,&prog,0,ctx);
		if (i < 2) goto err;
		ctx->tos++;
		}
	else if (mectx->prog != NULL)
		prog=mectx->prog;
	else
		return(1);

	wsize=prog[3];
	prog+=4;
	tmp= &(ctx->bn[ctx->tos++]);
	bn_wexpand(r,nl);
	bn_wexpand(tmp,(wsize+6)*nl);
	prod=tmp->d;
	t1=tmp->d+nl*2;
	acc=tmp->d+nl*4;
	val[0]=tmp->d+nl*6;
	rr=mont->RR.d;
	np=mont->N.d;
	n0=mont->n0;

	bn_mul_normal(prod,aa->d,nl,rr,nl);
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
	for (;;)
		{
		if (nsq == 0) break;
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
		if ((nsq == 0) && (idx == 1)) goto lastmul;
		bn_mul_normal(prod,acc,nl,val[idx>>1],nl);
		bn_from_montgomery_words(acc,prod,np,nl,n0);
		if (op != NULL)
			{
			ret=(*op->poll)(op,0xFF,pollcnt++);
			if (ret != 0) goto err;
			}
		continue;
lastmul:
		bn_mul_normal(prod,acc,nl,aa->d,nl);
		bn_from_montgomery_words(r->d,prod,np,nl,n0);
		goto done;
		}
	i=nl;
	while (i < nl+nl)
		{
		acc[i]=0;
		i++;
		}
	bn_from_montgomery_words(r->d,acc,np,nl,n0);
done:
	r->top=nl;
	if (r->top > 0)
		{
		BN_ULONG *q;

		q=r->d+(r->top-1);
		while (r->top > 0)
			{
			if (*q-- != 0) break;
			r->top--;
			}
		}
	ret=0;
err:
	if (op != NULL)
		{
		ret=(*op->poll)(op,0xFF,-1);
		if (ret != 0) goto err;
		}
	ctx->tos=tos;
	return(ret);
	}
