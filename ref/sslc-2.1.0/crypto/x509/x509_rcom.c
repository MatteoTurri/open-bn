/*
 * X509_get_notAfter_buf, X509_get_version and X509_get_notBefore of the SSL-C fork.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "bio.h"
#include "asn1.h"
#include "evp.h"
#include "x509.h"
#include "objects.h"


char *X509_get_notAfter_buf(x,buf,len)
X509 *x;
char *buf;
int len;
	{
	BIO *bio;

	buf[0]='\0';
	bio=BIO_new(BIO_s_mem());
	if (bio != NULL)
		{
		if (ASN1_UTCTIME_print(bio,X509_get_notAfter(x)))
			{
			BIO_gets(bio,buf,len);
			BIO_free(bio);
			return(buf);
			}
		else
			{
			BIO_free(bio);
			}
		}
	return(NULL);
	}

/* X509_get_version(x) = ASN1_INTEGER_get(x->cert_info->version) */
long X509_get_version(x)
X509 *x;
	{
	return(ASN1_INTEGER_get(x->cert_info->version));
	}


ASN1_UTCTIME *X509_get_notBefore(x)
X509 *x;
	{
	return(x->cert_info->validity->notBefore);
	}
