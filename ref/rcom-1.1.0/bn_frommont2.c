/*
 * BN_from_montgomery. RCOM uses a single BN_CTX temporary, takes the word count from mont->nwords
 * and adds the carry of each reduction step in two steps instead of rippling it.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

#define MONT_WORD

int BN_from_montgomery(ret,a,mont,ctx)
BIGNUM *ret;
BIGNUM *a;
BN_MONT_CTX *mont;
BN_CTX *ctx;
	{
	BIGNUM *n,*t1;
	BN_ULONG *ap,*np,*rp,n0,v;
	BN_ULONG c;
	BN_ULONG *bp;
	int al,nl,max,i;
	int retn=0;

	t1=&(ctx->bn[ctx->tos]);
	if (!BN_copy(t1,a)) goto err;
	n=&(mont->N);

	ap=a->d;
	al=mont->nwords;

	nl=n->top;
	if ((al == 0) || (nl == 0)) { t1->top=0; return(1); }

	max=(nl+al+1);
	if (bn_wexpand(t1,max) == NULL) goto err;
	if (bn_wexpand(ret,max) == NULL) goto err;

	t1->neg=a->neg^n->neg;
	np=n->d;
	rp=t1->d;
	bp=&(t1->d[nl]);

	for (i=t1->top; i<max; i++)
		t1->d[i]=0;

	t1->top=max;
	n0=mont->n0;

	c=0;
	for (i=0; i<nl; i++)
		{
		v=bn_mul_add_words(rp,np,nl,(rp[0]*n0));
		v+=c;
		c=(v<c);
		if ((*bp+=v) < v) c++;
		bp++;
		rp++;
		}
	*bp=c;

	bn_fix_top(t1);

	BN_rshift(ret,t1,mont->nwords*BN_BITS2);

	if (BN_ucmp(ret,&(mont->N)) >= 0)
		{
		BN_usub(ret,ret,&(mont->N));
		}
	retn=1;
err:
	return(retn);
	}
