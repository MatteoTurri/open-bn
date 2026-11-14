/*
 * PK_CTX_new and the default method table, all pk_unimplemented.
 */

#include "pk_int.h"


static PK_METH pkMethDefaultTable = {
    pk_unimplemented, pk_unimplemented, pk_unimplemented, pk_unimplemented,
    pk_unimplemented, pk_unimplemented, pk_unimplemented, 0
};

void *PK_CTX_new(void)
{
    PK_CTX *ctx;

    if ((ctx = (PK_CTX *)R_malloc(16)) == 0)
        return 0;
    ctx->methods = &pkMethDefaultTable;
    ctx->eitems = 0;
    ctx->impl = 0;
    ctx->flags = 1;
    return ctx;
}
