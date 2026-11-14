/*
 * PK_sign: opFn with op 10.
 */

#include "pk_int.h"

int PK_sign(PK_CTX *ctxArg, void *outBuf, void *outLenPtr, void *bufCap, void *in, void *inLen, void *padFlags)
{
    return (*ctxArg->methods->opFn)(ctxArg, 10, outBuf, outLenPtr, bufCap, in, inLen, padFlags);
}
