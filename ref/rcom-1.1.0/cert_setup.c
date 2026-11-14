/*
 * The default methods of the SSLCERT layer and SSLCERT_normal_setup, which registers them.
 */

#include <stdio.h>
#include "asn1.h"
#include "buffer.h"
#include "x509.h"
#include "pem.h"
#include "ssl_locl.h"
#include "sslcert.h"


static int sslcert_get_serialNumber_helper(cert, out)
    X509 *cert;
    sslcert_info *out;
{
    int ret;
    ASN1_STRING *value;

    ret = 0;
    value = (ASN1_STRING *)X509_get_serialNumber(cert);
    if (value != 0 && out != 0) {
        out->len = value->length;
        out->data = value->data;
        out->type = value->type;
        ret = 1;
    }
    return ret;
}
/* The case 29 and 30 methods call a callback that the caller passes. */
static int sslcert_do_cb_helper(target, info, cb, cbArg1, cbArg2)
    void *target;
    sslcert_info *info;
    int (*cb)();
    void *cbArg1;
    void *cbArg2;
{
    int ret;

    ret = 0;
    if (cb != 0)
        ret = (*cb)(target, info->len, info->type, cbArg1, cbArg2);
    return ret;
}
/* Case 30, the verification of a chain: builds an X509_STORE_CTX on the stack with the depth and the
 * SSL, then calls the application callback if any, X509_verify_cert otherwise; *perr gets
 * ctx->error. */
static int sslcert_do_verify_cb_helper(s, sk, store, cb, cbArg, perr)
    SSL *s;
    STACK *sk;
    X509_STORE *store;
    int (*cb)();
    void *cbArg;
    long *perr;
{
    X509_STORE_CTX ctx;
    X509 *cert;
    int ret;

    ret = 0;
    cert = (X509 *)sk->data[0];
    X509_STORE_CTX_init(&ctx, store, cert, sk);
    if (SSL_get_verify_depth(s) >= 0)
        X509_STORE_CTX_set_depth(&ctx, SSL_get_verify_depth(s));
    X509_STORE_CTX_set_ex_data(&ctx, SSL_get_ex_data_X509_STORE_CTX_idx(), (char *)s);
    if (cb != 0)
        ret = (*cb)(&ctx, cbArg);
    else
        ret = X509_verify_cert(&ctx, cbArg);
    if (perr != 0)
        *perr = ctx.error;
    X509_STORE_CTX_cleanup(&ctx);
    return ret;
}
static int sslcert_get_notAfter_helper(cert, out)
    X509 *cert;
    sslcert_info *out;
{
    int ret;
    ASN1_STRING *value;

    ret = 0;
    value = (ASN1_STRING *)X509_get_notAfter(cert);
    if (value != 0 && out != 0) {
        out->len = value->length;
        out->data = value->data;
        out->type = value->type;
        ret = 1;
    }
    return ret;
}
static int sslcert_get_notBefore_helper(cert, out)
    X509 *cert;
    sslcert_info *out;
{
    int ret;
    ASN1_STRING *value;

    ret = 0;
    value = (ASN1_STRING *)X509_get_notBefore(cert);
    if (value != 0 && out != 0) {
        out->len = value->length;
        out->data = value->data;
        out->type = value->type;
        ret = 1;
    }
    return ret;
}
/* Case 23: the length (i2d_X509_NAME) and the DER bytes of an X509_NAME, type 45. */
static int sslcert_name_get_info_helper(name, out)
    void *name;
    sslcert_info *out;
{
    int ret;
    X509_NAME *nm;

    ret = 0;
    nm = (X509_NAME *)name;
    if (i2d_X509_NAME(nm, 0) > 0 && out != 0 && nm->bytes != 0) {
        out->len = nm->bytes->length;
        out->data = nm->bytes->data;
        out->type = 45;
        ret = 1;
    }
    return ret;
}
/* Case 26: the info of the OID and of the data of an X509_NAME_ENTRY, for
 * SSLCERT_NAME_ENTRY_get_info. */
static int sslcert_name_entry_get_info_helper(entry, out_oid, out_data)
    void *entry;
    sslcert_info *out_oid;
    sslcert_info *out_data;
{
    int ret;
    X509_NAME_ENTRY *e;

    ret = 0;
    e = (X509_NAME_ENTRY *)entry;
    if (out_oid != 0) {
        out_oid->type = 6;
        out_oid->data = e->object->data;
        out_oid->len = e->object->length;
        ret = 1;
    }
    if (out_data != 0) {
        out_data->type = e->value->type;
        out_data->data = e->value->data;
        out_data->len = e->value->length;
        ret = 1;
    }
    return ret;
}
/* Case 39: stores the callback in the verify slot of the X509_STORE and returns 1. */
static int sslcert_store_set_verify_cb_helper(store, cb)
    X509_STORE *store;
    int (*cb)();
{
    store->verify_cb = cb;
    return 1;
}
/* Case 54: decodes type DER X509_PUBKEY and installs its key in *a, freeing the previous one and
 * taking type reference to the new one; the first argument is not used. */
