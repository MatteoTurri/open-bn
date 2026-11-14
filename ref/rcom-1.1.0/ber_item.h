/*
 * The BER item tree of RCOM: BER_parse builds it in a vector of items, the OP_CTX interpreter walks
 * and edits it, BER_ITEMS_encode writes it back.
 */
#ifndef BER_ITEM_H
#define BER_ITEM_H

/* One node of the tree (28 bytes). */
typedef struct BER_ITEM {
    /* 0x00 */ int len;              /* content length (summed over the children of a constructed item) */
    /* 0x04 */ unsigned char *data;  /* content bytes */
    /* 0x08 */ int tag;
    /* 0x0C */ unsigned char flags;  /* bits 7-6 class, bit 5 constructed */
    /* 0x0D */ unsigned char lenOfLen;  /* length of the long-form length, 0 in short form */
    /* 0x0E */ unsigned char state;  /* 0x8 header set, 0x4 prefix byte cached, 0x1 data owned */
    /* 0x0F */ unsigned char prefix;  /* cached prefix byte */
    /* 0x10 */ struct BER_ITEM *parent;
    /* 0x14 */ struct BER_ITEM *next;
    /* 0x18 */ struct BER_ITEM *child;
} BER_ITEM;

/* Vector of items, stored by value. */
typedef struct BER_ITEMS_SK {
    /* 0x00 */ unsigned int count;
    /* 0x04 */ unsigned int capacity;
    /* 0x08 */ BER_ITEM *arr;
    /* 0x0C */ int flag;  /* 0x2 arr owned, 0x1 the vector itself is on the heap */
} BER_ITEMS_SK;

int BER_ITEM_cmp_tag(BER_ITEM *item, int wantedTag);
int BER_ITEM_get_long(BER_ITEM *item, int *outVal);
int BER_ITEM_set_long(BER_ITEM *item, int value, void *outBufArg);
void BER_ITEM_set_header(BER_ITEM *item, unsigned int cls, unsigned int tag, unsigned int flags);
void BER_ITEM_set_data(BER_ITEM *item, void *data, int len);
void BER_ITEM_set_all(BER_ITEM *item, int cls, int tag, void *data, int len, int flagsMid,
                      int flagsExtra);
void BER_ITEM_set_prefix_byte(BER_ITEM *item, unsigned int prefixByte);
void BER_ITEMS_append(BER_ITEM *x, BER_ITEM *y);
int BER_ITEMS_under(BER_ITEM *x, BER_ITEM *y);
int BER_ITEMS_recalc_length(BER_ITEM *root);
int BER_ITEMS_encode(BER_ITEM *root, char *buf, int *outLen, int bufLen);
int BER_read_item(BER_ITEM *item, unsigned char *buf, unsigned int len);
int BER_parse(BER_ITEMS_SK *sk, char *data, int dataLen, int *outConsumed);
void BER_ITEMS_SK_init(BER_ITEMS_SK *sk, BER_ITEM *arr, unsigned int count, unsigned int capacity);
void BER_ITEMS_SK_free(BER_ITEMS_SK *sk);
int BER_ITEMS_SK_get(BER_ITEMS_SK *sk, int *outIndex);
int BER_ITEMS_SK_grow(BER_ITEMS_SK *sk, int newCap);

#endif
