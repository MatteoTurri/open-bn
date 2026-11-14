/*
 * op_asn1_check: checks the header of the current BER item (RCOM interpreter).
 */

#include "../../ref/rcom-1.1.0/op_ctx.h"

/*
 * Checks the current item against three operands and builds an error mask
 * (bit 0 class, bit 1 length/tag, bit 2 masked tag). `mode` is a set of flags:
 * bits 0-1 = expected class, bit 2 = `op2` is a bitmap of allowed tags, bit 3 =
 * store the mask in register 0 instead of returning an error.
 */
int op_asn1_check(OpCtx *ctx, int mode, unsigned int op1, unsigned int op2, unsigned int op3)
{
	BER_ITEM *rec;
	int expClass;
	int result;
	int val1;
	int val2;
	int val3;

	if (ctx->cur < 0)
		return 2;

	rec = (&ctx->sk)->arr + ctx->cur;
	val1 = OP_VAL(ctx, op1);
	val2 = OP_VAL(ctx, op2);
	val3 = OP_VAL(ctx, op3);

	if (!(mode & 4)) {
		expClass = (mode & 3) << 6;
		result = expClass != (unsigned char)(rec->flags & 0xc0);
		result = (val1 != rec->tag) ? (result | 2) : result;
		result = (((rec->flags & ~0xc0) & val3) != val2) ? (result | 4) : result;
	} else {
		result = val1 != (unsigned char)(rec->flags & 0xc0);
		val3 = 1 << rec->tag;
		result = ((val3 & val2) != val3) ? (result | 2) : result;
	}

	if (mode & 8) {
		ctx->regs[0] = result;
		return 0;
	} else {
		return result ? 3 : 0;
	}
}
