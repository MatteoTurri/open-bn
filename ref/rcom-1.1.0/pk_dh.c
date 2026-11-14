/*
 * pk_dh_sslc_init: the DH implementation behind the DH PK_METH.
 */
#include "pk_dh_impl.h"

int pk_dh_sslc_init(PK_CTX *ctxArg, void *eitems, int unused3, int unused4)
{
    R_EITEM *item;
    DhImpl *dh;
    int n;
    int ret;
    BN_CTX *ctx;

    ret = 1;
    ctx = 0;

    if ((ctx = BN_CTX_new()) == 0)
        return 1;

    dh = ctxArg->impl;
    if (dh != 0)
        pk_dh_sslc_cleanup(ctxArg);

    dh = (DhImpl *)R_malloc(104);
    if (dh == 0)
        return 1;

    memset(dh, 0, 104);
    ctxArg->impl = dh;

    BN_init(&dh->p);
    BN_init(&dh->g);
    BN_init(&dh->pubKey);
    BN_init(&dh->privKey);
    dh->flags = 0;

    n = 0;
    R_EITEMS_find_item(eitems, 1, 2, &n, &item, 4);
    dh->rng = (item != 0) ? item->data : 0;

    R_EITEMS_find_item(eitems, 1, 3, &n, &item, 4);
    dh->surrender = (item != 0) ? item->data : 0;

    n = 0;
    R_EITEMS_find_item(eitems, 18, 1, &n, &item, 4);
    if (item == 0)
        goto done;

    BN_bin2bn(item->data, item->length, &dh->p);

    if ((&dh->p)->top <= 0 || ((*(unsigned int *)(&dh->p)->d) & 1) == 0)
        goto done;

    R_EITEMS_find_item(eitems, 18, 2, &n, &item, 4);
    if (item == 0)
        goto done;

    BN_bin2bn(item->data, item->length, &dh->g);

    dh->flags = dh->flags | 1;

    BN_ME_CTX_new(0, &dh->modExpCtx);

    if (BN_ME_CTX_set(dh->modExpCtx, &dh->p, 1, 0, ctx) != 0)
        goto done;

    R_EITEMS_find_item(eitems, 18, 5, &n, &item, 4);

    n = BN_num_bits(&dh->p);
    if (item == 0) {
        dh->privBits = n;
    } else {
        dh->privBits = (n < item->length) ? n : item->length;
    }

    if (R_EITEMS_find_item(eitems, 18, 4, &n, &item, 4) == 0) {
        BN_bin2bn(item->data, item->length, &dh->privKey);
        dh->flags = dh->flags | 4;
    }

    if (R_EITEMS_find_item(eitems, 18, 3, &n, &item, 4) == 0) {
        BN_bin2bn(item->data, item->length, &dh->pubKey);
        dh->flags = dh->flags | 2;
    }

    ret = 0;

done:
    if (ctx != 0)
        BN_CTX_free(ctx);

    return ret;
}
