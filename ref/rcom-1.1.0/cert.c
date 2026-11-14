/*
 * The SSLCERT layer of SSL-C: a table of methods over the X509 functions and the private keys,
 * filled by SSLCERT_normal_setup through SSLCERT_set_field, and the trampolines that call them. A
 * trampoline with an empty slot reports ERR_put_error(26, function, 14) and returns 0 unless noted.
 */

#include <stdio.h>
#include <string.h>
#include "err.h"
#include "sslcert.h"

typedef struct SSLCERT_METHOD_TABLE {
    int flag_a;                            /* 0 */
    int flag_b;                            /* 4 */
    void *new_fn;                          /* 8   case1  X509_new */
    void *free_fn;                         /* 12  case2  X509_free */
    void *verify_fn;                       /* 16  case3  X509_verify */
    void *digest_fn;                       /* 20  case4  X509_digest */
    void *to_binary_fn;                    /* 24  case5  i2d_X509 */
    void *from_binary_fn;                  /* 28  case6  d2i_X509 */
    void *reference_inc_fn;                /* 32  case7  X509_reference_inc */
    void *get_signature_type_fn;           /* 36  case8  X509_get_signature_type */
    void *subject_name_cmp_fn;              /* 40  case9  X509_subject_name_cmp */
    void *get_issuer_name_fn;              /* 44  case10 X509_get_issuer_name */
    void *get_subject_name_fn;             /* 48  case11 X509_get_subject_name */
    void *get_pubkey_fn;                   /* 52  case12 X509_get_pubkey */
    void *get_serialNumber_info_fn;        /* 56  case13 static (X509_get_serialNumber) */
    void *get_notAfter_fn;                 /* 60  case14 static (X509_get_notAfter) */
    void *get_notBefore_fn;                /* 64  case15 static (X509_get_notBefore) */
    void *name_new_fn;                     /* 68  case16 X509_NAME_new */
    void *name_free_fn;                    /* 72  case17 X509_NAME_free */
    void *name_dup_fn;                     /* 76  case18 X509_NAME_dup */
    void *name_hash_fn;                    /* 80  case19 X509_NAME_hash */
    void *name_cmp_fn;                     /* 84  case21 X509_NAME_cmp */
    void *name_from_binary_fn;             /* 88  case22 d2i_X509_NAME */
    void *name_to_binary_fn;               /* 92  case31 i2d_X509_NAME */
    void *name_get_info_fn;                /* 96  case23 static (i2d_X509_NAME len-only) */
    void *name_get_entry_count_fn;         /* 100 case24 X509_NAME_get_entry_count */
    void *name_get_entry_fn;               /* 104 case25 X509_NAME_get_entry */
    void *name_entry_get_info_fn;          /* 108 case26 static */
    void *store_get_by_subject_fn;         /* 112 case27 X509_STORE_get_by_subject */
    void *store_new_fn;                    /* 116 case35 X509_STORE_new */
    void *store_free_fn;                   /* 120 case36 X509_STORE_free */
    void *store_load_locations_fn;         /* 124 case37 X509_STORE_load_locations */
    void *store_set_default_paths_fn;      /* 128 case38 X509_STORE_set_default_paths */
    void *store_set_verify_cb_fn;          /* 132 case39 static */
    void *store_ctx_new_fn;                /* 136 case40 X509_STORE_CTX_new */
    void *store_ctx_init_fn;               /* 140 case41 X509_STORE_CTX_init */
    void *store_ctx_free_fn;               /* 144 case42 X509_STORE_CTX_free */
    void *store_ctx_cleanup_fn;            /* 148 case43 X509_STORE_CTX_cleanup */
    void *store_ctx_get_ex_new_index_fn;   /* 152 case44 X509_STORE_CTX_get_ex_new_index */
    void *store_ctx_cleanup_ex_data_fn;    /* 156 case45 X509_STORE_CTX_cleanup_ex_data */
    void *store_ctx_set_ex_data_fn;        /* 160 case46 X509_STORE_CTX_set_ex_data */
    void *store_ctx_get_ex_data_fn;        /* 164 case47 X509_STORE_CTX_get_ex_data */
    void *store_ctx_get_error_fn;          /* 168 case48 X509_STORE_CTX_get_error */
    void *store_ctx_set_error_fn;          /* 172 case49 X509_STORE_CTX_set_error */
    void *store_ctx_get_error_depth_fn;    /* 176 case50 X509_STORE_CTX_get_error_depth */
    void *store_ctx_get_current_cert_fn;   /* 180 case59 X509_STORE_CTX_get_current_cert */
    void *reserved_28_fn;                  /* 184 case 28 (the switch has no case 28) */
    void *do_application_cb_fn;            /* 188 case29 static */
    void *do_verify_cb_fn;                 /* 192 case30 static */
    void *check_private_key_fn;            /* 196 case32 X509_check_private_key */
    void *pem_read_bio_fn;                 /* 200 case33 PEM_read_bio_X509 */
    void *from_binary_bio_fn;              /* 204 case34 d2i_X509_bio */
    void *pkey_new_fn;                     /* 208 case52 SSL_EVP_PKEY_new */
    void *pkey_free_fn;                    /* 212 case53 SSL_EVP_PKEY_free */
    void *pkey_from_binary_fn;             /* 216 case51 d2i_PrivateKey */
    void *pkey_from_pubkey_binary_fn;      /* 220 case54 static */
    void *pkey_reference_inc_fn;           /* 224 case55 SSL_EVP_PKEY_reference_inc */
    void *get_version_fn;                  /* 228 never set (NOTE: bug: normal_setup asks for case 56, which does not exist) */
} SSLCERT_METHOD_TABLE;

