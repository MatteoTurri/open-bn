/*
 * The notBefore/notAfter string and buffer accessors of the SSL-C fork.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "bio.h"
#include "asn1.h"
#include "evp.h"
#include "x509.h"
#include "objects.h"

/* SSL-C: returns the data of the ASN1_UTCTIME directly. */
char *X509_get_notBefore_str(x)
X509 *x;
	{
	return((char *)X509_get_notBefore(x)->data);
	}
