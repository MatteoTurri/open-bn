/*
 * A_SetModularOperations: a helper of the BSAFE algae layer (A_*).
 */

#include "cmp.h"
#include "alg.h"

/* Fills the table of the 7 modular operations of an algae context: each slot
 * takes the pointer passed by the caller or, when it is null, the default CMP_*
 * implementation (CMP_ModExp, CMP_ModExpCRT, CMP_ModExpMultiPrimeCRT,
 * CMP_ModInvert, CMP_ModularReduce, CMP_ComputeGCD, CMP_ModMultiply, in slot
 * order). */
void A_SetModularOperations(AModOpFn *ops, AModOpFn modExp, AModOpFn modExpCRT,
                            AModOpFn modExpMultiPrimeCRT, AModOpFn modInvert,
                            AModOpFn modularReduce, AModOpFn computeGCD,
                            AModOpFn modMultiply)
{
    if (modExp != 0)
        ops[0] = modExp;
    else
        ops[0] = CMP_ModExp;
    if (modExpCRT != 0)
        ops[1] = modExpCRT;
    else
        ops[1] = CMP_ModExpCRT;
    if (modExpMultiPrimeCRT != 0)
        ops[2] = modExpMultiPrimeCRT;
    else
        ops[2] = CMP_ModExpMultiPrimeCRT;
    if (modInvert != 0)
        ops[3] = modInvert;
    else
        ops[3] = CMP_ModInvert;
    if (modularReduce != 0)
        ops[4] = modularReduce;
    else
        ops[4] = CMP_ModularReduce;
    if (computeGCD != 0)
        ops[5] = computeGCD;
    else
        ops[5] = CMP_ComputeGCD;
    if (modMultiply != 0)
        ops[6] = modMultiply;
    else
        ops[6] = CMP_ModMultiply;
}
