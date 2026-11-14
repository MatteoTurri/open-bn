/*
 * BER_ITEM_get_long: reads a BER INTEGER item into a long.
 */

#include <string.h>

#include "bsafe.h"
#include "ber_item.h"


int BER_ITEM_get_long(BER_ITEM *item, int *outVal)
{
    int status;
    unsigned int len;
    unsigned int i;
    unsigned char *ptr;
    unsigned char curByte;
    int accum;

    status = BER_ITEM_cmp_tag(item, 2);
    if (status != 0)
        return status;

    /* i is first the flag of the cached prefix byte, then the loop counter. */
    i = ((item->state & 4) != 0);
    len = item->len;
    if (len == 0) {
        *outVal = 0;
        return 0;
    }
    /* 5 or more content bytes do not fit in a long: error 9. */
    if (len >= 5)
        return 9;

    ptr = item->data;
    if (i)
        curByte = item->prefix;
    else
        curByte = *ptr++;

    if (curByte & 0x80)
        accum = -256;
    else
        accum = 0;
    i = 0;
    for (;;) {
        accum = accum | curByte;
        if (!((i = i + 1) < len))
            break;
        accum = accum << 8;
        curByte = *ptr++;
    }
    *outVal = accum;
    return 0;
}
