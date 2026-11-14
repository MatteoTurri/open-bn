/*
 * The OP_CTX interpreter: runs a program of 32-bit code words over a tree of
 * BER items (decode, then run; or run, then encode).
 */
#ifndef RCOM_OP_CTX_H
#define RCOM_OP_CTX_H

#include <string.h>

#include "ber_item.h"

#include "r_com.h"

typedef struct OpCtx OpCtx;

/* Handler of an opcode: the high nibble of the low byte of a code word picks
   it in OpCtx.funcs, the low nibble is the mode, the three high bytes are the
   operands. An operand with bit 0x80 set names register (operand & 0xF),
   otherwise it is a literal. */
typedef int (*OpHandlerFn)(OpCtx *ctx, unsigned int mode, unsigned int operand1,
                           unsigned int operand2, unsigned int operand3);

struct OpCtx {
    /* 0x00 */ OpHandlerFn *funcs;  /* OP_all_funcs or OP_read_funcs */
    /* 0x04 */ unsigned char pad_04[4];  /* unused, but part of the layout */
    /* 0x08 */ BER_ITEMS_SK sk;  /* the item tree */
    /* 0x18 */ int codeLen;  /* code words, the final 0 included */
    /* 0x1C */ unsigned int *code;
    /* 0x20 */ int pc;  /* index of the next code word */
    /* 0x24 */ int markCount;
    /* 0x28 */ int labelCount;
    /* 0x2C */ unsigned char pad_2C[4];  /* unused, but part of the layout */
    /* 0x30 */ int sp;  /* words on the argument stack */
    /* 0x34 */ int stackCap;  /* words allocated, grows by 6 */
    /* 0x38 */ unsigned int *stack;
    /* 0x3C */ int *marks;  /* item indices saved by op_mark */
    /* 0x40 */ unsigned int *labels;  /* code index of each label, 0xFFFF = none */
    /* 0x44 */ int regs[16];
    /* 0x84 */ int (*callback)();  /* set by OP_CTX_set_function */
    /* 0x88 */ void *arg;  /* argument of OP_CTX_run, passed to the callback */
    /* 0x8C */ int cur;  /* index of the current item, -1 = none */
    /* 0x90 */ int nest;  /* the next item goes under the current one */
    /* 0x94 */ BER_ITEM *curItem;  /* &sk.arr[cur], for the callback */
    /* 0x98 */ int flags;  /* bit 0: allocated, freed by OP_CTX_free; bit 1: call the callback before each word */
}; /* size = 0x9C */

/* The register named by an operand, and the value of an operand: that
   register when bit 0x80 is set, otherwise the operand itself. */
#define OP_REG(ctx, op) (*((ctx)->regs + ((op) & 0xf)))
#define OP_VAL(ctx, op) (((op) & 0x80) ? OP_REG(ctx, op) : (int)(op))

void *OP_all_funcs(void);
void *OP_read_funcs(void);

int OP_CTX_init_meth(OpCtx *ctx, void *funcs);
int OP_CTX_init(OpCtx *ctx);
int OP_CTX_init_read(OpCtx *ctx);
int OP_CTX_set_function(OpCtx *ctx, void *fn);
void OP_CTX_reset(OpCtx *ctx);
void OP_CTX_free(OpCtx *ctx);
int OP_CTX_code(OpCtx *ctx, unsigned int *code);
int OP_CTX_decode(OpCtx *ctx, unsigned char *data, int len, int *consumed);
int OP_CTX_run(OpCtx *ctx, void *arg);
int OP_CTX_encode(OpCtx *ctx, unsigned char *buf, int *outLen, int bufLen);

int OP_default_callback(OpCtx *ctx, void *items, int *out, int code, int key1, int key2);

/* op_ctx.c */
int OP_decode(OpCtx *ctxArg, unsigned int *code, void *runArg, unsigned char *data, int len,
              int *consumed);
int OP_encode(OpCtx *ctxArg, unsigned int *code, void *runArg, unsigned char *buf, int *outLen,
              int bufLen);

/* op_rsa_privdec.c */
void *OP_rsa_pkcs1_private_key_decode(void);

/* op_rsa_pubdec.c */
void *OP_rsa_pkcs1_public_key_decode(void);

/* op_rsa_pubenc.c */
void *OP_rsa_pkcs1_public_key_encode(void);

#endif
