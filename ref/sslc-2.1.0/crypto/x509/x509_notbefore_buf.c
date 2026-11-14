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

char *X509_get_notBefore_buf(x,buf,len)
X509 *x;
char *buf;
int len;
	{
	BIO *bio;

	buf[0]='\0';
	bio=BIO_new(BIO_s_mem());
	if (bio != NULL)
		{
		if (ASN1_UTCTIME_print(bio,X509_get_notBefore(x)))
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

char *X509_get_notAfter_str(x)
X509 *x;
	{
	return((char *)X509_get_notAfter(x)->data);
	}
