/*
 * The BER item tree of RCOM: computing the lengths of a tree (BER_ITEMS_recalc_length) and writing
 * it back as DER (BER_ITEMS_encode).
 */
#include <string.h>

#include "bsafe.h"
#include "ber_item.h"


/* Recomputes the BER lengths of the tree rooted at root, walking it without recursion: down to the
 * first child, across to the next sibling, up to the parent (goingUp tells from where the walk came).
 * A constructed item (or one with a prefix byte) starts its length at 1 or 0 and goes down; the
 * others add their content and header length to the parent, or to the total at the root. */
static int ber_len_of_len(BER_ITEM *item);

int BER_ITEMS_recalc_length(BER_ITEM *root)
{
    BER_ITEM *cur;
    BER_ITEM *saved;
    int goingUp;
    int totalLen;

    goingUp = 0;
    totalLen = 0;
    cur = root;

    for (;;) {
        if (!goingUp) {
            if (!(cur->flags & 0x8)) {
                if ((cur->flags & 0x20) || (cur->flags & 0x4)) {
                    if (cur->state & 0x4)
                        cur->len = 1;
                    else
                        cur->len = 0;
                    cur->lenOfLen = 0;
                    if (cur->child != 0) {
                        cur = cur->child;
                        continue;
                    }
                }
            }
        }

        if (cur->flags & 0x8) {
            cur->lenOfLen = 0;
        } else {
            cur->lenOfLen = ber_len_of_len(cur);
        }
        cur->state = cur->state & 0xf7;

        if (cur->parent != 0) {
            cur->parent->len += cur->lenOfLen + cur->len;
        } else {
            totalLen += cur->len + cur->lenOfLen;
        }

        saved = cur;
        if (cur->next != 0) {
            goingUp = 0;
            cur = cur->next;
        } else {
            goingUp = 1;
            cur = cur->parent;
        }

        if (cur == 0)
            break;
    }

    return totalLen;
}


/* Serializes the tree rooted at root into buf (bufLen bytes) with the walk of
 * BER_ITEMS_recalc_length: each item writes its header, its cached prefix byte and, for a leaf, its
 * contents. *outLen gets the bytes written. 12: the lengths were not recomputed; 2: the buffer is too
 * short. */
/* The length of the header of an item: the tag octets (1 to 4) and the length octets (1 for the
 * indefinite length, otherwise short or long form). */
static int ber_len_of_len(BER_ITEM *item)
{
    int n;
    unsigned int tag;
    unsigned int len;

    tag = item->tag;
    if (tag < 31)
        n = 1;
    else if (tag < 128)
        n = 2;
    else if (tag < 16384)
        n = 3;
    else
        n = 4;

    len = item->len;
    if (item->flags & 2)
        n += 1;
    else if (len < 128)
        n += 1;
    else if (len < 256)
        n += 2;
    else if (len <= 0xffff)
        n += 3;
    else if (len <= 0xffffff)
        n += 4;
    else
        n += 5;
    return n;
}

static int ber_write_header(char *buf, BER_ITEM *item);

int BER_ITEMS_encode(BER_ITEM *root, char *buf, int *outLen, int bufLen)
{
    BER_ITEM *cur;
    BER_ITEM *saved;
    BER_ITEM *node;
    char *bufEnd;
    char *bufStart;
    int len;
    int goingUp;

    bufStart = buf;
    goingUp = 0;
    bufEnd = buf + bufLen;
    cur = root;

    for (;;) {
        node = cur;
        if (!goingUp) {
            if (node->state & 0x8)
                return 12;

            if (buf + node->len + node->lenOfLen > bufEnd)
                return 2;

            if (!(node->flags & 0x8))
                buf = buf + ber_write_header(buf, node);

            len = node->len;
            if ((node->state & 0x4) && len != 0) {
                *buf++ = node->prefix;
                len--;
            }

            if (!(node->flags & 0x8) &&
                ((node->flags & 0x20) || (node->flags & 0x4))) {
                if (cur->child != 0) {
                    cur = cur->child;
                    continue;
                }
            } else {
                if (len != 0) {
                    memcpy(buf, node->data, len);
                    buf = buf + len;
                }
            }
        }

        saved = cur;
        if (cur->next != 0) {
            goingUp = 0;
            cur = cur->next;
        } else {
            goingUp = 1;
            cur = cur->parent;
        }

        if (cur == 0)
            break;
    }

    *outLen = buf - bufStart;
    return 0;
}

/* Writes the DER header of the item into buf: the identifier, the tag in base 128, then the length
 * (0x50 for the indefinite form, short form below 128, otherwise long form). Returns the bytes
 * written. `constructed` is computed and not used. */
static int ber_write_header(char *buf, BER_ITEM *item)
{
    unsigned char first;
    char *lenPos;
    char *p;
    unsigned int tag;
    int constructed;
    int nlen;
    unsigned int len;

    p = buf;
    tag = item->tag;
    constructed = item->flags & 0x20;
    first = item->flags & 0xe0;

    if (tag < 31) {
        first = first | tag;
        *p++ = first;
    } else {
        first = first | 0x1f;
        *p++ = first;
        if (tag >= 16384)
            *p++ = (tag >> 14) & 0x7f;
        if (tag >= 128)
            *p++ = (tag >> 7) & 0x7f;
        *p++ = tag & 0x7f;
    }

    len = item->len;
    if (item->flags & 2) {
        *p++ = 0x50;
    } else if (len < 128) {
        *p++ = len;
    } else {
        lenPos = p++;
        nlen = 1;
        if (len > 0xffffff) {
            nlen++;
            *p++ = len >> 24;
        }
        if (len > 0xffff) {
            nlen++;
            *p++ = len >> 16;
        }
        if (len >= 256) {
            nlen++;
            *p++ = len >> 8;
        }
        *p++ = len;
        *lenPos = nlen | 0x80;
    }
    return p - buf;
}
