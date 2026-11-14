/*
 * The functions of the R_EITEM lists (r_eitem*.c, r_eitems*.c).
 */
#ifndef RCOM_R_EITEMS_H
#define RCOM_R_EITEMS_H

#include "r_eitem.h"

/* r_eitems_add.c */
int R_EITEMS_add(void *eitems, int key1, int key2, int type, int data, int length, int flags);

/* r_eitem_items.c */
int R_EITEMS_add_item(R_EITEMS *eitems, R_EITEM *item, int flags);

/* r_eitem_copy.c */
int R_EITEMS_check_list(void *eitems, int cap, int *typeList);
int R_EITEM_copy(R_EITEM *dst, R_EITEM *src, int otherFlags);

/* r_eitems_find.c */
int R_EITEMS_compact(R_EITEMS *eitems, int unused);
int R_EITEMS_find_item(R_EITEMS *eitems, int key1, int key2, int *pIndex, R_EITEM **out, int flag);

/* r_eitem_ops.c */
int R_EITEMS_delete(R_EITEMS *eitems, int key1, int key2, int unused4);

/* r_eitem.c */
R_EITEMS *R_EITEMS_dup(R_EITEMS *dst, R_EITEMS *src, int type);
void R_EITEM_free(R_EITEM *item);
void R_EITEM_init(R_EITEM *item);
int R_EITEM_set(R_EITEM *item, int key1, int key2, unsigned char *data, unsigned int length,
                int mode);

/* r_eitems_free.c */
void R_EITEMS_free(R_EITEMS *eitems);

/* r_eitems_new.c */
void *R_EITEMS_new(void);

/* r_eitem_new.c */
R_EITEM *R_EITEM_new(void);

#endif
