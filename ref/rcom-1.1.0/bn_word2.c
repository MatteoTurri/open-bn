/*
 * bn_div_words: divides the double word h:l by d.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

BN_ULONG bn_div_words(BN_ULONG h, BN_ULONG l, BN_ULONG d)
{
    return (BN_ULONG)(((BN_ULLONG)h << 32 | l) / d);
}
