/*
 * The hooks of the RSA sign and verify callbacks of the SSL-C fork.
 */

#include <stdio.h>
#include "rsa.h"

/* default hooks: generic function pointers, cast by the callers. */
static void *rsa_sign_hook = 0;
static void *rsa_asn1_octet_sign_hook = 0;
static void *rsa_verify_hook = 0;
static void *rsa_asn1_octet_verify_hook = 0;

void RSA_set_sign_cb(void *cb)
{
    rsa_sign_hook = cb;
}

void RSA_set_ASN1_OCTET_sign_cb(void *cb)
{
    rsa_asn1_octet_sign_hook = cb;
}

void RSA_set_verify_cb(void *cb)
{
    rsa_verify_hook = cb;
}

void RSA_set_ASN1_OCTET_verify_cb(void *cb)
{
    rsa_asn1_octet_verify_hook = cb;
}

int RSA_sign_ASN1_OCTET_STRING(int type, unsigned char *m, unsigned int m_len,
                                unsigned char *sigret, unsigned int *siglen, RSA *rsa)
{
    int ret = 0;

    if (rsa_asn1_octet_sign_hook != 0) {
        ret = (*(int (*)(int, unsigned char *, unsigned int, unsigned char *,
                          unsigned int *, void *))rsa_asn1_octet_sign_hook)
              (type, m, m_len, sigret, siglen, rsa);
    }
    return ret;
}

int RSA_verify_ASN1_OCTET_STRING(int type, unsigned char *m, unsigned int m_len,
                                  unsigned char *sigbuf, unsigned int siglen, RSA *rsa)
{
    int ret = 0;

    if (rsa_asn1_octet_verify_hook != 0) {
        ret = (*(int (*)(int, unsigned char *, unsigned int, unsigned char *,
                          unsigned int, void *))rsa_asn1_octet_verify_hook)
              (type, m, m_len, sigbuf, siglen, rsa);
    }
    return ret;
}
