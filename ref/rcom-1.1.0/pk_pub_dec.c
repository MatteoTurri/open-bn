/*
 * PK_pub_dec: opFn with op 4.
 */

#include "pk_int.h"

int PK_pub_dec(PK_CTX *ctxArg, void *outBuf, void *outLenPtr, void *bufCap, void *in, void *inLen, void *padFlags)
{
    return (*ctxArg->methods->opFn)(ctxArg, 4, outBuf, outLenPtr, bufCap, in, inLen, padFlags);
}
