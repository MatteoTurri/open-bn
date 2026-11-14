/*
 * op_asn1_get: reads fields of the current BER item into the registers of
 * the RCOM interpreter.
 */

#include <string.h>

#include "../../ref/rcom-1.1.0/op_ctx.h"


/*
 * Mode 0: class and constructed bit, length and tag into the registers named
 * by op1, op2, op3. Mode 1: the integer value of the item into register `op1`, read
 * from a 16-byte copy when `op2` is non-zero (the item is then marked as long
 * form, tag = 2).
 */
int op_asn1_get(OpCtx *ctx, int mode, unsigned int op1, unsigned int op2, unsigned int op3)
{
	BER_ITEM *rec;
	int sel;
	int status;
	unsigned char copy[16];
	BER_ITEM *src;

	if (ctx->cur < 0)
		return 2;

	rec = (&ctx->sk)->arr + ctx->cur;

	if (mode == 0) {
		if (op1 & 0x80)
			OP_REG(ctx, op1) = (unsigned char)(rec->flags & 0xc0);
		if (op2 & 0x80)
			OP_REG(ctx, op2) = rec->tag;
		if (op3 & 0x80)
			OP_REG(ctx, op3) = (unsigned char)(rec->flags & 0x2f);
		return 0;
	} else if (mode == 1) {
		if (!(op1 & 0x80))
			return 2;
		sel = OP_VAL(ctx, op2);
		if (sel != 0) {
			memcpy(copy, rec, 16);
			rec->tag = 2;
			src = (BER_ITEM *)copy;
		} else {
			src = rec;
		}
		status = BER_ITEM_get_long(src, &OP_REG(ctx, op1));
		return status;
	}
	return 2;
}
