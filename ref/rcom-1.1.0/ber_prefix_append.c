/*
 * BER_ITEM_set_prefix_byte and BER_ITEMS_append.
 */

#include <string.h>

#include "bsafe.h"
#include "ber_item.h"


void BER_ITEM_set_prefix_byte(BER_ITEM *item, unsigned int prefixByte)
{
    if (!(item->state & 0x4)) {
        item->state = item->state | 0x4;
        item->len++;
    }
    item->prefix = (unsigned char)prefixByte;
}


void BER_ITEMS_append(BER_ITEM *x, BER_ITEM *y)
{
    y->parent = x->parent;
    if (y->parent != 0)
        y->parent->state = y->parent->state | 8;
    y->next = x->next;
    x->next = y;
}