static SSLCERT_METHOD_TABLE sslcert_methods;

/* Clears the table (SSLCERT_normal_setup). */
void sslcert_set_clear()
{
    memset(&sslcert_methods, 0, sizeof(sslcert_methods));
    return;
}

int SSLCERT_set_flag(which, value)
    int which;
    int value;
{
    int ret = 1;
    switch (which) {
    case 0:
        sslcert_methods.flag_a = value;
        break;
    case 1:
        sslcert_methods.flag_b = value;
        break;
    default:
        ret = 0;
        break;
    }
    return ret;
}

/* Stores funcptr in the slot of case (1 to 59); the cases 20 and 56 to 58 fall into the default and
 * store nothing. */
int SSLCERT_set_field(which, fn)
    int which;
    void *fn;
{
    int ret = 1;
    switch (which) {
    case 1: sslcert_methods.new_fn = fn; break;
    case 2: sslcert_methods.free_fn = fn; break;
    case 3: sslcert_methods.verify_fn = fn; break;
    case 4: sslcert_methods.digest_fn = fn; break;
    case 5: sslcert_methods.to_binary_fn = fn; break;
    case 6: sslcert_methods.from_binary_fn = fn; break;
    case 7: sslcert_methods.reference_inc_fn = fn; break;
    case 8: sslcert_methods.get_signature_type_fn = fn; break;
    case 9: sslcert_methods.subject_name_cmp_fn = fn; break;
    case 10: sslcert_methods.get_issuer_name_fn = fn; break;
    case 11: sslcert_methods.get_subject_name_fn = fn; break;
    case 12: sslcert_methods.get_pubkey_fn = fn; break;
    case 13: sslcert_methods.get_serialNumber_info_fn = fn; break;
    case 14: sslcert_methods.get_notAfter_fn = fn; break;
    case 15: sslcert_methods.get_notBefore_fn = fn; break;
    case 16: sslcert_methods.name_new_fn = fn; break;
    case 17: sslcert_methods.name_free_fn = fn; break;
    case 18: sslcert_methods.name_dup_fn = fn; break;
    case 19: sslcert_methods.name_hash_fn = fn; break;
    case 21: sslcert_methods.name_cmp_fn = fn; break;
    case 22: sslcert_methods.name_from_binary_fn = fn; break;
    case 31: sslcert_methods.name_to_binary_fn = fn; break;
    case 23: sslcert_methods.name_get_info_fn = fn; break;
    case 24: sslcert_methods.name_get_entry_count_fn = fn; break;
    case 25: sslcert_methods.name_get_entry_fn = fn; break;
    case 26: sslcert_methods.name_entry_get_info_fn = fn; break;
    case 35: sslcert_methods.store_new_fn = fn; break;
    case 36: sslcert_methods.store_free_fn = fn; break;
    case 37: sslcert_methods.store_load_locations_fn = fn; break;
    case 38: sslcert_methods.store_set_default_paths_fn = fn; break;
    case 39: sslcert_methods.store_set_verify_cb_fn = fn; break;
    case 27: sslcert_methods.store_get_by_subject_fn = fn; break;
    case 40: sslcert_methods.store_ctx_new_fn = fn; break;
    case 41: sslcert_methods.store_ctx_init_fn = fn; break;
    case 42: sslcert_methods.store_ctx_free_fn = fn; break;
    case 43: sslcert_methods.store_ctx_cleanup_fn = fn; break;
    case 44: sslcert_methods.store_ctx_get_ex_new_index_fn = fn; break;
    case 45: sslcert_methods.store_ctx_cleanup_ex_data_fn = fn; break;
    case 46: sslcert_methods.store_ctx_set_ex_data_fn = fn; break;
    case 47: sslcert_methods.store_ctx_get_ex_data_fn = fn; break;
    case 48: sslcert_methods.store_ctx_get_error_fn = fn; break;
    case 49: sslcert_methods.store_ctx_set_error_fn = fn; break;
    case 50: sslcert_methods.store_ctx_get_error_depth_fn = fn; break;
    case 59: sslcert_methods.store_ctx_get_current_cert_fn = fn; break;
    case 28: sslcert_methods.reserved_28_fn = fn; break;
    case 33: sslcert_methods.pem_read_bio_fn = fn; break;
    case 32: sslcert_methods.check_private_key_fn = fn; break;
    case 34: sslcert_methods.from_binary_bio_fn = fn; break;
    case 29: sslcert_methods.do_application_cb_fn = fn; break;
    case 30: sslcert_methods.do_verify_cb_fn = fn; break;
    case 52: sslcert_methods.pkey_new_fn = fn; break;
    case 53: sslcert_methods.pkey_free_fn = fn; break;
    case 51: sslcert_methods.pkey_from_binary_fn = fn; break;
    case 54: sslcert_methods.pkey_from_pubkey_binary_fn = fn; break;
    case 55: sslcert_methods.pkey_reference_inc_fn = fn; break;
    default: ret = 0; break;
    }
    return ret;
}

