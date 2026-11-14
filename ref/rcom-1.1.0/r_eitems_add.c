/*
 * R_EITEMS_add.
 */
#include "r_eitem.h"
#include "r_eitems.h"


/* R_EITEMS_add: builds the item on the stack and adds it, bit 0x8 of the flags removed. */


int R_EITEMS_add(void *eitems, int key1, int key2, int type, int data, int length, int flags)
{
    R_EITEM desc;

    desc.key1 = key1;
    desc.key2 = key2;
    desc.flags = flags & ~8;
    desc.length = length;
    desc.data = (unsigned char *)data;
    desc.type = type;

    return R_EITEMS_add_item(eitems, &desc, flags & ~8);
}
