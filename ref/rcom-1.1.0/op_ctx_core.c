/*
 * OP_CTX_set_function: the callback of the interpreter.
 */
#include "bsafe.h"
#include "op_ctx.h"

int OP_CTX_set_function(OpCtx *ctx, void *fn)
{
    ctx->callback = fn;
    return 0;
}
