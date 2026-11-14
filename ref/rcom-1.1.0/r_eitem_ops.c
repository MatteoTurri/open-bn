/*
 * R_EITEMS_delete.
 */
#include "r_eitem.h"
#include "r_eitems.h"


/* Finds the item with R_EITEMS_find_item, frees it and moves the following ones down; the last
 * argument is not used. */
int R_EITEMS_delete(R_EITEMS *eitems, int key1, int key2, int unused4)
{
    R_EITEM *foundItem;
    int ret;
    int foundIndex = 0;
    int newCount;

    ret = R_EITEMS_find_item(eitems, key1, key2, &foundIndex, &foundItem, 0);
    if (ret != 0) {
        return ret;
    }

    R_EITEM_free(foundItem);

    newCount = eitems->count - 1;
    ret = foundIndex - 1;
    while (ret < newCount) {
        eitems->items[ret] = eitems->items[ret + 1];
        ret++;
    }
    eitems->count--;
    return 0;
}
