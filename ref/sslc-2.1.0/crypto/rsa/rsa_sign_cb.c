/*
 * rsa_sign_cb and rsa_verify_cb, from crypto/rsa/rsa_sign.c of SSLeay 0.9.0b, with
 * R_malloc/R_free; the verify has no md5/md2 compatibility branch.
 */






#include <stdio.h>
#include "cryptlib.h"
#include "rsa.h"
#include "objects.h"
#include "x509.h"
/* R_malloc and R_free come from r_com.h, which crypto.h
   includes. */
int rsa_sign_cb(type,m,m_len,sigret,siglen,rsa)
int type;
unsigned char *m;
unsigned int m_len;
unsigned char *sigret;
unsigned int *siglen;
RSA *rsa;
	{
	X509_SIG sig;
	ASN1_TYPE parameter;
	int i,j,ret=1;
	unsigned char *p,*s;
	X509_ALGOR algor;
	ASN1_OCTET_STRING digest;

	sig.algor= &algor;
	sig.algor->algorithm=OBJ_nid2obj(type);
	if (sig.algor->algorithm == NULL)
		{
		RSAerr(126,120);
		return(0);
		}
	if (sig.algor->algorithm->length == 0)
		{
		RSAerr(126,119);
		return(0);
		}
	parameter.type=V_ASN1_NULL;
	parameter.value.ptr=NULL;
	sig.algor->parameter= &parameter;

	sig.digest= &digest;
	sig.digest->data=m;
	sig.digest->length=m_len;

	i=i2d_X509_SIG(&sig,NULL);
	j=RSA_size(rsa);
	if ((i-RSA_PKCS1_PADDING) > j)
		{
		RSAerr(126,111);
		return(0);
		}
	s=(unsigned char *)R_malloc((unsigned int)j+1);
	if (s == NULL)
		{
		RSAerr(126,ERR_R_MALLOC_FAILURE);
		return(0);
		}
	p=s;
	i2d_X509_SIG(&sig,&p);
	i=RSA_private_encrypt(i,s,sigret,rsa,RSA_PKCS1_PADDING);
	if (i <= 0)
		ret=0;
	else
		*siglen=i;

	memset(s,0,(unsigned int)j+1);
	R_free(s);
	return(ret);
	}

int rsa_verify_cb(dtype,m,m_len,sigbuf,siglen,rsa)
int dtype;
unsigned char *m;
unsigned int m_len;
unsigned char *sigbuf;
unsigned int siglen;
RSA *rsa;
	{
	int i,ret=0,sigtype;
	unsigned char *p,*s;
	X509_SIG *sig=NULL;

	if (siglen != (unsigned int)RSA_size(rsa))
		{
		RSAerr(127,117);
		return(0);
		}

	s=(unsigned char *)R_malloc((unsigned int)siglen);
	if (s == NULL)
		{
		RSAerr(127,ERR_R_MALLOC_FAILURE);
		goto err;
		}
	i=RSA_public_decrypt((int)siglen,sigbuf,s,rsa,RSA_PKCS1_PADDING);

	if (i <= 0) goto err;

	p=s;
	sig=d2i_X509_SIG(NULL,&p,(long)i);
	if (sig == NULL) goto err;
	sigtype=OBJ_obj2nid(sig->algor->algorithm);

#ifdef RSA_DEBUG
	/* put a backward compatability flag in EAY */
	fprintf(stderr,"in(%s) expect(%s)\n",OBJ_nid2ln(sigtype),
		OBJ_nid2ln(dtype));
#endif

	/* no md5/md2 compatibility branch here */
	if (sigtype != dtype)
		{
		RSAerr(127,118);
		goto err;
		}
	if (	((unsigned int)sig->digest->length != m_len) ||
		(memcmp(m,sig->digest->data,m_len) != 0))
		{
		RSAerr(127,103);
		}
	else
		ret=1;
err:
	if (sig != NULL) X509_SIG_free(sig);
	memset(s,0,(unsigned int)siglen);
	R_free(s);
	return(ret);
	}
