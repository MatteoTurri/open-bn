/*
 * bn_mod_exp_meth_default: the getter of the method a BN_ME_CTX gets when it is created without one
 * (BN_ME_METH_word).
 */

#include "bn_me.h"

BN_ME_METH *(*bn_mod_exp_meth_default)(void) = BN_ME_METH_word;
