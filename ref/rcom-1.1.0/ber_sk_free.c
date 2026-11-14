/*
 * BER_ITEMS_SK_free: frees a vector of BER items.
 */

#include <string.h>

#include "bsafe.h"
#include "ber_item.h"

#include "r_com.h"

void BER_ITEMS_SK_free(BER_ITEMS_SK *sk)
{
    unsigned int i;

    for (i = 0; i < sk->capacity; i++) {
        if (sk->arr[i].data != 0) {
            if (sk->arr[i].state & 0x1) {
                R_free(sk->arr[i].data);
                sk->arr[i].data = 0;
            }
        }
    }

    if (sk->flag & 0x2) {
        if (sk->arr != 0) {
            R_free(sk->arr);
            sk->arr = 0;
        }
    }
    sk->count = 0;
    if (sk->flag & 0x1) {
        R_free(sk);
    }
}
