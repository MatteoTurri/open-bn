/*
 * OP_default_callback: the callback OP_decode and OP_encode install, which moves
 * values between the current BER item and an R_EITEMS list.
 */
#include "bsafe.h"
#include "op_ctx.h"
#include "r_eitem.h"
#include "r_eitems.h"

/* Installed by OP_decode and OP_encode with OP_CTX_set_function and called by OP_CTX_run with
 * (ctx, items, out, code, key1, key2). code < 32 is an ASN.1 tag; then '@' (0x40) exists, 'A' copy
 * with type, 'B' add from the current item, 'C' add an integer, 'D' extract the value. */

int OP_default_callback(OpCtx *ctx, void *items, int *out, int code, int key1, int key2)
{
    void *eitems;
    R_EITEM *item;

    eitems = items;
    if (code < 32) {
        if (R_EITEMS_find_item(eitems, key1, key2, 0, &item, 0) != 0)
            return 1;
        BER_ITEM_set_all(ctx->curItem, 0, code, item->data, item->length, 0, 0);
    } else if (code == 65) {
        if (R_EITEMS_find_item(eitems, key1, key2, 0, &item, 0) != 0)
            return 1;
        BER_ITEM_set_all(ctx->curItem, 0, item->type, item->data,
                         item->length, 0, 0);
    } else if (code == 68) {
        if (R_EITEMS_find_item(eitems, key1, key2, 0, &item, 0) != 0)
            return 1;
        *out = item->length;
    } else if (code == 66) {
        R_EITEMS_add(eitems, key1, key2, ctx->curItem->tag,
                     (int)ctx->curItem->data, ctx->curItem->len, 16);
    } else if (code == 67) {
        R_EITEMS_add(eitems, key1, key2, 2, 0, *out, 16);
    } else if (code == 64) {
        R_EITEMS_find_item(eitems, key1, key2, 0, &item, 0);
        *out = (item != 0);
        return 0;
    } else {
        return 2;
    }
    return 0;
}
