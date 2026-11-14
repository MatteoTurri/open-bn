/*
 * op_finish: final opcode of the RCOM BER interpreter.
 */

#include "../../ref/rcom-1.1.0/op_ctx.h"

/* Returns the operand `op1`: register (op1 & 0xF) if bit 0x80 is set, otherwise the literal. */
int op_finish(void *ctx, void *mode, int op1, void *op2)
{
	return (op1 & 0x80) ? *(int *)((op1 & 0xf) * 4 + (char *)ctx + 68) : op1;
}
