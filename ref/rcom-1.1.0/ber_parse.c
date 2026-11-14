/*
 * BER_parse: builds the BER item tree of an encoding.
 */

#include <string.h>

#include "bsafe.h"
#include "ber_item.h"


/* Builds the BER_ITEM tree in the vector sk from data (dataLen bytes); *outConsumed gets the bytes
 * used. Two states in one loop: 1 reads the next item (a new slot from BER_ITEMS_SK_get, then
 * BER_read_item) and links it as first child or as next sibling; 2 goes up one level. The parent is
 * kept both as a pointer and as an index, because BER_ITEMS_SK_get may move the vector. Errors: 2 out
 * of the buffer, 11 an end of contents outside an indefinite-length item, 12 the end of contents in
 * an item that is not constructed. */

int BER_parse(BER_ITEMS_SK *sk, char *data, int dataLen, int *outConsumed)
{
    int parentIdx;
    int newIdx;
    BER_ITEM *parent = 0;
    BER_ITEM *item = 0;
    int status = 999;
    int state;
    char *p = data;
    char *end = p + dataLen;
    char *limit = end;
    int flagA = 0;
    int flagB = 0;
    int flagC = 0;
    int descend;

    state = 1;
    descend = 1;
    parentIdx = -1;
    flagB = 0;

    for (;;) {
        switch (state) {
        case 1:
            status = BER_ITEMS_SK_get(sk, &newIdx);
            if (status != 0)
                goto ret;

            item = sk->arr + newIdx;
            status = BER_read_item(item, p, limit - p);
            if (status != 0)
                goto ret;

            flagA = item->flags & 0x2;
            flagC = item->flags & 0x20;
            p += item->lenOfLen;

            if (parentIdx == -1)
                parent = 0;
            else
                parent = sk->arr + parentIdx;

            if (descend) {
                item->parent = parent;
                if (parent != 0) {
                    parent->child = item;
                    if (!flagB)
                        limit = (char *)parent->data + parent->len;
                    flagB = parent->flags & 0x2;
                } else {
                    flagB = 0;
                }
                item->next = 0;
            } else {
                item->parent = parent->parent;
                item->next = 0;
                parent->next = item;
            }

            descend = 0;
            parent = item;
            parentIdx = newIdx;

            if (parent->tag == 0 && parent->len == 0 &&
                !(parent->flags & 0xc0)) {
                if (flagB) {
                    parent->parent->len =
                        p - (char *)parent->parent->data;
                    state = 2;
                    break;
                } else {
                    status = 11;
                    goto ret;
                }
            }

            if (flagC && (flagA || parent->len)) {
                flagB = flagA;
                descend = 1;
            } else {
                p += parent->len;
            }

            if (p > limit) {
                status = 2;
                goto ret;
            }

            if (p == limit)
                state = 2;
            else
                state = 1;

            if (flagA && !flagC) {
                status = 12;
                goto ret;
            }
            break;

        case 2:
            parentIdx += parent->parent - parent;
            parent = parent->parent;
            descend = 0;
            if (parent == 0) {
                status = 0;
                goto done;
            }

            if (parent->parent != 0) {
                flagB = parent->parent->flags & 0x2;
                if (flagB) {
                    limit = end;
                } else {
                    limit = (char *)parent->parent->data +
                            parent->parent->len;
                }
            } else {
                flagB = 0;
                limit = end;
            }

            if (limit < p) {
                status = 2;
                goto ret;
            }

            if (limit == p) {
                state = 2;
                break;
            } else {
                state = 1;
                break;
            }
            break;

        default:
            break;
        }
    }

done:
    if (outConsumed != 0)
        *outConsumed = p - data;
ret:
    return status;
}
