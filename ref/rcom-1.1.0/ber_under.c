/*
 * BER_ITEMS_under: links an item as the last child of another.
 */

#include <string.h>

#include "bsafe.h"
#include "ber_item.h"


int BER_ITEMS_under(BER_ITEM *x, BER_ITEM *y)
{
    if (!(x->flags & 0x20)) {
        if (!(x->flags & 0x4))
            return 4;
    }
    y->parent = x;
    y->next = x->child;
    x->child = y;
    x->state = x->state | 8;
    return 0;
}