int SSLCERT_get_flag(which)
    int which;
{
    int ret = -1;
    switch (which) {
    case 0:
        ret = sslcert_methods.flag_a;
        break;
    case 1:
        ret = sslcert_methods.flag_b;
        break;
    default:
        break;
    }
    return ret;
}


X509 *SSLCERT_new()
{
    if (sslcert_methods.new_fn == 0) {
        ERR_put_error(26, 132, 14, __FILE__, 867);
        return 0;
    }
    return (*(X509 *(*)())sslcert_methods.new_fn)();
}

/* The trampolines, in the order of the cases of SSLCERT_set_field. */

void SSLCERT_free(a)
    X509 *a;
{
    if (sslcert_methods.free_fn == 0) {
        ERR_put_error(26, 112, 14, __FILE__, 879);
        return;
    }
    (*(void (*)())sslcert_methods.free_fn)(a);
    return;
}

int SSLCERT_verify(a, r)
    X509 *a;
    EVP_PKEY *r;
{
    if (sslcert_methods.verify_fn == 0) {
        ERR_put_error(26, 150, 14, __FILE__, 893);
        return 0;
    }
    return (*(int (*)())sslcert_methods.verify_fn)(a, r);
}

int SSLCERT_digest(data, type, md, len)
    X509 *data;
    EVP_MD *type;
    unsigned char *md;
    unsigned int *len;
{
    if (sslcert_methods.digest_fn == 0) {
        ERR_put_error(26, 110, 14, __FILE__, 908);
        return 0;
    }
    return (*(int (*)())sslcert_methods.digest_fn)(data, type, md, len);
}

int SSLCERT_to_binary(a, pp)
    X509 *a;
    unsigned char **pp;
{
    if (sslcert_methods.to_binary_fn == 0) {
        ERR_put_error(26, 106, 14, __FILE__, 921);
        return 0;
    } else {
        return (*(int (*)())sslcert_methods.to_binary_fn)(a, pp);
    }
}

X509 *SSLCERT_from_binary(a, pp, length)
    X509 **a; unsigned char **pp; long length;
{
    if (sslcert_methods.from_binary_fn == 0) {
        ERR_put_error(26, 105, 14, __FILE__, 935);
        return 0;
    } else {
        return (*(X509 *(*)())sslcert_methods.from_binary_fn)(a, pp, length);
    }
}

