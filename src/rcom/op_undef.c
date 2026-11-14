/*
 * op_undef: handler of the undefined opcodes of the RCOM BER interpreter.
 */

#include "../../ref/rcom-1.1.0/op_ctx.h"

/* Ignores its operands and returns 2. */
int op_undef(void *ctx, void *mode, void *op1, void *op2)
{
	return 2;
}
