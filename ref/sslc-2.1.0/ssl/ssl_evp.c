/*
 * SSL_EVP_PKEY_* and SSL_X509_certificate_type*: SSL-C wrappers of the EVP_PKEY_* functions.
 */
#include <stdio.h>
#include "ssl_locl.h"


#define SSL_EVP_GATED_TYPE 116

int SSL_EVP_PKEY_bits(EVP_PKEY *pkey)
{
    if (pkey->type == SSL_EVP_GATED_TYPE) {
        if (SSL_feature_test(2) == 0)
            goto zero;
    }
    return EVP_PKEY_bits(pkey);
zero:
    return 0;
}

int SSL_EVP_PKEY_size(EVP_PKEY *pkey)
{
    if (pkey == 0)
        return 0;
    if (pkey->type == SSL_EVP_GATED_TYPE) {
        if (SSL_feature_test(2) == 0)
            goto zero;
    }
    return EVP_PKEY_size(pkey);
zero:
    return 0;
}

int SSL_EVP_PKEY_save_parameters(EVP_PKEY *pkey, int mode)
{
    if (SSL_feature_test(2) != 0)
        return EVP_PKEY_save_parameters(pkey, mode);
    return 0;
}

int SSL_EVP_PKEY_copy_parameters(EVP_PKEY *to, EVP_PKEY *from)
{
    if (SSL_feature_test(2) != 0)
        return EVP_PKEY_copy_parameters(to, from);
    return 0;
}

int SSL_EVP_PKEY_missing_parameters(EVP_PKEY *pkey)
{
    if (SSL_feature_test(2) != 0)
        return EVP_PKEY_missing_parameters(pkey);
    return 0;
}

int SSL_EVP_PKEY_cmp_parameters(EVP_PKEY *a, EVP_PKEY *b)
{
    if (SSL_feature_test(2) != 0)
        return EVP_PKEY_cmp_parameters(a, b);
    return -1;
}

EVP_PKEY *SSL_EVP_PKEY_new(void)
{
    return EVP_PKEY_new();
}

void SSL_EVP_PKEY_reference_inc(EVP_PKEY *pkey)
{
    EVP_PKEY_reference_inc(pkey);
}

int SSL_EVP_PKEY_assign(EVP_PKEY *pkey, int type, char *key)
{
    return EVP_PKEY_assign(pkey, type, key);
}

int SSL_EVP_PKEY_key_type(int type)
{
    return EVP_PKEY_key_type((EVP_PKEY *)type);
}

void SSL_EVP_PKEY_free(EVP_PKEY *pkey)
{
    EVP_PKEY_free(pkey);
    return;
}


int SSL_X509_certificate_type_ex(sig_type,pkey)
int sig_type;
EVP_PKEY *pkey;
	{
	int ret;

	ret=0;
	if (pkey == NULL) return(0);

	switch (SSL_EVP_PKEY_key_type((int)pkey))
		{
	case 6:
		ret=0x11;
		ret|=0x20;
		break;
	case 116:
		ret=0x12;
		break;
	case 28:
		ret=0x44;
		break;
	default:
		break;
		}

	switch (sig_type)
		{
	case 6:
		ret|=0x100;
		break;
	case 512:
		ret|=0x200;
		break;
	default:
		break;
		}

	if (SSL_EVP_PKEY_size(pkey) < 513)
		ret|=0x1000;

	return(ret);
	}

int SSL_X509_certificate_type(x,pkey)
X509 *x;
EVP_PKEY *pkey;
	{
	EVP_PKEY *pk;

	if (x == NULL)
		{
		return(0);
		}
	if (pkey == NULL)
		pk=SSLCERT_get_pubkey(x);
	else
		pk=pkey;
	if (pk == NULL) return(0);
	return(SSL_X509_certificate_type_ex(SSLCERT_get_signature_type(x),pk));
	}
