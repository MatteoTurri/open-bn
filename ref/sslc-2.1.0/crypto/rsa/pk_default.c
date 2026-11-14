/*
 * DSA_new, DSA_size, DSA_sign_setup, DSA_sign, DSA_verify, DSA_free and DSA_params_dup of the
 * SSL-C fork, on default_dsa_method.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "dsa.h"

/* dsa_default_method: new_fn, free_fn, size_cb, setup_cb, sign_cb, verify_cb, p_dup_cb;
 * DSA_new_internal and DSA_free_internal by default (DSA_enable_default_method). */

DSA_DEFAULT_METHOD default_dsa_method = { 0 };


DSA *DSA_new(void)
{
    if (default_dsa_method.new_fn == 0) {
        ERR_put_error(10, 104, 14, __FILE__, 28);
        return 0;
    } else {
        return (*(DSA *(*)(void))default_dsa_method.new_fn)();
    }
}

int DSA_size(DSA *dsa)
{
    if (default_dsa_method.size_cb == 0) {
        ERR_put_error(10, 107, 14, __FILE__, 40);
        return -1;
    } else {
        return (*(int (*)(void *))default_dsa_method.size_cb)(dsa);
    }
}

int DSA_sign_setup(DSA *dsa, BN_CTX *ctx_in, BIGNUM **kinvp, BIGNUM **rp)
{
    if (default_dsa_method.setup_cb == 0) {
        ERR_put_error(10, 106, 14, __FILE__, 56);
        return -1;
    } else {
        return (*(int (*)(DSA *, BN_CTX *, BIGNUM **, BIGNUM **))default_dsa_method.setup_cb)
               (dsa, ctx_in, kinvp, rp);
    }
}

int DSA_sign(int type, unsigned char *dgst, int dlen, unsigned char *sig,
             unsigned int *siglen, DSA *dsa)
{
    if (default_dsa_method.sign_cb == 0) {
        ERR_put_error(10, 105, 14, __FILE__, 73);
        return -1;
    } else {
        return (*(int (*)(int, unsigned char *, int, unsigned char *,
                           unsigned int *, void *))default_dsa_method.sign_cb)
               (type, dgst, dlen, sig, siglen, dsa);
    }
}

int DSA_verify(int type, unsigned char *dgst, int dgst_len, unsigned char *sigbuf,
               int siglen, DSA *dsa)
{
    if (default_dsa_method.verify_cb == 0) {
        ERR_put_error(10, 108, 14, __FILE__, 90);
        return -1;
    } else {
        return (*(int (*)(int, unsigned char *, int, unsigned char *,
                           int, void *))default_dsa_method.verify_cb)
               (type, dgst, dgst_len, sigbuf, siglen, dsa);
    }
}

void DSA_free(DSA *r)
{
    if (default_dsa_method.free_fn == 0) {
        ERR_put_error(10, 103, 14, __FILE__, 103);
        return;
    } else {
        (*(void (*)(void *))default_dsa_method.free_fn)(r);
    }
    return;
}

DSA *DSA_params_dup(DSA *dsa)
{
    if (default_dsa_method.p_dup_cb == 0) {
        ERR_put_error(10, 109, 14, __FILE__, 116);
        return 0;
    } else {
        return (*(DSA *(*)(void *))default_dsa_method.p_dup_cb)(dsa);
    }
}
