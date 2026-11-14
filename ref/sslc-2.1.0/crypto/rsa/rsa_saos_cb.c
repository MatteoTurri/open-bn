/*
 * The RSA_METHOD callbacks of crypto/rsa/rsa_saos.c of SSLeay 0.9.0b, with R_malloc/R_free.
 * The fork's codes:
 */


#define RCOM_F_SIGN_SAOS	128
#define RCOM_F_VERIFY_SAOS	129
#define RCOM_R_DIGEST_TOO_BIG	111
#define RCOM_R_WRONG_SIG_LENGTH	117
#define RCOM_R_BAD_SIGNATURE	103
#include <stdio.h>
#include "cryptlib.h"
#include "asn1.h"
#include "rsa.h"
#include "err.h"
/* R_malloc and R_free come from r_com.h, which crypto.h
   includes. */

int rsa_sign_ASN1_OCTET_STRING_cb(type,m,m_len,sigret,siglen,rsa)
int type;
unsigned char *m;
unsigned int m_len;
unsigned char *sigret;
unsigned int *siglen;
RSA *rsa;
	{
	int ret=0;
	ASN1_OCTET_STRING sig;
	int i,j;
	unsigned char *p,*s;
	sig.type=V_ASN1_OCTET_STRING;
	sig.length=m_len;
	sig.data=m;

	i=i2d_ASN1_OCTET_STRING(&sig,NULL);
	j=RSA_size(rsa);
	if ((i-RSA_PKCS1_PADDING) > j)
		{
		RSAerr(RCOM_F_SIGN_SAOS,RCOM_R_DIGEST_TOO_BIG);
		return(0);
		}
	s=(unsigned char *)R_malloc((unsigned int)j+1);
	if (s == NULL)
		{
		RSAerr(RCOM_F_SIGN_SAOS,ERR_R_MALLOC_FAILURE);
		return(0);
		}
	p=s;
	i2d_ASN1_OCTET_STRING(&sig,&p);
	i=RSA_private_encrypt(i,s,sigret,rsa,RSA_PKCS1_PADDING);
	if (i > 0)
		{
		ret=1;
		*siglen=i;
		}

	memset(s,0,(unsigned int)j+1);
	R_free(s);
	return(ret);
	}



int rsa_verify_ASN1_OCTET_STRING_cb(dtype,m,m_len,sigbuf,siglen,rsa)
int dtype;
unsigned char *m;
unsigned int m_len;
unsigned char *sigbuf;
unsigned int siglen;
RSA *rsa;
	{
	int ret=0,i;
	unsigned char *p,*s;
	ASN1_OCTET_STRING *sig=NULL;

	if (siglen != (unsigned int)RSA_size(rsa))
		{
		RSAerr(RCOM_F_VERIFY_SAOS,RCOM_R_WRONG_SIG_LENGTH);
		return(0);
		}

	s=(unsigned char *)R_malloc((unsigned int)siglen);
	if (s == NULL)
		{
		RSAerr(RCOM_F_VERIFY_SAOS,ERR_R_MALLOC_FAILURE);
		goto err;
		}
	i=RSA_public_decrypt((int)siglen,sigbuf,s,rsa,RSA_PKCS1_PADDING);

	if (i <= 0) goto err;

	p=s;
	sig=d2i_ASN1_OCTET_STRING(NULL,&p,(long)i);
	if (sig == NULL) goto err;

	if (	((unsigned int)sig->length != m_len) ||
		(memcmp(m,sig->data,m_len) != 0))
		{
		RSAerr(RCOM_F_VERIFY_SAOS,RCOM_R_BAD_SIGNATURE);
		}
	else
		ret=1;
err:
	if (sig != NULL) ASN1_OCTET_STRING_free(sig);
	memset(s,0,(unsigned int)siglen);
	R_free(s);
	return(ret);
	}
