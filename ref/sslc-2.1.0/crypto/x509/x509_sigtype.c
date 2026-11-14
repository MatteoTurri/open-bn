/*
 * X509_get_signature_type of the SSL-C fork.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "bio.h"
#include "asn1.h"
#include "evp.h"
#include "x509.h"
#include "objects.h"


/* X509_get_signature_type(x) = EVP_PKEY_type(OBJ_obj2nid(x->sig_alg->algorithm)) */
int X509_get_signature_type(x)
X509 *x;
	{
	return(EVP_PKEY_type(OBJ_obj2nid(x->sig_alg->algorithm)));
	}
