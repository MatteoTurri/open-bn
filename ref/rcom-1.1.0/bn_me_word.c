/*
 * BN_ME_METH_word: the Montgomery method on words and its context functions.
 */

#include <stdio.h>
#include "bn.h"
#include "bn_me.h"
#include "r_com.h"
#include <string.h>


static BN_ME_METH g_bn_me_meth_word = {
    0, "Word", bn_mont_ctx_useit, bn_mod_exp_word,
    bn_mont_ctx_new_word,
    bn_mont_ctx_free_word,
    bn_mont_ctx_set_word
};

BN_ME_METH *BN_ME_METH_word(void)
{
    return &g_bn_me_meth_word;
}

/* The context is one 112-byte allocation: the BN_ME_WORD header and, at +32, the BN_MONT_CTX that
 * mont points to. bn_mont_ctx_free_word also frees prog and extra. */


int bn_mont_ctx_new_word(BN_ME_METH *meth, void **out)
{
    BN_ME_WORD *ret;
    BN_MONT_CTX *mont;

    ret = (BN_ME_WORD *)R_malloc(112);
    if (ret == 0)
        return 1;
    memset(ret, 0, 32);
    ret->base.meth = meth;
    ret->mont = (BN_MONT_CTX *)((char *)ret + 32);
    mont = ret->mont;
    BN_MONT_CTX_init(mont);
    ret->prog = 0;
    ret->extra = 0;
    *out = ret;
    return 0;
}

int bn_mont_ctx_free_word(BN_ME_WORD *ctxArg)
{
    BN_MONT_CTX_free(ctxArg->mont);
    if (ctxArg->prog != 0)
        R_free(ctxArg->prog);
    if (ctxArg->extra != 0)
        R_free(ctxArg->extra);
    R_free(ctxArg);
    return 0;
}

/* The setFn of the word method. Mode 1 sets the Montgomery context from mod (BN_MONT_CTX_set_word)
 * and pads RR and N with zero words up to the nwords of the method; mode 3 stores the exponent
 * program of mod (BN_gen_exp_bits) in prog. Other modes return 1. In mode 3 the program pointer comes
 * back through nwords. */


int bn_mont_ctx_set_word(BN_ME_WORD *ctxArg, BIGNUM *mod, int mode, int flags,
    BN_CTX *bnctx)
{
	int r;
	BN_MONT_CTX *mont;
	int top;
	int nwords;
	int i;

	if (mode == 1) {
		mont = ctxArg->mont;
		r = BN_MONT_CTX_set_word(mont, mod, bnctx);
		top = mod->top;
		nwords = ctxArg->base.meth->nwords;

		if (nwords > top) {
			if ((&(mont->RR))->top < nwords) {
				if (nwords <= (&(mont->RR))->max)
					;
				else
					bn_expand2((&(mont->RR)), nwords); for (i = (&(mont->RR))->top; i < nwords; i++)
					(&(mont->RR))->d[i] = 0;
			}
			if ((&(mont->N))->top < nwords) {
				if (nwords <= (&(mont->N))->max)
					;
				else
					bn_expand2((&(mont->N)), nwords); for (i = (&(mont->N))->top; i < nwords; i++)
					(&(mont->N))->d[i] = 0;
			}
		}

		return r == 0;
	} else if (mode == 3) {
		i = BN_gen_exp_bits(mod, (unsigned char **)&nwords, flags, bnctx);
		if (i < 2)
			return 1;

		if (ctxArg->prog != 0)
			R_free(ctxArg->prog);

		if ((ctxArg->prog = R_malloc(i)) == 0)
			return 1;

		memcpy(ctxArg->prog, (void *)nwords, i);

		return 0;
	} else {
		return 1;
	}
}

