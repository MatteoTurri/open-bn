/*
 * AllocAndCopyIntegerItems: copies the integer items of a key into the pool of the key object.
 */

#include "aglobal.h"
#include "bsafe.h"


/* Copies each of the count integer items named by items[] (big-endian, without the leading zero
 * bytes) from the block `header` to the block `outBuf`. items[i] points into a table based at layoutBase;
 * items[i] - layoutBase is the offset of the item in all three blocks, which share the layout. */
int AllocAndCopyIntegerItems(void *outBuf, void *header, ITEM *layoutBase,
                             ITEM **items, unsigned int count, void *ctx)
{
    unsigned int i;

    for (i = 0; i < count; i++) {
        unsigned int off = (unsigned int)((char *)items[i] - (char *)layoutBase);
        ITEM *dst = (ITEM *)((char *)outBuf + off);
        ITEM item = *(ITEM *)((char *)header + off);
        int status;

        while (item.len != 0 && *(unsigned char *)item.data == 0) {
            item.len--;
            item.data = (unsigned char *)item.data + 1;
        }
        dst->len = item.len;
        status = B_MemoryPoolAllocAndCopy(ctx, (void **)&dst->data, item.data, item.len);
        if (status != 0)
            return status;
    }
    return 0;
}
