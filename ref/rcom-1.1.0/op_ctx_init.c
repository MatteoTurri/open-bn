/*
 * OP_CTX_init: a context with all the handlers.
 */
#include "bsafe.h"
#include "op_ctx.h"

/* NOTE: bug: no return statement */
int OP_CTX_init(OpCtx *ctx)
{
    OP_CTX_init_meth(ctx, OP_all_funcs());
}
