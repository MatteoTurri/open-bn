/*
 * R_EITEMS_find_item and R_EITEMS_compact.
 */
#include "r_com.h"
#include "r_eitems.h"
#include <string.h>
#include "r_eitem.h"


/* Finds the first item whose two keys match (a key of 0 matches anything). *pIndex is the index to
 * start from and gets the index found + 1. With flag & 4 the search wraps around once. */
int R_EITEMS_find_item(R_EITEMS *eitems, int key1, int key2,
                       int *pIndex, R_EITEM **out, int flag)
{
    int match1;
    int match2;
    int last;
    int i;
    int ret;

    ret = 0;
    if (eitems == 0 || eitems->count == 0) {
        *out = 0;
        return 1;
    }
    last = pIndex ? *pIndex : 0;
    if (last > eitems->count) {
        last = eitems->count - 1;
    }
    match1 = (key1 == 0);
    match2 = (key2 == 0);
    i = last;
    while (match1 == 0 && match2 == 0) {
        if (i >= eitems->count) {
            if (flag & 4) {
                i = 0;
                flag &= ~4;
            } else {
                break;
            }
        }
        if ((match1 != 0 || key1 == eitems->items[i]->key1) &&
            (match2 != 0 || key2 == eitems->items[i]->key2)) {
            match2 = 1;
            match1 = 1;
            break;
        }
        i++;
        if (i == last) {
            break;
        }
    }
    if (match1 != 0 && match2 != 0) {
        *out = eitems->items[i];
        if (pIndex) {
            *pIndex = i + 1;
        }
        ret = 0;
    } else {
        *out = 0;
        ret = 1;
    }
    return ret;
}

/* Moves the data of the items into one block: sums their lengths, allocates the block, copies each
 * one in order, frees the old buffers the items owned (flag 2, then cleared) and replaces
 * eitems->extra with the block. The second argument is not used. */
int R_EITEMS_compact(R_EITEMS *eitems, int unused)
{
    int i;
    int total;
    R_EITEM *item;
    R_EITEM **items;
    unsigned char *buf;
    unsigned char *base;

    total = 0;
    items = eitems->items;
    for (i = 0; i < eitems->count; i++) {
        if (items[i]->data != 0) {
            total += items[i]->length;
        }
    }
    if ((buf = (unsigned char *)R_malloc(total)) == 0) {
        return 2;
    }
    base = buf;
    for (i = 0; i < eitems->count; i++) {
        item = items[i];
        if (item->data != 0 && item->length != 0) {
            memcpy(buf, item->data, item->length);
            if (item->flags & 2) {
                R_free(item->data);
            }
            item->flags &= ~2;
            item->data = buf;
            buf += item->length;
        }
    }
    if (eitems->extra != 0) {
        R_free(eitems->extra);
    }
    eitems->extra = base;
    return 0;
}
