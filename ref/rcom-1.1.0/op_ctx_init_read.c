/*
 * OP_CTX_init_read: a context with the read-only handlers.
 */
#include "bsafe.h"
#include "op_ctx.h"

/* NOTE: bug: no return statement */
int OP_CTX_init_read(OpCtx *ctx)
{
    OP_CTX_init_meth(ctx, OP_read_funcs());
}
