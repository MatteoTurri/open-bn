/* SSL-C: the RSA of the fork keeps its key as an R_EITEMS list and its method as a PK_METH;
 * pk_ctx is created on first use, under the lock. */

#include <stdio.h>
#include "cryptlib.h"
#include "r_eitems.h"
#include "pk_int.h"
#include "rsa.h"
#include "rsa_int.h"

/* ---- costruttore/distruttore reali ---- */


void RSA_free(RSA *r)
{
    int refs;

    if (r == 0)
        return;

#line 219 /* the line number is part of the binary */
    refs = R_locked_add(&r->references, -1, 9, __FILE__, __LINE__);
    if (refs > 0)
        return;

    if (r->pk_ctx != 0)
        PK_CTX_free(r->pk_ctx);
    if (r->eitems != 0)
        R_EITEMS_free(r->eitems);
    if (r->extra_buf != 0)
        R_free(r->extra_buf);
    R_free(r);
}

static PK_METH *(*rsa_default_meth_hook)(void) = PK_METH_rsa_pkcs1_SSLv23;

RSA *RSA_new(void)
{
    return RSA_new_meth((*rsa_default_meth_hook)());
}

void RSA_set_default_meth(PK_METH *(*fn)(void))
{
    rsa_default_meth_hook = fn;
}

/* ---- accessor eitems ---- */

int RSA_get(RSA *r, int component, unsigned char **data, int *len)
{
    int result = 0;
    R_EITEM *item;

    result = R_EITEMS_find_item(r->eitems, 16, component, 0, &item, 0);
    if (item != 0) {
        if (data != 0)
            *data = item->data;
        if (len != 0)
            *len = item->length;
        return 0;
    } else {
        if (data != 0)
            *data = 0;
        if (len != 0)
            *len = 0;
        return 0;
    }
}

int RSA_set(RSA *r, int component, unsigned char *data, int len)
{
    int ret = 0;

    if (R_EITEMS_add(r->eitems, 16, component, 0, (int)data, len, 18) == 0)
        ret = 1;
    return ret;
}

int RSA_size(RSA *r)
{
    int result = 0;
    R_EITEM *item = 0;
    int index = 0;

    R_EITEMS_find_item(r->eitems, 16, 16, &index, &item, 0);
    if (item != 0) {
        result = item->length;
        if (item->data[0] == 0)
            result -= 1;
    }
    return result;
}

