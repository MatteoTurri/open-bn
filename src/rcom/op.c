/*
 * Opcodes op_label, op_asn1_encode and op_call of the RCOM BER interpreter.
 */

#include "../../ref/rcom-1.1.0/op_ctx.h"

/* Ignores its operands and returns 0. */
int op_label(void *ctx, void *mode, void *op1, void *op2)
{
	return 0;
}


/* DER contents of a BOOLEAN (mode 4). */
static const unsigned char s_derTrue[1] = { 0xff };
static const unsigned char s_derFalse[1] = { 0x00 };

/*
 * Builds or updates a BER item. Without bit 3 of `mode` a new item is
 * allocated and finally linked to the tree (under the current item if
 * nest is set, otherwise after it); with bit 3 the current item is
 * changed in place.
 */
int op_asn1_encode(OpCtx *ctx, int mode, unsigned int op1, unsigned int op2, unsigned int op3)
{
	int status;
	int idx;
	int val1;
	int val2;
	int val3;
	BER_ITEM *item;
	BER_ITEM *prev;

	val1 = OP_VAL(ctx, op1);
	val2 = OP_VAL(ctx, op2);
	val3 = OP_VAL(ctx, op3);

	if (!(mode & 8)) {
		status = BER_ITEMS_SK_get(&ctx->sk, &idx);
		if (status != 0)
			return status;
		item = (&ctx->sk)->arr + idx;
	} else {
		if (ctx->cur < 0)
			return 2;
		item = (&ctx->sk)->arr + ctx->cur;
	}

	switch (mode) {
	case 0:
		ctx->curItem = item;
		if (ctx->callback == 0)
			return 4;
		status = ctx->callback(ctx, ctx->arg, ctx->regs, val1, val2, val3);
		break;
	case 1:
		ctx->curItem = item;
		if (ctx->callback == 0)
			return 4;
		status = ctx->callback(ctx, ctx->arg, ctx->regs, 2, val1, val2);
		if (item->len != 0) {
			if ((unsigned char)(item->data[0] & 0x80))
				BER_ITEM_set_prefix_byte(item, 0);
		}
		break;
	case 2:
		BER_ITEM_set_all(item, val1, val2, 0, 0, val3, 0);
		break;
	case 3:
		BER_ITEM_set_long(item, val1, 0);
		break;
	case 4:
		BER_ITEM_set_all(item, 0, 1, val1 ? s_derTrue : s_derFalse, 1, 0, 0);
		return 0;
	case 8:
		item->tag = val1;
		item->flags = (val2 & ~0x3f) | (val3 & 0x2f);
		return 0;
	case 9:
		BER_ITEM_set_prefix_byte(item, val1);
		return 0;
	default:
		return 3;
	}

	if (!(mode & 8)) {
		if (ctx->cur >= 0) {
			prev = (&ctx->sk)->arr + ctx->cur;
			if (ctx->nest)
				BER_ITEMS_under(prev, item);
			else
				BER_ITEMS_append(prev, item);
		}
		ctx->cur = idx;
		ctx->nest = 0;
	}
	return 0;
}

/*
 * Calls the installed handler with the registers and three operands. In
 * mode 1 the current item is advanced by one value byte (len--,
 * data++) for the duration of the call.
 */
int op_call(OpCtx *ctx, int mode, unsigned int op1, unsigned int op2, unsigned int op3)
{
	int status;
	int borrow;
	int val1;
	int val2;
	int val3;
	BER_ITEM *rec = 0;

	if (ctx->callback == 0)
		return 2;

	val1 = OP_VAL(ctx, op1);
	val2 = OP_VAL(ctx, op2);
	val3 = OP_VAL(ctx, op3);

	if (ctx->cur < 0)
		ctx->curItem = 0;
	else
		ctx->curItem = (&ctx->sk)->arr + ctx->cur;

	borrow = mode == 1;
	if (borrow) {
		rec = ctx->curItem;
		if (rec->len == 0)
			return 3;
		rec->len--;
		rec->data++;
	}

	status = ctx->callback(ctx, ctx->arg, ctx->regs, val1, val2, val3);

	if (borrow) {
		rec->len++;
		rec->data--;
	}
	return status;
}
