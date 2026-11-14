/*
 * DH_set, DH_get, DH_generate_key, DH_compute_key and DH_params_dup of SSL-C: the DH object keeps its
 * key in an R_EITEMS list and works through a PK_CTX.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "r_eitems.h"
#include "pk_int.h"
#include "dh.h"

int DH_set(DH *dh, int component, unsigned char *data, int len)
{
    int ret;

    ret = R_EITEMS_add(dh->eitems, 18, component, 2, (int)data, len, 18);
    return ret;
}

int DH_get(DH *dh, int component, unsigned char **data, int *len)
{
    int result = 1;
    R_EITEM *item = 0;
    int found;
    R_EITEM local;

retry:
    found = R_EITEMS_find_item(dh->eitems, 18, component, 0, &item, 0);
    if (found == 0) {
        if (data != 0)
            *data = item->data;
        if (len != 0)
            *len = item->length;
        return 0;
    } else {
        if (dh->pk_ctx != 0 && result != 0) {
        R_EITEM_init(&local);
        result = 0;
        if (PK_get_eitem(dh->pk_ctx, &local, (void *)component, 0) != 0)
            goto tail;

        if (local.length == 0) {
            R_EITEM_free(&local);
            goto tail;
        }

        if (R_EITEMS_add(dh->eitems, 18, component, 2, (int)local.data, local.length, 18) != 0) {
            R_EITEM_free(&local);
            goto tail;
        }
        R_EITEM_free(&local);
        goto retry;
        }

tail:
        if (data != 0)
            *data = 0;
        if (len != 0)
            *len = 0;
        return 1;
    }
}

int DH_generate_key(DH *dh)
{
    int result;

    if ((dh->flags & 0x100) == 0) {
        dh->flags = dh->flags | 0x100;
        PK_CTX_set(dh->pk_ctx, dh->meth, dh->eitems, 0, 0);
    }
    result = PK_phase1(dh->pk_ctx, 0);
    return (result == 0);
}

int DH_compute_key(unsigned char *key, int *out_len, unsigned int max_len, unsigned char *pub,
                   unsigned int pub_len, DH *dh)
{
    int result;
    int secret_len;

    if ((dh->flags & 0x100) == 0) {
        dh->flags = dh->flags | 0x100;
        PK_CTX_set(dh->pk_ctx, dh->meth, dh->eitems, 0, 0);
    }

    result = PK_phase2(dh->pk_ctx, key, &secret_len, max_len, pub, pub_len, 0);
    if (out_len != 0)
        *out_len = secret_len;
    return (result == 0) ? secret_len : 0;
}

DH *DH_params_dup(DH *dh)
{
    DH *ret = 0;

    ret = DH_new_meth(dh->meth);
    if (ret == 0)
        goto err;

    if (!R_EITEMS_dup(ret->eitems, dh->eitems, 18))
        goto err;

    R_EITEMS_delete(ret->eitems, 18, 3, 0);
    R_EITEMS_delete(ret->eitems, 18, 4, 0);
    return ret;

err:
    if (ret != 0)
        DH_free(ret);
    return 0;
}
