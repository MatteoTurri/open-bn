/*
 * R_EITEMS_free.
 */
#include "r_com.h"
#include "r_eitem.h"
#include "r_eitems.h"


void R_EITEMS_free(R_EITEMS *eitems)
{
    int i;

    for (i = 0; i < eitems->count; i++) {
        R_EITEM_free(eitems->items[i]);
    }

    if (eitems->items != 0) {
        R_free(eitems->items);
        eitems->items = 0;
    }
    if (eitems->extra != 0) {
        R_free(eitems->extra);
    }
    if (eitems->flags & 1) {
        R_free(eitems);
    }
}
