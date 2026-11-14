/*
 * X509_get_serialNumber_buf, X509_NAME_ENTRY_get_data_ptr and X509_NAME_ENTRY_get_oid_buf of
 * the SSL-C fork.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "bio.h"
#include "asn1.h"
#include "evp.h"
#include "x509.h"
#include "objects.h"


char *X509_get_serialNumber_buf(x,buf,len)
X509 *x;
char *buf;
int len;
	{
	BIO *bio;

	buf[0]='\0';
	bio=BIO_new(BIO_s_mem());
	if (bio != NULL)
		{
		i2a_ASN1_INTEGER(bio,X509_get_serialNumber(x));
		BIO_gets(bio,buf,len);
		BIO_free(bio);
		return(buf);
		}
	return(NULL);
	}

char *X509_NAME_ENTRY_get_data_ptr(ne)
X509_NAME_ENTRY *ne;
	{
	return((char *)ne->value->data);
	}

char *X509_NAME_ENTRY_get_oid_buf(ne,buf,len)
X509_NAME_ENTRY *ne;
char *buf;
int len;
	{
	buf[0]='\0';
	if (i2t_ASN1_OBJECT_ex(buf,len,ne->object) > 0)
		return(buf);
	else
		return(NULL);
	}
