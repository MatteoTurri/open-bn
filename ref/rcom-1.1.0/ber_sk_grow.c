/*
 * BER_ITEMS_SK_grow: grows a vector of BER items.
 */

#include <string.h>

#include "bsafe.h"
#include "ber_item.h"

#include "r_com.h"


/* Grows the vector to newCap items (28 bytes each): R_malloc when it has none, otherwise R_realloc
 * and zeroing of the new tail. If the block moved, the links of the items (parent, next, child, which
 * point into the vector) are rebased. 6: the vector does not own its array; 5: the allocation
 * failed. */
int BER_ITEMS_SK_grow(BER_ITEMS_SK *sk, int newCap)
{
    int wasRealloc;
    unsigned int i;
    int delta;
    BER_ITEM *newArr;
    BER_ITEM *oldBase;
    BER_ITEM *oldEnd;

    if (newCap <= sk->count)
        return 0;

    if (sk->flag & 0x2) {
        if (sk->arr == 0) {
            wasRealloc = 0;
            newArr = (BER_ITEM *)R_malloc(newCap * 28);
            memset(newArr, 0, newCap * 28);
        } else {
            wasRealloc = 1;
            newArr = (BER_ITEM *)R_realloc((char *)sk->arr, newCap * 28, sk->capacity * 28);
            memset(newArr + sk->capacity, 0, (newCap - sk->capacity) * 28);
        }

        if (newArr == 0)
            return 5;

        if (sk->arr != newArr && wasRealloc) {
            oldBase = sk->arr;
            oldEnd = sk->arr + sk->count;
            for (i = 0; i < sk->count; i++) {
                if (newArr[i].parent >= oldBase &&
                    newArr[i].parent <= oldEnd) {
                    delta = newArr[i].parent - oldBase;
                    newArr[i].parent =
                        newArr + delta;
                }
                if (newArr[i].next >= oldBase &&
                    newArr[i].next <= oldEnd) {
                    delta = newArr[i].next - oldBase;
                    newArr[i].next =
                        newArr + delta;
                }
                if (newArr[i].child >= oldBase &&
                    newArr[i].child <= oldEnd) {
                    delta = newArr[i].child - oldBase;
                    newArr[i].child =
                        newArr + delta;
                }
            }
        }

        sk->capacity = newCap;
        sk->arr = newArr;
        return 0;
    }
    return 6;
}
