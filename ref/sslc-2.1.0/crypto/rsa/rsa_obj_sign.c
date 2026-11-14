/*
 * RSA_sign of the SSL-C fork.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "r_eitems.h"
#include "pk_int.h"
#include "rsa.h"
#include "rsa_int.h"

/* RSA_sign: maps the digest NID to its OID, then PK_encode_pkcs1_digest and PK_sign. */


int RSA_sign(int type, unsigned char *m, unsigned int m_len,
             unsigned char *sigret, unsigned int *siglen, RSA *rsa)
{
    int result = -1;
    int info;
    int encLen;
    void *ctx = 0;
    void *algOid = 0;

    ctx = rsa_get_pk_ctx(rsa);
    if (ctx == 0)
        goto end;

    if (PK_get_info(ctx, 259, 0, &info) != 0)
        goto end;

    switch (type) {
    case 4:
        algOid = "\x2a\x86\x48\x86\xf7\x0d\x02\x05";
        break;
    case 41:
        algOid = "\x2b\x0e\x03\x02\x12";
        break;
    case 64:
        algOid = "\x2b\x0e\x03\x02\x1a";
        break;
    case 3:
        algOid = "\x2a\x86\x48\x86\xf7\x0d\x02\x02";
        break;
    case 117:
        algOid = "\x2b\x24\x03\x02\x01";
        break;
    default:
        goto end;
    }

    PK_encode_pkcs1_digest(sigret, &encLen, 10000, algOid, m, m_len);

    if (PK_sign(ctx, sigret, siglen, (void *)info, sigret, (void *)encLen, 0) != 0)
        goto end;
    result = info;

end:
    return result;
}
