/*
 * PK_priv_enc: opFn with op 1.
 */

#include "pk_int.h"

int PK_priv_enc(PK_CTX *ctxArg, void *outBuf, void *outLenPtr, void *bufCap, void *in, void *inLen, void *padFlags)
{
    return (*ctxArg->methods->opFn)(ctxArg, 1, outBuf, outLenPtr, bufCap, in, inLen, padFlags);
}
