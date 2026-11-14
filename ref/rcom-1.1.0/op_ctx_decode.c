/*
 * OP_CTX_decode.
 */
#include "bsafe.h"
#include "op_ctx.h"

/* Parses the encoding into the item vector of ctx; when items were added, the first of them becomes
 * the current one. */
int OP_CTX_decode(OpCtx *ctx, unsigned char *data, int len, int *consumed)
{
    int status;
    unsigned int savedCount;

    savedCount = ctx->sk.count;
    status = BER_parse(&ctx->sk, data, len, consumed);
    if (status == 0) {
        if (savedCount != ctx->sk.count)
            ctx->cur = savedCount;
    }
    return status;
}

int OP_CTX_run(OpCtx *ctx, void *arg)
{
    unsigned int *table;
    unsigned int code;
    unsigned int lowByte;
    unsigned int nibbleHigh;
    unsigned int nibbleLow;
    int status = 0;
    unsigned int operand1;
    unsigned int operand2;
    unsigned int operand3;
    int count = 0;
    int legacy;

    ctx->pc = 0;
    table = ctx->code;
    legacy = ctx->flags & 0x2;
    if (legacy != 0) {
        if (ctx->callback == 0)
            legacy = 0;
    }
    ctx->arg = arg;

    for (;;) {
        if (ctx->pc > ctx->codeLen)
            return 9;

        if (legacy != 0) {
            status = ctx->callback(ctx, ctx->arg, ctx->regs, -1, 0, 0);
            if (status != 0)
                break;
        }

        if ((code = table[ctx->pc++]) == 0)
            break;

        count++;
        lowByte = code & 0xff;
        nibbleLow = lowByte & 0xf;
        nibbleHigh = (lowByte >> 4) & 0xf;
        operand1 = (code >> 8) & 0xff;
        operand2 = (code >> 16) & 0xff;
        operand3 = (code >> 24) & 0xff;

        status = ctx->funcs[nibbleHigh](ctx, nibbleLow, operand1, operand2, operand3);
        if (status != 0 || lowByte == 1)
            break;
    }
    return status;
}
