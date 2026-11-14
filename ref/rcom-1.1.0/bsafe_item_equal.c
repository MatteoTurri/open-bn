/*
 * B_ItemEqual: compares two byte strings.
 */

#include "bsafe.h"


int B_ItemEqual(ITEM *a, ITEM *b)
{
    int result = 0;
    if (a->len == b->len)
        result = (T_memcmp(a->data, b->data, a->len) == 0);
    return result;
}
