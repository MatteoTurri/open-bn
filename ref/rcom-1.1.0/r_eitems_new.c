/*
 * R_EITEMS_new.
 */
#include "r_com.h"
#include "r_eitems.h"
#include <string.h>
#include "r_eitem.h"

void *R_EITEMS_new(void)
{
    R_EITEM *item;

    if ((item = (R_EITEM *)R_malloc(24)) == 0) {
        return 0;
    }
    memset(item, 0, 24);
    item->flags = 1;
    return item;
}
