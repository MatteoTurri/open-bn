/*
 * KITItem: the key info type of a generic {data, len} item (KI_Item, KITItemAddInfo).
 */
#include "bsafe_kitype.h"
#include "bsafe.h"


B_KeyInfoType KITItem = { KITItemAddInfo, B_KeyInfoTypeMakeError };

int KI_Item(void **info)
{
    *info = &KITItem;
    return 1;
}

/* Fills the KITItem key info: a generic {data, len} header that is copied and
 * whose size is recorded, with no B_InfoCacheAddInfo registration (unlike the
 * other KIT_*AddInfo). */

int KITItemAddInfo(void *ctx, unsigned int *header)
{
    void *entry;
    int status;

    status = B_MemoryPoolAllocAndCopy(ctx, &entry, (void *)header[0], header[1]);
    if (status != 0)
        return status;
    return B_KeyAddItemInfo(ctx, entry, header[1]);
}
