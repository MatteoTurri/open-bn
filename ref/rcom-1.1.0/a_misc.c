/*
 * A_IntegerBits: a helper of the BSAFE algae layer (A_*).
 */

#include "alg.h"

/* Returns the number of significant bits of a big-endian integer of `len`
 * bytes: skips the leading zero bytes, then finds the highest bit of the
 * first nonzero byte. 0 if every byte is zero. */
unsigned int A_IntegerBits(unsigned char *value, unsigned int len)
{
    unsigned int i;
    unsigned int bits;
    unsigned int mask;
    unsigned char b;

    for (i = 0; i < len; i++)
        if (value[i] != 0)
            break;
    if (i == len)
        return 0;

    b = value[i];
    i++;
    bits = 8;
    for (mask = 128; (b & mask) == 0; mask >>= 1)
        bits--;

    return (len - i) * 8 + bits;
}