void SSLCERT_reference_inc(x)
    X509 *x;
{
    if (sslcert_methods.reference_inc_fn == 0) {
        ERR_put_error(26, 136, 14, __FILE__, 947);
        return;
    }
    (*(void (*)())sslcert_methods.reference_inc_fn)(x);
    return;
}

int SSLCERT_get_signature_type(x)
    X509 *x;
{
    if (sslcert_methods.get_signature_type_fn == 0) {
        ERR_put_error(26, 119, 14, __FILE__, 960);
        return 0;
    }
    return (*(int (*)())sslcert_methods.get_signature_type_fn)(x);
}

int SSLCERT_subject_name_cmp(a, b)
    X509 *a, *b;
{
    if (sslcert_methods.subject_name_cmp_fn == 0) {
        ERR_put_error(26, 149, 14, __FILE__, 973);
        return 0;
    }
    return (*(int (*)())sslcert_methods.subject_name_cmp_fn)(a, b);
}

X509_NAME *SSLCERT_get_issuer_name(a)
    X509 *a;
{
    if (sslcert_methods.get_issuer_name_fn == 0) {
        ERR_put_error(26, 114, 14, __FILE__, 985);
        return 0;
    }
    return (*(void *(*)())sslcert_methods.get_issuer_name_fn)(a);
}

X509_NAME *SSLCERT_get_subject_name(a)
    X509 *a;
{
    if (sslcert_methods.get_subject_name_fn == 0) {
        ERR_put_error(26, 120, 14, __FILE__, 997);
        return 0;
    }
    return (*(void *(*)())sslcert_methods.get_subject_name_fn)(a);
}

EVP_PKEY *SSLCERT_get_pubkey(x)
    X509 *x;
{
    if (sslcert_methods.get_pubkey_fn == 0) {
        ERR_put_error(26, 117, 14, __FILE__, 1009);
        return 0;
    }
    return (*(void *(*)())sslcert_methods.get_pubkey_fn)(x);
}

/* serialNumber_info, notAfter and notBefore call a two-argument helper (cert, &info) and copy the
 * three fields of the sslcert_info to the out parameters: type, data, length. */
int SSLCERT_get_serialNumber_info(cert, type, data, len)
    X509 *cert;
    long *type;
    long *data;
    long *len;
{
    sslcert_info info;
    int ret;

    if (sslcert_methods.get_serialNumber_info_fn == 0) {
        ERR_put_error(26, 118, 14, __FILE__, 1027);
        return 0;
    }
    ret = (*(int (*)())sslcert_methods.get_serialNumber_info_fn)(cert, &info);
    if (ret) {
        *data = (long)info.data;
        *len = info.len;
        *type = info.type;
    }
    return ret;
}

int SSLCERT_get_notAfter(cert, type, data, len)
    X509 *cert;
    int *type;
    char **data;
    int *len;
{
    int ret;
    sslcert_info info;

    if (sslcert_methods.get_notAfter_fn == 0) {
        ERR_put_error(26, 115, 14, __FILE__, 1054);
        return 0;
    }
    ret = (*(int (*)())sslcert_methods.get_notAfter_fn)(cert, &info);
    if (ret == 1) {
        *type = info.type;
        *data = info.data;
        *len = info.len;
    }
    return ret;
}

int SSLCERT_get_notBefore(cert, type, data, len)
    X509 *cert;
    int *type;
    char **data;
    int *len;
{
    int ret;
    sslcert_info info;

    if (sslcert_methods.get_notBefore_fn == 0) {
        ERR_put_error(26, 116, 14, __FILE__, 1080);
        return 0;
    }
    ret = (*(int (*)())sslcert_methods.get_notBefore_fn)(cert, &info);
    if (ret == 1) {
        *type = info.type;
        *data = info.data;
        *len = info.len;
    }
    return ret;
}

X509_NAME *SSLCERT_NAME_new()
{
    if (sslcert_methods.name_new_fn == 0) {
        ERR_put_error(26, 130, 14, __FILE__, 1098);
        return 0;
    }
    return (*(void *(*)())sslcert_methods.name_new_fn)();
}

