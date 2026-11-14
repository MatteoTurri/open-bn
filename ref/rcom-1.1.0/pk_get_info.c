/*
 * PK_get_info: the getInfoFn of the method.
 */

#include "pk_int.h"

int PK_get_info(PK_CTX *ctxArg, int type, void *which, void *out)
{
    return (*ctxArg->methods->getInfoFn)(ctxArg, type, which, out);
}
