/*
 * Reading BER items: the header setters, BER_read_item and the item vector (BER_ITEMS_SK_init,
 * BER_ITEMS_SK_get).
 */
#include <string.h>

#include "bsafe.h"
#include "ber_item.h"


/* BER_ITEMS_SK_*: a vector of BER_ITEM structures (not of pointers, as the STACK of SSL-C). */


void BER_ITEM_set_header(BER_ITEM *item, unsigned int cls, unsigned int tag, unsigned int flags)
{
    item->flags = (cls & 0xc0) | (flags & 0x2f);
    item->state = item->state | 0x8;
    if ((cls & 0xc0) == 0 && (tag == 16 || tag == 17))
        item->flags = item->flags | 0x20;
    item->tag = tag;
}


void BER_ITEM_set_data(BER_ITEM *item, void *data, int len)
{
    item->state = (item->state & 0xff) | 0x8;
    item->data = data;
    item->len = len;
}

/* Decodes the BER header (identifier and length) in buf into the fields of the item and returns 0.
 * 2: the buffer is too short; 1: a tag of more than 32 bits; 3: a long-form length of more than 4
 * bytes. A multi-byte tag goes on while bit 0x80 is clear, the opposite of BER. */
int BER_read_item(BER_ITEM *item, unsigned char *buf, unsigned int len)
{
    unsigned int pos;
    unsigned int tag;
    unsigned int n;
    unsigned int tagByte;
    unsigned int idByte;
    unsigned int contentLen;

    pos = 0;
    if (len < 2)
        return 2;

    idByte = buf[pos++];
    item->flags = idByte & 0xe0;
    tag = idByte & 0x1f;
    if (tag == 31) {
        tag = 0;
        n = 0;
        for (;;) {
            if (pos >= len)
                return 2;
            tagByte = buf[pos++];
            tag = tag | (tagByte & 0x7f);
            if (tagByte & 0x80)
                break;
            n += 7;
            if (n > 32)
                return 1;
            tag = tag << 7;
        }
    }
    item->tag = tag;

    if (pos >= len)
        return 2;
    contentLen = buf[pos++];
    if (contentLen & 0x80) {
        contentLen = contentLen & 0x7f;
        if (contentLen == 0) {
            item->flags = item->flags | 2;
            item->state = item->state | 8;
        } else {
            if (contentLen > 4)
                return 3;
            if (pos + contentLen >= len)
                return 2;
            n = contentLen;
            contentLen = 0;
            for (;;) {
                contentLen = contentLen | buf[pos++];
                if (--n == 0)
                    break;
                contentLen = contentLen << 8;
            }
        }
    }

    item->len = contentLen;
    item->data = buf + pos;
    item->lenOfLen = pos;
    return 0;
}

void BER_ITEMS_SK_init(BER_ITEMS_SK *sk, BER_ITEM *arr, unsigned int count, unsigned int capacity)
{
    sk->count = count;
    sk->capacity = capacity;
    if (arr == 0)
        sk->flag = 2;
    else
        sk->flag = 0;
    sk->arr = arr;
}

int BER_ITEMS_SK_get(BER_ITEMS_SK *sk, int *outIndex)
{
    int status;
    BER_ITEM *newItem;

    if (sk->count >= sk->capacity) {
        status = BER_ITEMS_SK_grow(sk, (sk->count + 2) * 2);
        if (status != 0)
            return status;
    }

    *outIndex = sk->count;
    newItem = sk->arr + sk->count++;
    memset(newItem, 0, 28);
    return 0;
}
