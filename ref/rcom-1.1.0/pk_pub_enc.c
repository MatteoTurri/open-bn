/*
 * PK_pub_enc: opFn with op 3.
 */

#include "pk_int.h"

int PK_pub_enc(PK_CTX *ctxArg, void *outBuf, void *outLenPtr, void *bufCap, void *in, void *inLen, void *padFlags)
{
    return (*ctxArg->methods->opFn)(ctxArg, 3, outBuf, outLenPtr, bufCap, in, inLen, padFlags);
}
