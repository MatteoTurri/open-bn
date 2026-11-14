/*
 * BN_MONT_CTX_set_word: BN_MONT_CTX_set for the word method. The inverse is BN_mod_inverse_word on
 * the low word, ri is 1, nwords is the word count of the modulus, RR is 2^(nwords * 64) mod N, and RR
 * is padded with zero words up to nwords without changing its top.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

int BN_MONT_CTX_set_word(mont,mod,ctx)
BN_MONT_CTX *mont;
BIGNUM *mod;
BN_CTX *ctx;
	{
	BIGNUM tmp;
	BIGNUM *R;
	BN_ULONG buf0;
	BN_ULONG ninv;
	int i;

	if (mod->top == 0)
		return(0);

	BN_init(&tmp);

	R=&(mont->RR);				/* grab RR as a temp */
	BN_copy(&(mont->N),mod);		/* Set N */

	mont->ri=1;
	mont->nwords=(BN_num_bits(mod)+31)/32;

	BN_set_word(R,0);
	BN_set_bit(R,32);			/* R */

	buf0=mod->d[0];

	ninv=BN_mod_inverse_word(buf0);	/* Ri */
	BN_set_word(&tmp,ninv);
	BN_lshift(&tmp,&tmp,32);		/* R*Ri */

	if (tmp.top != 0 && !(tmp.top == 1 && tmp.d[0] == 0))
		BN_sub_word(&tmp,1);		/* R*Ri - 1 */
	else
		BN_set_word(&tmp,(BN_ULONG)-1);

	mont->n0=bn_div_words((tmp.top < 2) ? 0 : tmp.d[1],
	                      (tmp.top <= 0) ? 0 : tmp.d[0],
	                      buf0);
	BN_free(&tmp);

	/* setup RR for conversions */
	BN_set_word(&(mont->RR),0);
	BN_set_bit(&(mont->RR),mont->nwords*64);
	BN_mod(&(mont->RR),&(mont->RR),&(mont->N),ctx);

	if ((&(mont->RR))->top < mont->nwords) {
		if (mont->nwords <= (&(mont->RR))->max)
			;
		else
			bn_expand2(&(mont->RR),mont->nwords); for (i=(&(mont->RR))->top; i<mont->nwords; i++)
			(&(mont->RR))->d[i]=0;
	}

	return(1);
	}
