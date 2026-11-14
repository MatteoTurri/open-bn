/*
 * PK_get_eitem: getInfoFn with type 257 (an R_EITEM), the arguments swapped.
 */

#include "pk_int.h"

int PK_get_eitem(PK_CTX *ctxArg, void *idx, void *buf, void *unused)
{
    return (*ctxArg->methods->getInfoFn)(ctxArg, 257, buf, idx);
}
