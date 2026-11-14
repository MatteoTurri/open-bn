/*
 * PK_encode_pkcs1_digest and PK_METH_rsa_pkcs1_SSLv23, the method table of RSA with the SSLv23
 * padding.
 */

#include "pk_int.h"
#include <string.h>

extern int pk_rsa_sslc_init(), pk_rsa_sslc_cleanup(), pk_rsa_sslc(), pk_rsa_get();
extern int rsa_padding_add_pkcs1_type_1(), rsa_padding_check_pkcs1_type_1();
extern int rsa_padding_add_SSLv23(), rsa_padding_check_SSLv23();

static RsaPadMeth pkRsaSSLv23Padding = {
    rsa_padding_add_pkcs1_type_1, rsa_padding_check_pkcs1_type_1,
    rsa_padding_add_SSLv23, rsa_padding_check_SSLv23
};
static PK_METH pkMethRsaPkcs1SSLv23Table = {
    pk_rsa_sslc_init, pk_rsa_sslc_cleanup, pk_unimplemented, pk_rsa_sslc,
    pk_unimplemented, pk_unimplemented, pk_rsa_get, &pkRsaSSLv23Padding
};
/* Builds the DER of a PKCS #1 DigestInfo by hand: SEQUENCE { SEQUENCE { OID, NULL }, OCTET STRING
 * digest }. */

int PK_encode_pkcs1_digest(void *outBuf, int *outLen, int maxOutLen,
                            void *algOid, void *digestData, int digestLen)
{
    int oidLen;
    int totalLen;
    unsigned char *buf;

    buf = (unsigned char *)outBuf;
    oidLen = strlen(algOid);
    totalLen = (oidLen + 8) + digestLen;
    *outLen = totalLen + 2;
    if (outLen == 0)
        return 0;
    if (maxOutLen < totalLen + 2)
        return 1;
    buf[0] = 0x30;
    buf[1] = (unsigned char)totalLen;
    totalLen = totalLen - (digestLen + 4);
    buf[2] = 0x30;
    buf[3] = (unsigned char)totalLen;
    buf[4] = 0x06;
    buf[5] = (unsigned char)oidLen;
    memcpy(buf + 6, algOid, oidLen);
    buf = buf + 6 + oidLen;
    buf[0] = 0x05;
    buf[1] = 0x00;
    buf[2] = 0x04;
    buf[3] = (unsigned char)digestLen;
    memcpy(buf + 4, digestData, digestLen);
    return 0;
}

PK_METH *PK_METH_rsa_pkcs1_SSLv23(void)
{
    return &pkMethRsaPkcs1SSLv23Table;
}
