/*
 * X509_get_pubkey_bits of the SSL-C fork.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "bio.h"
#include "asn1.h"
#include "evp.h"
#include "x509.h"
#include "objects.h"

int X509_get_pubkey_bits(x)
X509 *x;
	{
	return(EVP_PKEY_bits(X509_get_pubkey(x)));
	}
