/*
 * op_if and op_maths: conditional jump and arithmetic opcodes of the RCOM
 * BER interpreter.
 */

#include "../../ref/rcom-1.1.0/op_ctx.h"

/*
 * Compares the operand `op1` with 0 (mode 0 ==, 1 !=, 2 <=, 3 >=, 4 <, 5 >)
 * and, if true, jumps to label `label | labelHi << 8`. Mode 6 saves the pc in a
 * register and jumps (a call), mode 7 sets the pc to the operand (a return).
 */
int op_if(OpCtx *ctx, int mode, unsigned int op1, int label, unsigned int labelHi)
{
	int operand;
	int cmpResult;
	int geOne;
	int signBit;
	int isZero;

	label = label | (labelHi << 8);

	operand = OP_VAL(ctx, op1);

	geOne = operand >= 1;
	signBit = operand < 0;
	isZero = operand == 0;
	cmpResult = 0;

	switch (mode) {
	case 0:
		cmpResult = isZero;
		break;
	case 1:
		cmpResult = isZero == 0;
		break;
	case 2:
		cmpResult = isZero;
	case 4:
		cmpResult |= signBit;
		break;
	case 3:
		cmpResult = isZero;
	case 5:
		cmpResult |= geOne;
		break;
	case 6:
		cmpResult = 1;
		if (!(op1 & 0x80))
			return 2;
		OP_REG(ctx, op1) = ctx->pc;
		break;
	case 7:
		cmpResult = 0;
		ctx->pc = operand;
		break;
	default:
		return 2;
	}

	if (cmpResult) {
		if (label >= ctx->labelCount)
			return 2;
		ctx->pc = ctx->labels[label];
	}
	return 0;
}

/*
 * Register `dst` = lhs <op> rhs, with op by mode: + - * / % | & ^.
 * Division and modulo by zero return 5.
 */
int op_maths(OpCtx *ctx, int mode, unsigned int dst, unsigned int lhs, unsigned int rhs)
{
	int lhsVal;
	int rhsVal;
	int *slot;

	if (!(dst & 0x80))
		return 2;

	slot = &OP_REG(ctx, dst);
	lhsVal = OP_VAL(ctx, lhs);
	rhsVal = OP_VAL(ctx, rhs);

	switch (mode) {
	case 0:
		*slot = lhsVal + rhsVal;
		break;
	case 1:
		*slot = lhsVal - rhsVal;
		break;
	case 2:
		*slot = lhsVal * rhsVal;
		break;
	case 3:
		if (rhsVal == 0)
			return 5;
		*slot = lhsVal / rhsVal;
		break;
	case 4:
		if (rhsVal == 0)
			return 5;
		*slot = lhsVal % rhsVal;
		break;
	case 5:
		*slot = lhsVal | rhsVal;
		break;
	case 6:
		*slot = lhsVal & rhsVal;
		break;
	case 7:
		*slot = lhsVal ^ rhsVal;
		break;
	default:
		return 2;
	}
	return 0;
}
