/*
 * BER_ITEM_cmp_tag: checks the tag, the class and the constructed bit of a BER item (7 on a
 * mismatch).
 */
#include "bsafe.h"
#include "ber_item.h"

int BER_ITEM_cmp_tag(BER_ITEM *item, int wantedTag)
{
    int constructedExpected;
    int constructedActual;

    if (item->tag != wantedTag)
        return 7;
    if (item->flags & 0xc0)
        return 7;
    constructedExpected = (item->tag == 16 || item->tag == 17) ? 1 : 0;
    constructedActual = ((item->flags & 0x20) != 0);
    return (constructedExpected != constructedActual) ? 7 : 0;
}
