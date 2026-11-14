/*
 * op_mark: labels and navigation in the BER tree of the RCOM interpreter.
 */

#include "../../ref/rcom-1.1.0/op_ctx.h"

/*
 * Modes 0-3 record or jump to labels (mode 1/2 also choose whether the next
 * item goes under or after the current one); modes 8-10 move up to `op2` links
 * from the current item and store in register `op1` how many were taken.
 */
int op_mark(OpCtx *ctx, int mode, unsigned int op1, unsigned int op2, unsigned int op3)
{
	int imm;
	int n = 0;
	int saved = 0;
	BER_ITEM *item = 0;
	int *slot;

	if (mode & 8) {
		saved = n = OP_VAL(ctx, op2);
		if (ctx->cur < 0)
			return 2;
		item = (&ctx->sk)->arr + ctx->cur;
	} else {
		imm = op1 | (op2 << 8);
	}

	switch (mode) {
	case 0:
		ctx->marks[(op3 << 8) | op2] = ctx->cur;
		return 0;
	case 1:
		ctx->nest = 1;
		goto setmark;
	case 2:
		ctx->nest = 0;
		goto setmark;
	case 3:
		goto setmark;
	case 8:
		while (n > 0 && item->child != 0) {
			item = item->child;
			n--;
		}
		break;
	case 9:
		while (n > 0 && item->parent != 0) {
			item = item->parent;
			n--;
		}
		break;
	case 10:
		while (n > 0 && item->next != 0) {
			item = item->next;
			n--;
		}
		break;
	default:
		return 3;
	}

	if (op1 & 0x80) {
		slot = &OP_REG(ctx, op1);
		*slot = saved - n;
	} else if (n != 0) {
		return 2;
	}
	ctx->cur = item - ctx->sk.arr;
	return 0;

setmark:
	if (op1 != 0)
		ctx->cur = ctx->marks[op1];
	return 0;
}