void SSLCERT_NAME_free(a)
    X509_NAME *a;
{
    if (sslcert_methods.name_free_fn == 0) {
        ERR_put_error(26, 124, 14, __FILE__, 1110);
        return;
    }
    (*(void (*)())sslcert_methods.name_free_fn)(a);
    return;
}

X509_NAME *SSLCERT_NAME_dup(xn)
    X509_NAME *xn;
{
    if (sslcert_methods.name_dup_fn == 0) {
        ERR_put_error(26, 122, 14, __FILE__, 1123);
        return 0;
    }
    return (*(void *(*)())sslcert_methods.name_dup_fn)(xn);
}

unsigned long SSLCERT_NAME_hash(x)
    X509_NAME *x;
{
    if (sslcert_methods.name_hash_fn == 0) {
        ERR_put_error(26, 129, 14, __FILE__, 1135);
        return 0;
    }
    return (*(unsigned long (*)())sslcert_methods.name_hash_fn)(x);
}

int SSLCERT_NAME_cmp(a, b)
    X509_NAME *a, *b;
{
    if (sslcert_methods.name_cmp_fn == 0) {
        ERR_put_error(26, 121, 14, __FILE__, 1148);
        return 0;
    }
    return (*(int (*)())sslcert_methods.name_cmp_fn)(a, b);
}

void *SSLCERT_NAME_from_binary(a, pp, length)
    X509_NAME **a;
    unsigned char **pp;
    long length;
{
    if (sslcert_methods.name_from_binary_fn == 0) {
        ERR_put_error(26, 125, 14, __FILE__, 1162);
        return 0;
    }
    return (*(void *(*)())sslcert_methods.name_from_binary_fn)(a, pp, length);
}

int SSLCERT_NAME_to_binary(a, pp)
    X509_NAME *a;
    unsigned char **pp;
{
    if (sslcert_methods.name_to_binary_fn == 0) {
        ERR_put_error(26, 131, 14, __FILE__, 1175);
        return 0;
    }
    return (*(int (*)())sslcert_methods.name_to_binary_fn)(a, pp);
}

/* Calls name_get_info (case 23): *b gets the DER bytes and *c their length. */
int SSLCERT_NAME_get_info(name, data, len)
    X509_NAME *name;
    long *data;
    long *len;
{
    int r;
    sslcert_info info;

    if (sslcert_methods.name_get_info_fn == 0) {
        ERR_put_error(26, 128, 14, __FILE__, 1192);
        return 0;
    }
    r = (*(int (*)())sslcert_methods.name_get_info_fn)(name, &info);
    if (r == 1) {
        *data = (long)info.data;
        *len = info.len;
    }
    return r;
}

int SSLCERT_NAME_get_entry_count(name)
    X509_NAME *name;
{
    if (sslcert_methods.name_get_entry_count_fn == 0) {
        ERR_put_error(26, 127, 14, __FILE__, 1211);
        return 0;
    }
    return (*(int (*)())sslcert_methods.name_get_entry_count_fn)(name);
}

X509_NAME_ENTRY *SSLCERT_NAME_get_entry(name, loc)
    X509_NAME *name;
    int loc;
{
    if (sslcert_methods.name_get_entry_fn == 0) {
        ERR_put_error(26, 126, 14, __FILE__, 1224);
        return 0;
    }
    return (*(void *(*)())sslcert_methods.name_get_entry_fn)(name, loc);
}

/* The entry and six out parameters, filled from the info of the OID and of the data;
 * get_oid_info and get_data_info call it with the other group set to 0. */
int SSLCERT_NAME_ENTRY_get_info(entry, oid_type, oid_data, oid_len,
                                 val_type, val_data, val_len)
    X509_NAME_ENTRY *entry;
    int *oid_type;
    unsigned char **oid_data;
    int *oid_len;
    int *val_type;
    char **val_data;
    int *val_len;
{
    int ret;
    sslcert_info oidinfo;
    sslcert_info datainfo;

    ret = 0;
    if (sslcert_methods.name_entry_get_info_fn == 0) {
        ERR_put_error(26, 123, 14, __FILE__, 1247);
        return 0;
    }
    ret = (*(int (*)())sslcert_methods.name_entry_get_info_fn)(entry, &oidinfo, &datainfo);
    if (ret == 1) {
        if (oid_type != 0) *oid_type = oidinfo.type;
        if (oid_data != 0) *oid_data = oidinfo.data;
        if (oid_len != 0) *oid_len = oidinfo.len;
        if (val_type != 0) *val_type = datainfo.type;
        if (val_data != 0) *val_data = datainfo.data;
        if (val_len != 0) *val_len = datainfo.len;
    }
    return ret;
}

