/*
 * OP_decode and OP_encode: run an OP_CTX program over a BER encoding, on the
 * caller's context or on a temporary one.
 */
#include "bsafe.h"
#include "op_ctx.h"

int OP_decode(OpCtx *ctxArg, unsigned int *code, void *runArg, unsigned char *data, int len, int *consumed)
{
    OpCtx localCtx;
    OpCtx *ctx;
    int status;

    if (ctxArg == 0) {
        OP_CTX_init_read(&localCtx);
        OP_CTX_set_function(&localCtx, (void *)OP_default_callback);
        ctx = &localCtx;
    } else {
        OP_CTX_reset(ctxArg);
        ctx = ctxArg;
    }

    if (code != 0) {
        status = OP_CTX_code(ctx, code);
        if (status != 0)
            goto finish;
    }

    status = OP_CTX_decode(ctx, data, len, consumed);
    if (status != 0)
        goto finish;
    status = OP_CTX_run(ctx, runArg);

finish:
    if (ctxArg == 0)
        OP_CTX_free(&localCtx);
    return status;
}

int OP_encode(OpCtx *ctxArg, unsigned int *code, void *runArg, unsigned char *buf, int *outLen, int bufLen)
{
    OpCtx localCtx;
    OpCtx *ctx;
    int status;

    if (ctxArg == 0) {
        OP_CTX_init(&localCtx);
        OP_CTX_set_function(&localCtx, (void *)OP_default_callback);
        ctx = &localCtx;
    } else {
        OP_CTX_reset(ctxArg);
        ctx = ctxArg;
    }

    if (code != 0) {
        status = OP_CTX_code(ctx, code);
        if (status != 0)
            goto finish;
    }

    status = OP_CTX_run(ctx, runArg);
    if (status != 0)
        goto finish;
    status = OP_CTX_encode(ctx, buf, outLen, bufLen);

finish:
    if (ctxArg == 0)
        OP_CTX_free(&localCtx);
    return status;
}


