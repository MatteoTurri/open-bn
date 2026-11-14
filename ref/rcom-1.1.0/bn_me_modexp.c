/*
 * BN_ME_CTX_mod_exp: r = a^p on the modulus of the context, through the opFn of its method.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

#include "bn_me.h"

int BN_ME_CTX_mod_exp(mectx,r,a,p,unused5,ctx)
BN_ME_CTX *mectx;
BIGNUM *r;
BIGNUM *a;
BIGNUM *p;
void *unused5;
BN_CTX *ctx;
	{
	int n;
	int i;

	if (a->top == 0 || (a->top == 1 && a->d[0] == 0)) {
		BN_set_word(r,0);
		return(0);
	}
	if (p != 0) {
		if (p->top == 0 || (p->top == 1 && p->d[0] == 0)) {
			BN_set_word(r,1);
			return(0);
		} else if (p->top == 1 && p->d[0] == 1) {
			BN_copy(r,a);
			return(0);
		}
	}
	n=mectx->meth->nwords;

	if (a->top < n) {
		bn_wexpand(a,n); for (i=a->top; i<n; i++)
			a->d[i]=0;
	}

	return((*mectx->meth->opFn)(mectx,r,a,p,ctx));
	}
