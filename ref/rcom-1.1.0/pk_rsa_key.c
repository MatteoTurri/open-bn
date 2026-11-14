/*
 * PK_encode_rsa_public_key, PK_decode_rsa_public_key and PK_decode_rsa_private_key: the PKCS #1 RSA
 * keys through the OP_CTX programs.
 */

#include "pk_int.h"

#include "op_ctx.h"

int PK_encode_rsa_public_key(void *eitems, unsigned char *buf, int *outLen, int bufLen)
{
    return OP_encode(0, OP_rsa_pkcs1_public_key_encode(), eitems, buf, outLen, bufLen);
}

int PK_decode_rsa_public_key(void *eitems, unsigned char *data, int len, int *consumed)
{
    return OP_decode(0, OP_rsa_pkcs1_public_key_decode(), eitems, data, len, consumed);
}

int PK_decode_rsa_private_key(void *eitems, unsigned char *data, int len, int *consumed)
{
    return OP_decode(0, OP_rsa_pkcs1_private_key_decode(), eitems, data, len, consumed);
}
