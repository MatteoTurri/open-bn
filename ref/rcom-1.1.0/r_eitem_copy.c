/*
 * R_EITEM_copy and R_EITEMS_check_list.
 */
#include "r_eitem.h"
#include "r_eitems.h"


/* Copies src into dst with R_EITEM_set (a private copy when src or otherFlags has flag 2), then
 * copies the type, which R_EITEM_set clears. */
int R_EITEM_copy(R_EITEM *dst, R_EITEM *src, int otherFlags)
{
    int ret;
    int mode;

    if ((src->flags & 2) || (otherFlags & 2)) {
        mode = 2;
    } else {
        mode = 0;
    }

    ret = R_EITEM_set(dst, src->key1, src->key2, src->data, src->length, mode);
    dst->type = src->type;
    return ret;
}


/* Walks a 0-terminated list of type codes and returns the first nonzero result of
 * R_EITEMS_find_item. */
int R_EITEMS_check_list(void *eitems, int cap, int *typeList)
{
    int index = 0;
    int ret;
    R_EITEM *item;

    while (*typeList != 0) {
        ret = R_EITEMS_find_item(eitems, cap, *typeList, &index, &item, 4);
        if (ret != 0) {
            return ret;
        }
        typeList++;
    }
    return 0;
}
