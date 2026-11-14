/*
 * The setters of the default DSA method table (default_dsa_method: new, free, size, setup, sign,
 * verify, p_dup). DSA_enable_default_method installs DSA_new_internal and DSA_free_internal and clears
 * the five hooks.
 */

#include <stdio.h>
#include "dsa.h"

int DSA_enable_default_method(void)
{
    default_dsa_method.new_fn = (void *)DSA_new_internal;
    default_dsa_method.free_fn = (void *)DSA_free_internal;
    default_dsa_method.size_cb = 0;
    default_dsa_method.setup_cb = 0;
    default_dsa_method.sign_cb = 0;
    default_dsa_method.verify_cb = 0;
    default_dsa_method.p_dup_cb = 0;
    return 1;
}

void DSA_set_default_method_size_cb(void *cb)
{
    default_dsa_method.size_cb = cb;
    return;
}

void DSA_set_default_method_sign_cb(void *cb)
{
    default_dsa_method.sign_cb = cb;
    return;
}

void DSA_set_default_method_setup_cb(void *cb)
{
    default_dsa_method.setup_cb = cb;
    return;
}

void DSA_set_default_method_verify_cb(void *cb)
{
    default_dsa_method.verify_cb = cb;
    return;
}

void DSA_set_default_method_p_dup_cb(void *cb)
{
    default_dsa_method.p_dup_cb = cb;
    return;
}
