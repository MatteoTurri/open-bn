/*
 * pk_rsa_get.
 */


#include "pk_rsa_impl.h"

/* The getInfoFn of the RSA method. 258: the method name of the modular exponentiation context `which`
 * (0 the modulus, N the CRT context of factor N, limited to primeCount), "-" when there is none;
 * 259: the length of the modulus in bytes; otherwise 2. */

#include "bn_me.h"


int pk_rsa_get(PK_CTX *ctxArg, int type, int which, void *out)
{
    RsaImpl *rsa;
    void *ptr;
    void *outPtr;

    rsa = ctxArg->impl;

    if (type == 258) {
        if (which == 0) {
            ptr = rsa->modExpCtx;
        } else {
            if (which > rsa->primeCount)
                which = rsa->primeCount;
            if (rsa->primeCtxs == 0) {
                ptr = 0;
            } else {
                ptr = rsa->primeCtxs[which - 1];
            }
        }

        if (ptr == 0 || ((BN_ME_CTX *)ptr)->meth == 0) {
            *(void **)out = "-";
        } else {
            *(void **)out = ((BN_ME_CTX *)ptr)->meth->name;
        }
        return 0;
    } else {
        if (type == 259) {
            outPtr = out;
            *(int *)outPtr = (BN_num_bits(&rsa->n) + 7) / 8;
            return 0;
        }
    }
    return 2;
}
