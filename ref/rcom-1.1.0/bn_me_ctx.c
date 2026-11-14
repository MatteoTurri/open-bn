/*
 * BN_ME_CTX_new, BN_ME_CTX_free and BN_ME_CTX_set: a context of a modular exponentiation method,
 * built and driven through the BN_ME_METH of the method.
 */

#include <stdio.h>
#include "bn.h"
#include "bn_me.h"


/* Builds a context with the constructFn of the method (the default one when methods is 0). */

void *BN_ME_CTX_new(BN_ME_METH *methods, void **ctxOut)
{
    int ret;
    void *dummy;

    if (ctxOut == 0)
        ctxOut = &dummy;
    if (methods == 0)
        methods = (*bn_mod_exp_meth_default)();
    ret = (*methods->constructFn)(methods, ctxOut);
    if (ret != 0) {
        return 0;
    } else {
        return *ctxOut;
    }
}


void BN_ME_CTX_free(BN_ME_CTX *ctxArg)
{
    (*ctxArg->meth->destroyFn)(ctxArg);
}

/* With mode 1, unless flags has 1, the method is first chosen again from the BN_bnme_* cache for
 * the size of mod; then setFn of the method. */

int BN_ME_CTX_set(BN_ME_CTX *ctxArg, void *mod, int mode, int flags, void *extra)
{
    void *(*getter)(void);

    if (mode == 1 && ctxArg->meth != 0 && !(flags & 1)) {
        getter = (void *(*)(void))BN_bnme_get(BN_num_bits(mod), 2);
        if (getter != 0) {
            ctxArg->meth = (BN_ME_METH *)(*getter)();
        } else {
            ctxArg->meth = (BN_ME_METH *)(*bn_mod_exp_meth_default)();
        }
    }
    return (*ctxArg->meth->setFn)(ctxArg, mod, mode, flags, extra);
}
