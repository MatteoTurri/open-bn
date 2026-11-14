/*
 * The SSLCERT_* layer of RCOM, a table of methods over the X509 and SSL_EVP_PKEY
 * functions: the record its info getters fill, and the layer functions, with the
 * SSL-C types (the SSL layer calls them with the same arguments as the X509_* ones).
 */
#ifndef RCOM_SSLCERT_H
#define RCOM_SSLCERT_H

#include "ssl.h"

/* Filled by the get_serialNumber_info, get_notAfter, get_notBefore and NAME_*_get_info methods. */
typedef struct {
    /* 0x00 */ long type;
    /* 0x04 */ long len;
    /* 0x08 */ void *data;
    /* 0x0C */ long pad_0C[5];  /* unused, but part of the layout */
} sslcert_info; /* size = 0x20 */

/* cert_err.c */
void ERR_load_SSLCERT_strings(void);

/* cert.c */
int SSLCERT_NAME_ENTRY_get_data_info(X509_NAME_ENTRY *entry, int *type, char **data, int *len);
int SSLCERT_NAME_ENTRY_get_info(X509_NAME_ENTRY *entry, int *oid_type, unsigned char **oid_data,
                                int *oid_len, int *val_type, char **val_data, int *val_len);
int SSLCERT_NAME_ENTRY_get_oid_info(X509_NAME_ENTRY *entry, int *type, unsigned char **data,
                                    int *len);
int SSLCERT_NAME_cmp(X509_NAME *a, X509_NAME *b);
X509_NAME *SSLCERT_NAME_dup(X509_NAME *xn);
void SSLCERT_NAME_free(X509_NAME *a);
void *SSLCERT_NAME_from_binary(X509_NAME **a, unsigned char **pp, long length);
X509_NAME_ENTRY *SSLCERT_NAME_get_entry(X509_NAME *name, int loc);
int SSLCERT_NAME_get_entry_count(X509_NAME *name);
int SSLCERT_NAME_get_info(X509_NAME *name, long *data, long *len);
unsigned long SSLCERT_NAME_hash(X509_NAME *x);
X509_NAME *SSLCERT_NAME_new(void);
int SSLCERT_NAME_to_binary(X509_NAME *a, unsigned char **pp);
X509 *SSLCERT_PEM_read_bio_SSLCERT(BIO *bp, X509 **x, int (*cb)());
void SSLCERT_PKEY_free(EVP_PKEY *pkey);
EVP_PKEY *SSLCERT_PKEY_from_PUBKEY_binary(int type, EVP_PKEY **a, unsigned char **pp, long length);
EVP_PKEY *SSLCERT_PKEY_from_binary(int type, EVP_PKEY **a, unsigned char **pp, long length);
EVP_PKEY *SSLCERT_PKEY_new(void);
void SSLCERT_PKEY_reference_inc(EVP_PKEY *pkey);
void SSLCERT_STORE_CTX_cleanup(X509_STORE_CTX *ctx);
int SSLCERT_STORE_CTX_cleanup_ex_data(void);
void SSLCERT_STORE_CTX_free(X509_STORE_CTX *ctx);
X509 *SSLCERT_STORE_CTX_get_current_cert(X509_STORE_CTX *ctx);
int SSLCERT_STORE_CTX_get_error(X509_STORE_CTX *ctx);
int SSLCERT_STORE_CTX_get_error_depth(X509_STORE_CTX *ctx);
char *SSLCERT_STORE_CTX_get_ex_data(X509_STORE_CTX *ctx, int idx);
int SSLCERT_STORE_CTX_get_ex_new_index(long argl, char *argp, int (*new_func)(),
                                       int (*dup_func)(), void (*free_func)());
void SSLCERT_STORE_CTX_init(X509_STORE_CTX *ctx, X509_STORE *store, X509 *x509, STACK *chain);
X509_STORE_CTX *SSLCERT_STORE_CTX_new(void);
void SSLCERT_STORE_CTX_set_error(X509_STORE_CTX *ctx, int s);
int SSLCERT_STORE_CTX_set_ex_data(X509_STORE_CTX *ctx, int idx, char *data);
void SSLCERT_STORE_free(X509_STORE *v);
int SSLCERT_STORE_get_by_subject(X509_STORE_CTX *vs, int type, X509_NAME *name, X509_OBJECT *ret);
int SSLCERT_STORE_load_locations(X509_STORE *ctx, char *file, char *dir);
X509_STORE *SSLCERT_STORE_new(void);
int SSLCERT_STORE_set_default_paths(X509_STORE *ctx);
int SSLCERT_STORE_set_verify_cb(X509_STORE *ctx, int (*cb)());
int SSLCERT_check_private_key(X509 *x509, EVP_PKEY *pkey);
int SSLCERT_digest(X509 *data, EVP_MD *type, unsigned char *md, unsigned int *len);
int SSLCERT_do_application_cb(SSL *s, STACK *sk, int (*cb)(), char *arg, long *result);
int SSLCERT_do_verify_cb(SSL *s, STACK *sk, X509_STORE *store, int verify_cb, int verify_cb_arg, long *result);
void SSLCERT_free(X509 *a);
X509 *SSLCERT_from_binary(X509 **a, unsigned char **pp, long length);
X509 *SSLCERT_from_binary_bio(BIO *bp, X509 *x509);
int SSLCERT_get_flag(int which);
X509_NAME *SSLCERT_get_issuer_name(X509 *a);
int SSLCERT_get_notAfter(X509 *cert, int *type, char **data, int *len);
int SSLCERT_get_notBefore(X509 *cert, int *type, char **data, int *len);
EVP_PKEY *SSLCERT_get_pubkey(X509 *x);
int SSLCERT_get_serialNumber_info(X509 *cert, long *type, long *data, long *len);
int SSLCERT_get_signature_type(X509 *x);
X509_NAME *SSLCERT_get_subject_name(X509 *a);
long SSLCERT_get_version(X509 *x);
X509 *SSLCERT_new(void);
void SSLCERT_reference_inc(X509 *x);
int SSLCERT_set_field(int which, void *fn);
int SSLCERT_set_flag(int which, int value);
int SSLCERT_subject_name_cmp(X509 *a, X509 *b);
int SSLCERT_to_binary(X509 *a, unsigned char **pp);
int SSLCERT_verify(X509 *a, EVP_PKEY *r);
void sslcert_set_clear(void);

/* cert_setup.c */
int SSLCERT_normal_setup(void);

/* cert_oid.c */
const char *SSLCERT_OID_to_string(const unsigned char *oid, int len, int shortform);

#endif
