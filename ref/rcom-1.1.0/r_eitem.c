/*
 * The R_EITEM items of RCOM: the list of type-coded extra items that replaces the BIGNUM fields of the
 * key objects (R_EITEM_init, R_EITEM_set, R_EITEMS_dup, R_EITEM_free).
 */

#include "r_com.h"
#include "r_eitems.h"
#include <string.h>
#include "r_eitem.h"

void R_EITEM_init(R_EITEM *item)
{
    memset(item, 0, 24);
}


/* Sets the keys, the data and the length of an item; type becomes 0. With mode & 2 (and data and
 * length set) the item gets its own copy of the data (flag 2), freeing the old one if it owned it;
 * otherwise it keeps the pointer and drops flag 2. */
int R_EITEM_set(R_EITEM *item, int key1, int key2, unsigned char *data, unsigned int length, int mode)
{
    unsigned char *newbuf;

    if (mode & 2) {
        if (data != 0) {
            if (length != 0) {
                if ((newbuf = (unsigned char *)R_malloc(length)) == 0) {
                    return 1;
                }
                memcpy(newbuf, data, length);
                if (item->data != 0) {
                    if (item->flags & 2) {
                        R_free(item->data);
                    }
                }
                item->data = newbuf;
                item->flags = item->flags | 2;
                goto join;
            }
        }
    }

    if (item->data != 0) {
        if (item->flags & 2) {
            R_free(item->data);
        }
    }
    item->data = data;
    item->flags = item->flags & ~2;

join:
    item->key1 = key1;
    item->key2 = key2;
    item->length = length;
    item->type = 0;
    return 0;
}


/* Adds a copy of each item of src to dst, creating dst when it is 0. If an add fails, dst is freed
 * (even when the caller passed it) and 0 is returned. */
R_EITEMS *R_EITEMS_dup(R_EITEMS *dst, R_EITEMS *src, int type)
{
    int ret;
    int i;
    R_EITEMS *dstLocal;

    if (dst == 0) {
        dstLocal = R_EITEMS_new();
        if (dstLocal == 0) {
            return 0;
        }
    } else {
        dstLocal = dst;
    }

    for (i = 0; i < src->count; i++) {
        if (src->items[i] != 0) {
            ret = R_EITEMS_add_item(dstLocal, src->items[i], type);
            if (ret != 0) {
                goto fail;
            }
        }
    }
    return dstLocal;

fail:
    R_EITEMS_free(dstLocal);
    return 0;
}

void R_EITEM_free(R_EITEM *item)
{
    if (item->flags & 2) {
        if (item->data != 0) {
            R_free(item->data);
        }
    }
    if (item->flags & 1) {
        R_free(item);
    }
}
