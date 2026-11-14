/*
 * R_EITEMS_add_item and the growing of an item list.
 */
#include "r_eitem.h"
#include "r_eitems.h"

#include "r_com.h"


/* Grows the items array to n entries. Returns 1 when the allocation fails, 0 otherwise (also when
 * there is nothing to do: n not above count, or not positive). */
static int R_EITEMS_resize(R_EITEMS *eitems, int n)
{
    void *p;

    if (eitems->capacity >= n || n <= 0)
        return 0;

    if (eitems->items == 0) {
        p = R_malloc(n * 4);
    } else {
        p = R_realloc((char *)eitems->items, n * 4, eitems->capacity * 4);
    }
    if (p == 0) {
        return 1;
    }
    eitems->items = (R_EITEM **)p;
    eitems->capacity = n;
    return 0;
}

int R_EITEMS_add_item(R_EITEMS *eitems, R_EITEM *item, int flags)
{
    int ret;
    R_EITEM *ni;

    if (flags & 0x10) {
        R_EITEMS_delete(eitems, item->key1, item->key2, 0);
    }
    if (eitems->capacity <= eitems->count) {
        if ((ret = R_EITEMS_resize(eitems, eitems->count + 4)) != 0) {
            return ret;
        }
    }
    if (!(flags & 8)) {
        if ((ni = R_EITEM_new()) == 0) {
            return 1;
        }
        if ((ret = R_EITEM_copy(ni, item, flags)) != 0) {
            return ret;
        }
        item = ni;
    }
    eitems->items[eitems->count++] = item;
    return 0;
}
