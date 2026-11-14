/*
 * The PK layer of RCOM: a public key context (PK_CTX) driven through a table of methods (PK_METH).
 * The operations share opFn with an opcode: 1 private encrypt, 3 public encrypt, 4 public decrypt,
 * 6 phase 2, 10 sign.
 */


#include "pk_int.h"

#include "r_eitem.h"
#include "r_eitems.h"

void PK_CTX_free(PK_CTX *ctxArg)
{
    if (ctxArg->methods != 0) {
        (*ctxArg->methods->destroyFn)(ctxArg);
    }
    if (ctxArg->eitems != 0) {
        R_EITEMS_free(ctxArg->eitems);
    }
    if (ctxArg->flags & 1) {
        R_free(ctxArg);
    }
}

int PK_CTX_set(PK_CTX *ctxArg, PK_METH *methods, void *eitems, void *unused4, void *unused5)
{
    ctxArg->methods = methods;
    return (*methods->initFn)(ctxArg, eitems, unused4, unused5);
}

int PK_phase1(PK_CTX *ctxArg, void *flags)
{
    return (*ctxArg->methods->phase1Fn)(ctxArg, 5, 0, flags);
}

int PK_phase2(PK_CTX *ctxArg, unsigned char *out, int *outLen, unsigned int outMax, unsigned char *in,
              unsigned int inLen, void *unused)
{
    return (*ctxArg->methods->opFn)(ctxArg, 6, out, outLen, outMax, in, inLen, unused);
}
