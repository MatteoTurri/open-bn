/*
 * CMP_AddInPlace.
 */

#include "cmp.h"
#include "bsafe.h"


/* b += a: b is first grown with zero words to the length of a, then added word by word with the
 * carry, which ripples through the rest of b and, past the top, becomes a new word 1. `i` is the
 * grow target, the loop index and the new length. */
int CMP_AddInPlace(CMPInt *a, CMPInt *b)
{
    int alen;
    int i;
    unsigned long carry;
    unsigned long sum;
    unsigned long *bv;
    unsigned long *av;

    if (b->length < a->length) {
        int oldlen;

        alen = a->length;
        i = alen;
        if (b->allocated < i) {
            int rc = CMP_realloc(i + 1, b);
            if (rc != 0)
                return rc;
        }
        oldlen = b->length;
        T_memset(b->value + oldlen, 0, (alen - oldlen) * 4);
        b->length = alen;
    }

    carry = 0;
    i = 0;
    bv = b->value;
    av = a->value;
    if (a->length > 0) {
        do {
            sum = bv[0] + av[0];
            bv[0] = sum + carry;
            if (sum < av[0] || bv[0] < sum)
                carry = 1;
            else
                carry = 0;
            i++;
            bv++;
            av++;
        } while (i < a->length);
    }
    if (carry == 0)
        return 0;

    while (i < b->length) {
        bv[0] = bv[0] + carry;
        carry = (bv[0] == 0);
        if (carry == 0)
            break;
        i++;
        bv++;
    }
    if (carry == 0)
        return 0;

    i = b->length + 1;
    if (b->allocated < i) {
        int rc = CMP_realloc(i, b);
        if (rc != 0)
            return rc;
    }
    b->value[i - 1] = 1;
    b->length = i;
    return 0;
}
