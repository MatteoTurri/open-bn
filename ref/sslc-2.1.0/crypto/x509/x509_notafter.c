/*
 * X509_get_notAfter of the SSL-C fork.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "bio.h"
#include "asn1.h"
#include "evp.h"
#include "x509.h"
#include "objects.h"

ASN1_UTCTIME *X509_get_notAfter(x)
X509 *x;
	{
	return(x->cert_info->validity->notAfter);
	}
