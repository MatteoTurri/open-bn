/*
 * BER_ITEM_set_long: encodes a long as the contents of a BER INTEGER item.
 */

#include <string.h>

#include "bsafe.h"
#include "ber_item.h"

#include "r_com.h"


int BER_ITEM_set_long(BER_ITEM *item, int value, void *outBufArg)
{
    int isNeg;
    int nbytes;
    int i;
    unsigned int absval;
    int allocated;
    unsigned char *p;
    static const unsigned char g_berZero[1] = {0};

    item->state = item->state & 0xfb;

    if (value == 0) {
        BER_ITEM_set_all(item, 0, 2, (void *)g_berZero, 1, 0, 0);
        return 0;
    }

    if (value < 0) {
        absval = (unsigned int)(-value);
        isNeg = 1;
    } else {
        absval = (unsigned int)value;
        isNeg = 0;
    }

    allocated = 0;
    if (outBufArg == 0) {
        if ((outBufArg = (char *)R_malloc(4)) == 0)
            return 2;
        else
            allocated = 1;
    }
    memset(outBufArg, 0, 4);

    p = (unsigned char *)outBufArg + 3;
    nbytes = 1;
    for (;;) {
        *--p = (unsigned char)absval;
        if (absval < 128)
            break;
        absval = absval >> 8;
        nbytes++;
    }

    for (i = 0; i < nbytes; i++)
        ((unsigned char *)outBufArg)[i] = p[i];

    if (isNeg)
        ((unsigned char *)outBufArg)[0] |= 0x80;

    BER_ITEM_set_all(item, 0, 2, outBufArg, nbytes, 0, allocated);
    return 0;
}