int SSLCERT_NAME_ENTRY_get_oid_info(entry, oid_type, oid_data, oid_len)
    X509_NAME_ENTRY *entry;
    int *oid_type;
    unsigned char **oid_data;
    int *oid_len;
{
    return SSLCERT_NAME_ENTRY_get_info(entry, oid_type, oid_data, oid_len, 0, 0, 0);
}

int SSLCERT_NAME_ENTRY_get_data_info(entry, val_type, val_data, val_len)
    X509_NAME_ENTRY *entry;
    int *val_type;
    char **val_data;
    int *val_len;
{
    return SSLCERT_NAME_ENTRY_get_info(entry, 0, 0, 0, val_type, val_data, val_len);
}


X509_STORE *SSLCERT_STORE_new()
{
    if (sslcert_methods.store_new_fn == 0) {
        ERR_put_error(26, 148, 14, __FILE__, 1295);
        return 0;
    }
    return (*(void *(*)())sslcert_methods.store_new_fn)();
}

void SSLCERT_STORE_free(v)
    X509_STORE *v;
{
    if (sslcert_methods.store_free_fn == 0) {
        ERR_put_error(26, 146, 14, __FILE__, 1307);
        return;
    }
    (*(void (*)())sslcert_methods.store_free_fn)(v);
    return;
}

int SSLCERT_STORE_load_locations(ctx, file, dir)
    X509_STORE *ctx; char *file, *dir;
{
    if (sslcert_methods.store_load_locations_fn == 0) {
        ERR_put_error(26, 103, 14, __FILE__, 1323);
        return 0;
    } else {
        return (*(int (*)())sslcert_methods.store_load_locations_fn)(ctx, file, dir);
    }
}

int SSLCERT_STORE_set_default_paths(ctx)
    X509_STORE *ctx;
{
    if (sslcert_methods.store_set_default_paths_fn == 0) {
        ERR_put_error(26, 104, 14, __FILE__, 1335);
        return 0;
    } else {
        return (*(int (*)())sslcert_methods.store_set_default_paths_fn)(ctx);
    }
}

/* No error is reported: returns 0 when the slot is empty. */
int SSLCERT_STORE_set_verify_cb(ctx, cb)
    X509_STORE *ctx; int (*cb)();
{
    if (sslcert_methods.store_set_verify_cb_fn == 0) {
        return 0;
    } else {
        return (*(int (*)())sslcert_methods.store_set_verify_cb_fn)(ctx, cb);
    }
}

int SSLCERT_STORE_get_by_subject(vs, type, name, ret)
    X509_STORE_CTX *vs; int type; X509_NAME *name; X509_OBJECT *ret;
{
    if (sslcert_methods.store_get_by_subject_fn == 0) {
        ERR_put_error(26, 147, 14, __FILE__, 1362);
        return 0;
    }
    return (*(int (*)())sslcert_methods.store_get_by_subject_fn)(vs, type, name, ret);
}

X509_STORE_CTX *SSLCERT_STORE_CTX_new()
{
    if (sslcert_methods.store_ctx_new_fn == 0) {
        ERR_put_error(26, 153, 14, __FILE__, 1383);
        return 0;
    }
    return (*(void *(*)())sslcert_methods.store_ctx_new_fn)();
}

void SSLCERT_STORE_CTX_init(ctx, store, x509, chain)
    X509_STORE_CTX *ctx; X509_STORE *store; X509 *x509; STACK *chain;
{
    if (sslcert_methods.store_ctx_init_fn == 0) {
        ERR_put_error(26, 143, 14, __FILE__, 1398);
        return;
    }
    (*(void (*)())sslcert_methods.store_ctx_init_fn)(ctx, store, x509, chain);
    return;
}

void SSLCERT_STORE_CTX_free(ctx)
    X509_STORE_CTX *ctx;
{
    if (sslcert_methods.store_ctx_free_fn == 0) {
        ERR_put_error(26, 152, 14, __FILE__, 1411);
        return;
    }
    (*(void (*)())sslcert_methods.store_ctx_free_fn)(ctx);
    return;
}

