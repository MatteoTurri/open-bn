/*
 * DH_size.
 */
/* The DH object of SSL-C: the key is an R_EITEMS list and the operations go through a PK_CTX that
 * DH_new_meth creates. */

#include <stdio.h>
#include "r_eitems.h"
#include "dh.h"

int DH_size(DH *dh)
{
    R_EITEM *item;
    int result = 0;

    if (R_EITEMS_find_item(dh->eitems, 18, 1, 0, &item, 0) == 0) {
        result = item->length;
        if (result > 0) {
            if (item->data[0] == 0)
                result -= 1;
        }
    }
    return result;
}
