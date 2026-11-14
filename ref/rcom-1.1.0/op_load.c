/*
 * The load and push/pop handlers of the OP_CTX interpreter.
 */
#include "bsafe.h"
#include "op_ctx.h"

#include "r_com.h"

/* Writes or reads register (reg & 0xF): mode 0 writes the low 16 bits with (highByte << 8 | lowByte),
 * mode 1 the high 16 bits, mode 2 takes the next code word. Without bit 0x80 in reg: error 2. */
int op_load(OpCtx *ctx, int mode, unsigned int reg, unsigned int lowByte, unsigned int highByte)
{
    int *slot;

    if (!(reg & 0x80))
        return 2;

    slot = &OP_REG(ctx, reg);

    switch (mode) {
    case 0:
        *slot = lowByte | (highByte << 8);
        break;
    case 1:
        *slot = (*slot & 0xffff) | (((highByte << 8) | lowByte) << 16);
        break;
    case 2:
        *slot = ctx->code[ctx->pc++];
        break;
    default:
        return 2;
    }
    return 0;
}

/* Pushes on ctx->stack the n operands of operands[] that have bit 0x80, walking them backwards; the stack
 * grows by 6 words (R_realloc gets stackCap * 4 + 6 bytes). 3 when the allocation fails. */
static int op_push_args(OpCtx *ctx, unsigned int *operands, int n)
{
    unsigned int *buf;
    int i;

    if (ctx->sp + n >= ctx->stackCap) {
        buf = (unsigned int *)R_realloc((char *)ctx->stack, ctx->stackCap * 4 + 6,
                                        ctx->stackCap * 4);
        if (buf == 0)
            return 3;
        ctx->stack = buf;
        ctx->stackCap = ctx->stackCap + 6;
    }

    for (i = n - 1; i >= 0; i--) {
        if (!(operands[i] & 0x80))
            continue;
        ctx->stack[ctx->sp++] = OP_VAL(ctx, operands[i]);
    }
    return 0;
}

/* Pops into register (operands[i] & 0xF) each operand with bit 0x80, forwards. 4 when the stack is empty
 * (checked once). */
static int op_pop_args(OpCtx *ctx, unsigned int *operands, int n)
{
    int i;

    if (ctx->sp == 0)
        return 4;
    for (i = 0; i < n; i++) {
        if (!(operands[i] & 0x80))
            continue;
        OP_REG(ctx, operands[i]) = ctx->stack[--ctx->sp];
    }
    return 0;
}


/* The push/pop opcode: mode 0 push, 1 pop, otherwise 2. */
int op_push_pop(OpCtx *ctx, int mode, unsigned int operand1, unsigned int operand2,
                unsigned int operand3)
{
    int ret;
    unsigned int operands[3];

    operands[0] = operand1;
    operands[1] = operand2;
    operands[2] = operand3;
    if (mode == 0) {
        ret = op_push_args(ctx, operands, 3);
        if (ret != 0)
            return ret;
    } else if (mode == 1) {
        ret = op_pop_args(ctx, operands, 3);
        if (ret != 0)
            return ret;
    } else
        return 2;
    return 0;
}


