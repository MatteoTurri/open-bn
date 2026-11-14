/*
 * BER_ITEM_set_all: sets the header and the contents of a BER item.
 */
#include "bsafe.h"
#include "ber_item.h"

void BER_ITEM_set_all(BER_ITEM *item, int cls, int tag, void *data, int len, int flagsMid, int flagsExtra)
{
    BER_ITEM_set_header(item, cls, tag, flagsMid);
    BER_ITEM_set_data(item, data, len);
    item->state = item->state | flagsExtra;
}
