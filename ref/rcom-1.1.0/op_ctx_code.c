/*
 * OP_CTX_code, OP_CTX_free, OP_CTX_reset and OP_CTX_encode.
 */
#include "bsafe.h"
#include "op_ctx.h"

#include "r_com.h"

/* A pass over the code words that OP_CTX_run will execute: builds the label table (opcode 0x20,
 * label N in the high 16 bits: labels[N] = the index of the word; the table grows with R_realloc and
 * the gaps hold 0xFFFF) and counts the registers used (opcodes 0x10-0x13), allocating ctx->marks
 * with one zeroed word per register. Opcode 0x92 has one more argument word. ctx->codeLen gets the
 * number of words scanned. */
int OP_CTX_code(OpCtx *ctx, unsigned int *code)
{
    int maxReg = 0;
    unsigned int word;
    int cursor = 0;
    int status = 0;
    /* the operand fields, cleared for each word and not read in this pass */
    int operand1;
    int operand2;
    int operand3;
    int label;
    int reg;
    int j;
    int cap = 0;
    int highWater = 0;
    unsigned int *labels;

    cap = ctx->labelCount;
    if (cap == 0) {
        labels = (unsigned int *)R_malloc(16);
        cap = 4;
    } else {
        labels = ctx->labels;
        ctx->labels = 0;
    }
    memset(labels, 255, cap * 4);

    ctx->code = code;
    ctx->codeLen = 0;
    ctx->cur = -1;

    for (;;) {
        word = code[cursor++];
        ctx->codeLen++;
        if (word == 0)
            break;

        operand3 = 0;
        operand2 = 0;
        operand1 = 0;
        label = -1;
        reg = -1;

        switch (word & 0xff) {
        case 16:
        case 17:
        case 18:
        case 19:
            reg = (word >> 16) & 0xffff;
            break;
        case 32:
            label = (word >> 16) & 0xffff;
            break;
        case 146:
            cursor++;
            break;
        default:
            break;
        }

        if (label >= 0) {
            if (cap < label + 1) {
                labels = (unsigned int *)R_realloc((char *)labels, (label + 5) * 4, cap * 4);
                cap = label + 4;
                ctx->labelCount = cap;
                for (j = highWater; j < cap; j++)
                    labels[j] = 0xffff;
            }
            if (labels[label] != 0xffff) {  /* redundant as written */
            }
            labels[label] = cursor;
            if (highWater < label + 1)
                highWater = label + 1;
        }
        if (reg > maxReg)
            maxReg = reg;
    }

    ctx->labelCount = highWater;
    if (ctx->labels != 0)
        R_free(ctx->labels);
    ctx->labels = labels;
    labels = 0;
    maxReg = maxReg + 1;
    ctx->markCount = maxReg;
    if (maxReg > 0) {
        if (ctx->marks != 0)
            R_free(ctx->marks);
        ctx->marks = (int *)R_malloc(maxReg * 4);
        memset(ctx->marks, 0, maxReg * 4);
    } else {
        ctx->marks = 0;
    }
    status = 0;
    if (labels != 0)
        R_free(labels);
    return status;
}

/* Frees the item vector, the three buffers and, if flags has 1, the context itself. */
void OP_CTX_free(OpCtx *ctx)
{
    BER_ITEMS_SK_free(&ctx->sk);
    if (ctx->marks != 0)
        R_free(ctx->marks);
    if (ctx->labels != 0)
        R_free(ctx->labels);
    if (ctx->stack != 0)
        R_free(ctx->stack);
    if (ctx->flags & 0x1)
        R_free(ctx);
}

/* Empties the item vector and clears the registers; no current item. */
void OP_CTX_reset(OpCtx *ctx)
{
    BER_ITEMS_SK_free(&ctx->sk);
    BER_ITEMS_SK_init(&ctx->sk, 0, 0, 0);
    ctx->cur = -1;
    ctx->sp = 0;
    memset(ctx->regs, 0, 64);
}

/* Recomputes the length of the tree; *outLen gets it when not 0. With buf 0 it only measures. */
int OP_CTX_encode(OpCtx *ctx, unsigned char *buf, int *outLen, int bufLen)
{
    int len;
    int status;

    len = BER_ITEMS_recalc_length((&ctx->sk)->arr);
    if (outLen != 0)
        *outLen = len;
    if (buf == 0)
        return 0;
    status = BER_ITEMS_encode((&ctx->sk)->arr, buf, outLen, bufLen);
    return status;
}