void SSLCERT_STORE_CTX_cleanup(ctx)
    X509_STORE_CTX *ctx;
{
    if (sslcert_methods.store_ctx_cleanup_fn == 0) {
        ERR_put_error(26, 137, 14, __FILE__, 1424);
        return;
    }
    (*(void (*)())sslcert_methods.store_ctx_cleanup_fn)(ctx);
    return;
}

/* Returns -1 on failure, the only trampoline that does; four arguments. */
/* Five arguments, the fifth passed on. */
int SSLCERT_STORE_CTX_get_ex_new_index(argl, argp, new_func, dup_func, free_func)
    long argl; char *argp; int (*new_func)(), (*dup_func)(); void (*free_func)();
{
    if (sslcert_methods.store_ctx_get_ex_new_index_fn == 0) {
        ERR_put_error(26, 102, 14, __FILE__, 1441);
        return -1;
    } else {
        return (*(int (*)())sslcert_methods.store_ctx_get_ex_new_index_fn)(argl, argp, new_func, dup_func, free_func);
    }
}

int SSLCERT_STORE_CTX_cleanup_ex_data()
{
    if (sslcert_methods.store_ctx_cleanup_ex_data_fn == 0) {
        ERR_put_error(26, 138, 14, __FILE__, 1458);
        return 0;
    }
    return (*(int (*)())sslcert_methods.store_ctx_cleanup_ex_data_fn)();
}

int SSLCERT_STORE_CTX_set_ex_data(ctx, idx, data)
    X509_STORE_CTX *ctx; int idx; char *data;
{
    if (sslcert_methods.store_ctx_set_ex_data_fn == 0) {
        ERR_put_error(26, 145, 14, __FILE__, 1472);
        return 0;
    }
    return (*(int (*)())sslcert_methods.store_ctx_set_ex_data_fn)(ctx, idx, data);
}

char *SSLCERT_STORE_CTX_get_ex_data(ctx, idx)
    X509_STORE_CTX *ctx; int idx;
{
    if (sslcert_methods.store_ctx_get_ex_data_fn == 0) {
        ERR_put_error(26, 142, 14, __FILE__, 1485);
        return 0;
    }
    return (*(char *(*)())sslcert_methods.store_ctx_get_ex_data_fn)(ctx, idx);
}

int SSLCERT_STORE_CTX_get_error(ctx)
    X509_STORE_CTX *ctx;
{
    if (sslcert_methods.store_ctx_get_error_fn == 0) {
        ERR_put_error(26, 140, 14, __FILE__, 1497);
        return 0;
    }
    return (*(int (*)())sslcert_methods.store_ctx_get_error_fn)(ctx);
}

void SSLCERT_STORE_CTX_set_error(ctx, s)
    X509_STORE_CTX *ctx; int s;
{
    if (sslcert_methods.store_ctx_set_error_fn == 0) {
        ERR_put_error(26, 144, 14, __FILE__, 1510);
        return;
    }
    (*(void (*)())sslcert_methods.store_ctx_set_error_fn)(ctx, s);
    return;
}

int SSLCERT_STORE_CTX_get_error_depth(ctx)
    X509_STORE_CTX *ctx;
{
    if (sslcert_methods.store_ctx_get_error_depth_fn == 0) {
        ERR_put_error(26, 141, 14, __FILE__, 1523);
        return 0;
    }
    return (*(int (*)())sslcert_methods.store_ctx_get_error_depth_fn)(ctx);
}

X509 *SSLCERT_STORE_CTX_get_current_cert(ctx)
    X509_STORE_CTX *ctx;
{
    if (sslcert_methods.store_ctx_get_current_cert_fn == 0) {
        ERR_put_error(26, 139, 14, __FILE__, 1535);
        return 0;
    }
    return (*(X509 *(*)())sslcert_methods.store_ctx_get_current_cert_fn)(ctx);
}

/* do_application_cb and do_verify_cb take five and six arguments. */
int SSLCERT_do_application_cb(s, sk, cb, arg, result)
    SSL *s; STACK *sk; int (*cb)(); char *arg; long *result;
{
    if (sslcert_methods.do_application_cb_fn == 0) {
        ERR_put_error(26, 111, 14, __FILE__, 1560);
        return 0;
    }
    return (*(int (*)())sslcert_methods.do_application_cb_fn)(s, sk, cb, arg, result);
}

