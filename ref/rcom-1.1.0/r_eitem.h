/*
 * The type-coded extra items of RCOM: an R_EITEM holds one component of a key (its keys, type and
 * data), an R_EITEMS list holds the items of a key object.
 */
#ifndef R_EITEM_H
#define R_EITEM_H

typedef struct r_eitem_st {
    /* 0x00 */ int key1;  /* the two keys R_EITEMS_find_item matches (e.g. 16 and the RSA component) */
    /* 0x04 */ int key2;
    /* 0x08 */ int type;
    /* 0x0C */ unsigned char *data;
    /* 0x10 */ int length;
    /* 0x14 */ int flags;  /* 1: the item is allocated, 2: it owns data */
} R_EITEM; /* size = 0x18 */

typedef struct {
    /* 0x00 */ int pad_00;  /* unused, but part of the layout */
    /* 0x04 */ int count;  /* items in use */
    /* 0x08 */ int capacity;  /* items allocated */
    /* 0x0C */ R_EITEM **items;
    /* 0x10 */ void *extra;  /* the block R_EITEMS_compact moves the data into */
    /* 0x14 */ int flags;  /* 1: the list is allocated */
} R_EITEMS; /* size = 0x18 */

#endif
