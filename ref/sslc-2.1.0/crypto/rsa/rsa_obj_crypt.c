/*
 * RSA_public_encrypt, RSA_private_encrypt and RSA_public_decrypt of the SSL-C fork.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "r_eitems.h"
#include "pk_int.h"
#include "rsa.h"
#include "rsa_int.h"

int RSA_public_encrypt(int flen, unsigned char *from, unsigned char *to,
                        RSA *rsa, int padding)
{
    int result = 0;
    void *ctx = 0;
    int outLen;
    int padtype = 0;

    if (padding == 2)
        padtype = 2048;

    ctx = rsa_get_pk_ctx(rsa);
    if (ctx == 0)
        goto end;

    if (PK_pub_enc(ctx, to, &outLen, (void *)-1, from, (void *)flen, (void *)padtype) != 0)
        goto end;
    result = outLen;
end:
    return result;
}

int RSA_private_encrypt(int flen, unsigned char *from, unsigned char *to,
                         RSA *rsa, int padding)
{
    int result = 0;
    void *ctx = 0;
    int outLen;
    int padtype = 0;

    if (padding == 2)
        padtype = 2048;

    ctx = rsa_get_pk_ctx(rsa);
    if (ctx == 0)
        goto end;

    if (PK_priv_enc(ctx, to, &outLen, (void *)-1, from, (void *)flen, (void *)padtype) != 0)
        goto end;
    result = outLen;
end:
    return result;
}

int RSA_public_decrypt(int flen, unsigned char *from, unsigned char *to,
                        RSA *rsa, int padding)
{
    int result = 0;
    void *ctx = 0;
    int outLen;
    int padtype = 0;

    if (padding == 2)
        padtype = 2048;

    ctx = rsa_get_pk_ctx(rsa);
    if (ctx == 0)
        goto end;

    if (PK_pub_dec(ctx, to, &outLen, (void *)-1, from, (void *)flen, (void *)padtype) != 0)
        goto end;
    result = outLen;
end:
    return result;
}
