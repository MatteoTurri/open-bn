/*
 * OP_CTX_init_meth.
 */
#include "bsafe.h"
#include "op_ctx.h"

/* Clears the whole context (156 bytes), sets up an empty item vector and installs the handler
 * table. */
int OP_CTX_init_meth(OpCtx *ctx, void *funcs)
{
    memset(ctx, 0, 156);
    BER_ITEMS_SK_init(&ctx->sk, 0, 0, 0);
    ctx->funcs = funcs;
}