static EVP_PKEY *sslcert_pkey_from_pubkey_binary_helper(type, a, pp, length)
    int type;
    EVP_PKEY **a;
    unsigned char **pp;
    long length;
{
    X509_PUBKEY *pub;
    EVP_PKEY *pkey;

    pkey = 0;
    pub = d2i_X509_PUBKEY(0, pp, length);
    if (pub == 0)
        return 0;
    pkey = X509_PUBKEY_get(pub);
    if (a != 0 && pkey != 0) {
        if (*a != 0)
            SSLCERT_PKEY_free(*a);
        *a = pkey;
        SSLCERT_PKEY_reference_inc(*a);
    }
    X509_PUBKEY_free(pub);
    return pkey;
}


int SSLCERT_normal_setup()
{
    sslcert_set_clear();
    SSLCERT_set_flag(0, 1);
    SSLCERT_set_flag(1, 1);
    SSLCERT_set_field(1, (void *)X509_new);
    SSLCERT_set_field(2, (void *)X509_free);
    SSLCERT_set_field(3, (void *)X509_verify);
    SSLCERT_set_field(4, (void *)X509_digest);
    SSLCERT_set_field(5, (void *)i2d_X509);
    SSLCERT_set_field(6, (void *)d2i_X509);
    SSLCERT_set_field(7, (void *)X509_reference_inc);
    SSLCERT_set_field(8, (void *)X509_get_signature_type);
    SSLCERT_set_field(9, (void *)X509_subject_name_cmp);
    SSLCERT_set_field(10, (void *)X509_get_issuer_name);
    SSLCERT_set_field(11, (void *)X509_get_subject_name);
    SSLCERT_set_field(12, (void *)X509_get_pubkey);
    SSLCERT_set_field(13, (void *)sslcert_get_serialNumber_helper);
    SSLCERT_set_field(14, (void *)sslcert_get_notAfter_helper);
    SSLCERT_set_field(15, (void *)sslcert_get_notBefore_helper);
    SSLCERT_set_field(16, (void *)X509_NAME_new);
    SSLCERT_set_field(17, (void *)X509_NAME_free);
    SSLCERT_set_field(18, (void *)X509_NAME_dup);
    SSLCERT_set_field(19, (void *)X509_NAME_hash);
    SSLCERT_set_field(21, (void *)X509_NAME_cmp);
    SSLCERT_set_field(22, (void *)d2i_X509_NAME);
    SSLCERT_set_field(31, (void *)i2d_X509_NAME);
    SSLCERT_set_field(23, (void *)sslcert_name_get_info_helper);
    SSLCERT_set_field(24, (void *)X509_NAME_get_entry_count);
    SSLCERT_set_field(25, (void *)X509_NAME_get_entry);
    SSLCERT_set_field(26, (void *)sslcert_name_entry_get_info_helper);
    SSLCERT_set_field(27, (void *)X509_STORE_get_by_subject);
    SSLCERT_set_field(29, (void *)sslcert_do_cb_helper);
    SSLCERT_set_field(30, (void *)sslcert_do_verify_cb_helper);
    SSLCERT_set_field(32, (void *)X509_check_private_key);
    SSLCERT_set_field(33, (void *)PEM_read_bio_X509);
    SSLCERT_set_field(34, (void *)d2i_X509_bio);
    SSLCERT_set_field(35, (void *)X509_STORE_new);
    SSLCERT_set_field(36, (void *)X509_STORE_free);
    SSLCERT_set_field(37, (void *)X509_STORE_load_locations);
    SSLCERT_set_field(38, (void *)X509_STORE_set_default_paths);
    SSLCERT_set_field(39, (void *)sslcert_store_set_verify_cb_helper);
    SSLCERT_set_field(40, (void *)X509_STORE_CTX_new);
    SSLCERT_set_field(41, (void *)X509_STORE_CTX_init);
    SSLCERT_set_field(42, (void *)X509_STORE_CTX_free);
    SSLCERT_set_field(43, (void *)X509_STORE_CTX_cleanup);
    SSLCERT_set_field(44, (void *)X509_STORE_CTX_get_ex_new_index);
    SSLCERT_set_field(45, (void *)X509_STORE_CTX_cleanup_ex_data);
    SSLCERT_set_field(46, (void *)X509_STORE_CTX_set_ex_data);
    SSLCERT_set_field(47, (void *)X509_STORE_CTX_get_ex_data);
    SSLCERT_set_field(48, (void *)X509_STORE_CTX_get_error);
    SSLCERT_set_field(49, (void *)X509_STORE_CTX_set_error);
    SSLCERT_set_field(50, (void *)X509_STORE_CTX_get_error_depth);
    SSLCERT_set_field(59, (void *)X509_STORE_CTX_get_current_cert);
    SSLCERT_set_field(51, (void *)d2i_PrivateKey);
    SSLCERT_set_field(52, (void *)SSL_EVP_PKEY_new);
    SSLCERT_set_field(53, (void *)SSL_EVP_PKEY_free);
    SSLCERT_set_field(54, (void *)sslcert_pkey_from_pubkey_binary_helper);
    SSLCERT_set_field(55, (void *)SSL_EVP_PKEY_reference_inc);
    /* NOTE: bug: SSLCERT_set_field has no case 56: get_version is never stored */
    SSLCERT_set_field(56, (void *)X509_get_version);
    return 1;
}
