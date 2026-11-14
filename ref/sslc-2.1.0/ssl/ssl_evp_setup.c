/*
 * SSL_library_evp_setup of the SSL-C fork.
 */
#include <stdio.h>
#include "ssl_locl.h"

int SSL_library_evp_setup(void)
{
    EVP_OBJ_add_cipher(EVP_des_cbc());
    EVP_OBJ_add_cipher(EVP_des_ede3_cbc());
    EVP_OBJ_add_cipher(EVP_rc4());
    EVP_OBJ_add_cipher(EVP_rc2_cbc());

    EVP_OBJ_add_digest(EVP_md2());

    EVP_OBJ_add_digest(EVP_md5());
    EVP_OBJ_add_digest_alias("MD5","ssl2-md5");
    EVP_OBJ_add_digest_alias("MD5","ssl3-md5");

    EVP_OBJ_add_digest(EVP_sha1());
    EVP_OBJ_add_digest_alias("SHA1","ssl3-sha1");

    EVP_OBJ_add_digest(EVP_dss1());

    SSL_enable_DSA();
    SSL_feature_set(2);
    SSL_enable_DH();
    SSL_feature_set(1);
    SSL_enable_RSA();

    return(1);
}