int SSLCERT_do_verify_cb(s, sk, store, verify_cb, verify_cb_arg, result)
    SSL *s; STACK *sk; X509_STORE *store; int verify_cb, verify_cb_arg; long *result;
{
    if (sslcert_methods.do_verify_cb_fn == 0) {
        ERR_put_error(26, 100, 14, __FILE__, 1587);
        return 0;
    } else {
        return (*(int (*)())sslcert_methods.do_verify_cb_fn)(s, sk, store, verify_cb, verify_cb_arg, result);
    }
}

int SSLCERT_check_private_key(x509, pkey)
    X509 *x509;
    EVP_PKEY *pkey;
{
    if (sslcert_methods.check_private_key_fn == 0) {
        ERR_put_error(26, 109, 14, __FILE__, 1601);
        return 0;
    }
    return (*(int (*)())sslcert_methods.check_private_key_fn)(x509, pkey);
}

X509 *SSLCERT_PEM_read_bio_SSLCERT(bp, x, cb)
    BIO *bp; X509 **x; int (*cb)();
{
    if (sslcert_methods.pem_read_bio_fn == 0) {
        ERR_put_error(26, 133, 14, __FILE__, 1616);
        return 0;
    }
    return (*(X509 *(*)())sslcert_methods.pem_read_bio_fn)(bp, x, cb);
}

X509 *SSLCERT_from_binary_bio(bp, x509)
    BIO *bp; X509 *x509;
{
    if (sslcert_methods.from_binary_bio_fn == 0) {
        ERR_put_error(26, 113, 14, __FILE__, 1630);
        return 0;
    }
    return (*(X509 *(*)())sslcert_methods.from_binary_bio_fn)(bp, x509);
}

EVP_PKEY *SSLCERT_PKEY_new()
{
    if (sslcert_methods.pkey_new_fn == 0) {
        ERR_put_error(26, 135, 14, __FILE__, 1641);
        return 0;
    }
    return (*(void *(*)())sslcert_methods.pkey_new_fn)();
}

void SSLCERT_PKEY_free(pkey)
    EVP_PKEY *pkey;
{
    if (sslcert_methods.pkey_free_fn == 0) {
        ERR_put_error(26, 134, 14, __FILE__, 1653);
        return;
    }
    (*(void (*)())sslcert_methods.pkey_free_fn)(pkey);
    return;
}

EVP_PKEY *SSLCERT_PKEY_from_binary(type, a, pp, length)
    int type; EVP_PKEY **a; unsigned char **pp; long length;
{
    if (sslcert_methods.pkey_from_binary_fn == 0) {
        ERR_put_error(26, 107, 14, __FILE__, 1669);
        return 0;
    } else {
        return (*(void *(*)())sslcert_methods.pkey_from_binary_fn)(type, a, pp, length);
    }
}

EVP_PKEY *SSLCERT_PKEY_from_PUBKEY_binary(type, a, pp, length)
    int type;
    EVP_PKEY **a;
    unsigned char **pp;
    long length;
{
    if (sslcert_methods.pkey_from_pubkey_binary_fn == 0) {
        ERR_put_error(26, 108, 14, __FILE__, 1684);
        return 0;
    } else {
        return (*(void *(*)())sslcert_methods.pkey_from_pubkey_binary_fn)(type, a, pp, length);
    }
}

void SSLCERT_PKEY_reference_inc(pkey)
    EVP_PKEY *pkey;
{
    if (sslcert_methods.pkey_reference_inc_fn == 0) {
        ERR_put_error(26, 136, 14, __FILE__, 1696);
        return;
    }
    (*(void (*)())sslcert_methods.pkey_reference_inc_fn)(pkey);
    return;
}

/* The slot of case 56 is never filled (see SSLCERT_normal_setup): this always fails. */
long SSLCERT_get_version(x)
    X509 *x;
{
    if (sslcert_methods.get_version_fn == 0) {
        ERR_put_error(26, 151, 14, __FILE__, 1709);
        return 0;
    }
    return (*(long (*)())sslcert_methods.get_version_fn)(x);
}

