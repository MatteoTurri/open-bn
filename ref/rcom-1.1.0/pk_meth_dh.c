/*
 * PK_METH_dh: the method table of DH.
 */

#include "pk_int.h"

extern int pk_dh_sslc_init(), pk_dh_sslc_cleanup(), pk_dh_sslc_generate_key();
extern int pk_dh_sslc_compute_key(), pk_dh_sslc_final(), pk_dh_sslc_get();

static PK_METH pkMethDhTable = {
    pk_dh_sslc_init, pk_dh_sslc_cleanup, pk_dh_sslc_generate_key, pk_dh_sslc_compute_key,
    pk_dh_sslc_final, pk_unimplemented, pk_dh_sslc_get, 0
};
PK_METH *PK_METH_dh(void)
{
    return &pkMethDhTable;
}
