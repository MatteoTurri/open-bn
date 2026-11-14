/* ssl/s3_lib.c */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 * 
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from 
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

#include <stdio.h>
#include "objects.h"
#include "ssl_locl.h"

char *ssl3_version_str="SSLv3 part of SSL-C 2.1.0 28-Nov-2001";

#define SSL3_NUM_CIPHERS	(sizeof(ssl3_ciphers)/sizeof(SSL_CIPHER))

#ifndef NOPROTO
static long ssl3_default_timeout(void );
static void ssl3_cleanup_tmp(SSL *s);
static int ssl3_set_tmp_dh(DH **pp, DH *dh);
static int ssl3_set_tmp_rsa(RSA **pp, RSA *rsa);
#else
static long ssl3_default_timeout();
static void ssl3_cleanup_tmp();
static int ssl3_set_tmp_dh();
static int ssl3_set_tmp_rsa();
#endif

static SSL_CIPHER ssl3_ciphers[]={
/* The RSA ciphers */
/* Cipher 01 */
	{
	1,
	SSL3_TXT_RSA_NULL_MD5,
	SSL3_CK_RSA_NULL_MD5,
	SSL_kRSA|SSL_aRSA|SSL_eNULL |SSL_MD5|SSL_NOT_EXP|SSL_SSLV3,
	0,
	SSL_ALL_CIPHERS,
	},
/* Cipher 02 */
	{
	1,
	SSL3_TXT_RSA_NULL_SHA,
	SSL3_CK_RSA_NULL_SHA,
	SSL_kRSA|SSL_aRSA|SSL_eNULL |SSL_SHA1|SSL_NOT_EXP|SSL_SSLV3,
	0,
	SSL_ALL_CIPHERS,
	},

/* anon DH */
/* Cipher 17 */
	{
	1,
	SSL3_TXT_ADH_RC4_40_MD5,
	SSL3_CK_ADH_RC4_40_MD5,
	SSL_kEDH |SSL_aNULL|SSL_RC4  |SSL_MD5 |SSL_EXP|SSL_SSLV3,
	0x205,	/* SSL-C: (export RSA 512 << 8) | 5 byte */
	SSL_ALL_CIPHERS,
	},
/* Cipher 18 */
	{
	1,
	SSL3_TXT_ADH_RC4_128_MD5,
	SSL3_CK_ADH_RC4_128_MD5,
	SSL_kEDH |SSL_aNULL|SSL_RC4  |SSL_MD5|SSL_NOT_EXP|SSL_SSLV3,
	0,
	SSL_ALL_CIPHERS,
	},
/* Cipher 19 */
	{
	1,
	SSL3_TXT_ADH_DES_40_CBC_SHA,
	SSL3_CK_ADH_DES_40_CBC_SHA,
	SSL_kEDH |SSL_aNULL|SSL_DES|SSL_SHA1|SSL_EXP|SSL_SSLV3,
	0x205,	/* SSL-C: (export RSA 512 << 8) | 5 byte */
	SSL_ALL_CIPHERS,
	},
/* Cipher 1A */
	{
	1,
	SSL3_TXT_ADH_DES_64_CBC_SHA,
	SSL3_CK_ADH_DES_64_CBC_SHA,
	SSL_kEDH |SSL_aNULL|SSL_DES  |SSL_SHA1|SSL_NOT_EXP|SSL_SSLV3,
	0,
	SSL_ALL_CIPHERS,
	},
/* Cipher 1B */
	{
	1,
	SSL3_TXT_ADH_DES_192_CBC_SHA,
	SSL3_CK_ADH_DES_192_CBC_SHA,
	SSL_kEDH |SSL_aNULL|SSL_3DES |SSL_SHA1|SSL_NOT_EXP|SSL_SSLV3,
	0,
	SSL_ALL_CIPHERS,
	},

/* RSA again */
/* Cipher 03 */
	{
	1,
	SSL3_TXT_RSA_RC4_40_MD5,
	SSL3_CK_RSA_RC4_40_MD5,
	SSL_kRSA|SSL_aRSA|SSL_RC4  |SSL_MD5 |SSL_EXP|SSL_SSLV3,
	0x205,	/* SSL-C: (export RSA 512 << 8) | 5 byte */
	SSL_ALL_CIPHERS,
	},
/* Cipher 04 */
	{
	1,
	SSL3_TXT_RSA_RC4_128_MD5,
	SSL3_CK_RSA_RC4_128_MD5,
	SSL_kRSA|SSL_aRSA|SSL_RC4  |SSL_MD5|SSL_NOT_EXP|SSL_SSLV3|SSL_MEDIUM,
	0,
	SSL_ALL_CIPHERS,
	},
/* Cipher 05 */
	{
	1,
	SSL3_TXT_RSA_RC4_128_SHA,
	SSL3_CK_RSA_RC4_128_SHA,
	SSL_kRSA|SSL_aRSA|SSL_RC4  |SSL_SHA1|SSL_NOT_EXP|SSL_SSLV3|SSL_MEDIUM,
	0,
	SSL_ALL_CIPHERS,
	},
/* Cipher 06 */
	{
	1,
	SSL3_TXT_RSA_RC2_40_MD5,
	SSL3_CK_RSA_RC2_40_MD5,
	SSL_kRSA|SSL_aRSA|SSL_RC2  |SSL_MD5 |SSL_EXP|SSL_SSLV3,
	0x205,	/* SSL-C: (export RSA 512 << 8) | 5 byte */
	SSL_ALL_CIPHERS,
	},
/* Cipher 07 */
	{
	1,
	SSL3_TXT_RSA_IDEA_128_SHA,
	SSL3_CK_RSA_IDEA_128_SHA,
	SSL_kRSA|SSL_aRSA|SSL_IDEA |SSL_SHA1|SSL_NOT_EXP|SSL_SSLV3|SSL_MEDIUM,
	0,
	SSL_ALL_CIPHERS,
	},
/* Cipher 08 */
	{
	1,
	SSL3_TXT_RSA_DES_40_CBC_SHA,
	SSL3_CK_RSA_DES_40_CBC_SHA,
	SSL_kRSA|SSL_aRSA|SSL_DES|SSL_SHA1|SSL_EXP|SSL_SSLV3,
	0x205,	/* SSL-C: (export RSA 512 << 8) | 5 byte */
	SSL_ALL_CIPHERS,
	},
/* Cipher 09 */
	{
	1,
	SSL3_TXT_RSA_DES_64_CBC_SHA,
	SSL3_CK_RSA_DES_64_CBC_SHA,
	SSL_kRSA|SSL_aRSA|SSL_DES  |SSL_SHA1|SSL_NOT_EXP|SSL_SSLV3|SSL_LOW,
	0,
	SSL_ALL_CIPHERS,
	},
/* Cipher 0A */
	{
	1,
	SSL3_TXT_RSA_DES_192_CBC3_SHA,
	SSL3_CK_RSA_DES_192_CBC3_SHA,
	SSL_kRSA|SSL_aRSA|SSL_3DES |SSL_SHA1|SSL_NOT_EXP|SSL_SSLV3|SSL_HIGH,
	0,
	SSL_ALL_CIPHERS,
	},

/*  The DH ciphers */
/* Cipher 0B */
	{
	0,
	SSL3_TXT_DH_DSS_DES_40_CBC_SHA,
	SSL3_CK_DH_DSS_DES_40_CBC_SHA,
	SSL_kDHd |SSL_aDH|SSL_DES|SSL_SHA1|SSL_EXP|SSL_SSLV3,
	0x205,	/* SSL-C: (export RSA 512 << 8) | 5 byte */
	SSL_ALL_CIPHERS,
	},
/* Cipher 0C */
	{
	0,
	SSL3_TXT_DH_DSS_DES_64_CBC_SHA,
	SSL3_CK_DH_DSS_DES_64_CBC_SHA,
	SSL_kDHd |SSL_aDH|SSL_DES  |SSL_SHA1|SSL_NOT_EXP|SSL_SSLV3|SSL_LOW,
	0,
	SSL_ALL_CIPHERS,
	},
/* Cipher 0D */
	{
	0,
	SSL3_TXT_DH_DSS_DES_192_CBC3_SHA,
	SSL3_CK_DH_DSS_DES_192_CBC3_SHA,
	SSL_kDHd |SSL_aDH|SSL_3DES |SSL_SHA1|SSL_NOT_EXP|SSL_SSLV3|SSL_HIGH,
	0,
	SSL_ALL_CIPHERS,
	},
/* Cipher 0E */
	{
	0,
	SSL3_TXT_DH_RSA_DES_40_CBC_SHA,
	SSL3_CK_DH_RSA_DES_40_CBC_SHA,
	SSL_kDHr |SSL_aDH|SSL_DES|SSL_SHA1|SSL_EXP|SSL_SSLV3,
	0x205,	/* SSL-C: (export RSA 512 << 8) | 5 byte */
	SSL_ALL_CIPHERS,
	},
/* Cipher 0F */
	{
	0,
	SSL3_TXT_DH_RSA_DES_64_CBC_SHA,
	SSL3_CK_DH_RSA_DES_64_CBC_SHA,
	SSL_kDHr |SSL_aDH|SSL_DES  |SSL_SHA1|SSL_NOT_EXP|SSL_SSLV3|SSL_LOW,
	0,
	SSL_ALL_CIPHERS,
	},
/* Cipher 10 */
	{
	0,
	SSL3_TXT_DH_RSA_DES_192_CBC3_SHA,
	SSL3_CK_DH_RSA_DES_192_CBC3_SHA,
	SSL_kDHr |SSL_aDH|SSL_3DES |SSL_SHA1|SSL_NOT_EXP|SSL_SSLV3|SSL_HIGH,
	0,
	SSL_ALL_CIPHERS,
	},

/* The Ephemeral DH ciphers */
/* Cipher 11 */
	{
	1,
	SSL3_TXT_EDH_DSS_DES_40_CBC_SHA,
	SSL3_CK_EDH_DSS_DES_40_CBC_SHA,
	SSL_kEDH|SSL_aDSS|SSL_DES|SSL_SHA1|SSL_EXP|SSL_SSLV3,
	0x205,	/* SSL-C: (export RSA 512 << 8) | 5 byte */
	SSL_ALL_CIPHERS,
	},
/* Cipher 12 */
	{
	1,
	SSL3_TXT_EDH_DSS_DES_64_CBC_SHA,
	SSL3_CK_EDH_DSS_DES_64_CBC_SHA,
	SSL_kEDH|SSL_aDSS|SSL_DES  |SSL_SHA1|SSL_NOT_EXP|SSL_SSLV3|SSL_LOW,
	0,
	SSL_ALL_CIPHERS,
	},
/* Cipher 13 */
	{
	1,
	SSL3_TXT_EDH_DSS_DES_192_CBC3_SHA,
	SSL3_CK_EDH_DSS_DES_192_CBC3_SHA,
	SSL_kEDH|SSL_aDSS|SSL_3DES |SSL_SHA1|SSL_NOT_EXP|SSL_SSLV3|SSL_HIGH,
	0,
	SSL_ALL_CIPHERS,
	},
/* Cipher 14 */
	{
	1,
	SSL3_TXT_EDH_RSA_DES_40_CBC_SHA,
	SSL3_CK_EDH_RSA_DES_40_CBC_SHA,
	SSL_kEDH|SSL_aRSA|SSL_DES|SSL_SHA1|SSL_EXP|SSL_SSLV3,
	0x205,	/* SSL-C: (export RSA 512 << 8) | 5 byte */
	SSL_ALL_CIPHERS,
	},
/* Cipher 15 */
	{
	1,
	SSL3_TXT_EDH_RSA_DES_64_CBC_SHA,
	SSL3_CK_EDH_RSA_DES_64_CBC_SHA,
	SSL_kEDH|SSL_aRSA|SSL_DES  |SSL_SHA1|SSL_NOT_EXP|SSL_SSLV3|SSL_LOW,
	0,
	SSL_ALL_CIPHERS,
	},
/* Cipher 16 */
	{
	1,
	SSL3_TXT_EDH_RSA_DES_192_CBC3_SHA,
	SSL3_CK_EDH_RSA_DES_192_CBC3_SHA,
	SSL_kEDH|SSL_aRSA|SSL_3DES |SSL_SHA1|SSL_NOT_EXP|SSL_SSLV3|SSL_HIGH,
	0,
	SSL_ALL_CIPHERS,
	},

/* Fortezza */
/* Cipher 1C */
	{
	0,
	SSL3_TXT_FZA_DMS_NULL_SHA,
	SSL3_CK_FZA_DMS_NULL_SHA,
	SSL_kFZA|SSL_aFZA |SSL_eNULL |SSL_SHA1|SSL_NOT_EXP|SSL_SSLV3,
	0,
	SSL_ALL_CIPHERS,
	},

/* Cipher 1D */
	{
	0,
	SSL3_TXT_FZA_DMS_FZA_SHA,
	SSL3_CK_FZA_DMS_FZA_SHA,
	SSL_kFZA|SSL_aFZA |SSL_eFZA |SSL_SHA1|SSL_NOT_EXP|SSL_SSLV3,
	0,
	SSL_ALL_CIPHERS,
	},

/* Cipher 1E */
	{
	0,
	SSL3_TXT_FZA_DMS_RC4_SHA,
	SSL3_CK_FZA_DMS_RC4_SHA,
	SSL_kFZA|SSL_aFZA |SSL_RC4  |SSL_SHA1|SSL_NOT_EXP|SSL_SSLV3,
	0,
	SSL_ALL_CIPHERS,
	},

/* SSL-C 2.1.0: the five TLS export 1024 and DHE-DSS-RC4 suites (0x03000062..66). */
/* RSA_EXPORT1024_WITH_RC4_56_SHA */
	{
	1,
	"EXP-RC4-56-SHA",
	0x03000064L,
	SSL_kRSA|SSL_aRSA|SSL_RC4|SSL_SHA|SSL_EXP|SSL_SSLV3,
	0x407,
	SSL_ALL_CIPHERS,
	},
/* RSA_EXPORT1024_WITH_DES_CBC_SHA */
	{
	1,
	"EXP-DES-56-SHA",
	0x03000062L,
	SSL_kRSA|SSL_aRSA|SSL_DES|SSL_SHA|SSL_EXP|SSL_SSLV3,
	0x408,
	SSL_ALL_CIPHERS,
	},
/* DHE_DSS_WITH_RC4_128_SHA */
	{
	1,
	"EDH-DSS-RC4-SHA",
	0x03000066L,
	SSL_kEDH|SSL_aDSS|SSL_RC4|SSL_SHA|SSL_SSLV3,
	0,
	SSL_ALL_CIPHERS,
	},
/* DHE_DSS_EXPORT1024_WITH_RC4_56_SHA */
	{
	1,
	"EXP-EDH-DSS-RC4-56-SHA",
	0x03000065L,
	SSL_kEDH|SSL_aDSS|SSL_RC4|SSL_SHA|SSL_EXP|SSL_SSLV3,
	0x407,
	SSL_ALL_CIPHERS,
	},
/* DHE_DSS_EXPORT1024_WITH_DES_CBC_SHA */
	{
	1,
	"EXP-EDH-DSS-DES-56-SHA",
	0x03000063L,
	SSL_kEDH|SSL_aDSS|SSL_DES|SSL_SHA|SSL_EXP|SSL_SSLV3,
	0x408,
	SSL_ALL_CIPHERS,
	},

/* end of list */
	};

static SSL3_ENC_METHOD SSLv3_enc_data={
	ssl3_enc,
	ssl3_mac,
	ssl3_setup_key_block,
	ssl3_generate_master_secret,
	ssl3_change_cipher_state,
	ssl3_final_finish_mac,
	MD5_DIGEST_LENGTH+SHA_DIGEST_LENGTH,
	ssl3_cert_verify_mac,
	SSL3_MD_CLIENT_FINISHED_CONST,4,
	SSL3_MD_SERVER_FINISHED_CONST,4,
	ssl3_alert_code,
	};

static SSL_METHOD SSLv3_data= {
	SSL3_VERSION,
	ssl3_new,
	ssl3_clear,
	ssl3_free,
	ssl_undefined_function,
	ssl_undefined_function,
	ssl3_read,
	ssl3_peek,
	ssl3_write,
	ssl3_shutdown,
	ssl3_renegotiate,
	ssl3_renegotiate_check,
	ssl3_ctrl,
	ssl3_ctx_ctrl,
	ssl3_get_cipher_by_char,
	ssl3_put_cipher_by_char,
	ssl3_pending,
	ssl3_num_ciphers,
	ssl3_get_cipher,
	ssl_bad_method,
	ssl3_default_timeout,
	&SSLv3_enc_data,
	};

static long ssl3_default_timeout()
	{
	/* 2 hours, the 24 hours mentioned in the SSLv3 spec
	 * is way too long for http, the cache would over fill */
	return(60*60*2);
	}

SSL_METHOD *sslv3_base_method()
	{
	return(&SSLv3_data);
	}

int ssl3_num_ciphers()
	{
	return(SSL3_NUM_CIPHERS);
	}

SSL_CIPHER *ssl3_get_cipher(u)
unsigned int u;
	{
	if (u < SSL3_NUM_CIPHERS)
		return(&(ssl3_ciphers[SSL3_NUM_CIPHERS-1-u]));
	else
		return(NULL);
	}

/* The problem is that it may not be the correct record type */
int ssl3_pending(s)
SSL *s;
	{
	if (s->rstate == SSL_ST_READ_BODY)
		{
		return(0);
		}
	else
		{
		return(s->s3->rrec.length);
		}
	}

#line 467 /* the line number is part of the binary */
int ssl3_new(s)
SSL *s;
	{
	SSL3_CTX *s3;

	if ((s3=(SSL3_CTX *)Malloc(sizeof(SSL3_CTX))) == NULL)
		{
		SSLerr(147,ERR_R_MALLOC_FAILURE);
		goto err;
		}
	memset(s3,0,sizeof(SSL3_CTX));

	if ((s3->finish_dgst1=EVP_MD_CTX_new()) == NULL)
		{
		SSLerr(147,ERR_R_MALLOC_FAILURE);
		goto err;
		}
	if ((s3->finish_dgst2=EVP_MD_CTX_new()) == NULL)
		{
		SSLerr(147,ERR_R_MALLOC_FAILURE);
		goto err;
		}

	s->s3=s3;
	s->method->ssl_clear(s);
	return(1);
err:
	return(0);
	}

void ssl3_free(s)
SSL *s;
	{
	if (s->s3->finish_dgst1 != NULL)
		EVP_MD_CTX_free(s->s3->finish_dgst1);
	if (s->s3->finish_dgst2 != NULL)
		EVP_MD_CTX_free(s->s3->finish_dgst2);
	if (s->s3->rbuf.buf != NULL)
		Free(s->s3->rbuf.buf);
	if (s->s3->wbuf.buf != NULL)
		Free(s->s3->wbuf.buf);
	if (s->s3->rrec.comp != NULL)
		Free(s->s3->rrec.comp);
	ssl3_cleanup_tmp(s);
	memset(s->s3,0,sizeof(SSL3_CTX));
	Free(s->s3);
	s->s3=NULL;
	}

void ssl3_clear(s)
SSL *s;
	{
	unsigned char *rp,*wp;
	EVP_MD_CTX *dgst1,*dgst2;
	long wlen,rlen;

	rp=s->s3->rbuf.buf;
	wp=s->s3->wbuf.buf;
	rlen=s->s3->rbuf.alloc_size;
	wlen=s->s3->wbuf.alloc_size;
	dgst1=s->s3->finish_dgst1;
	dgst2=s->s3->finish_dgst2;

	if (s->s3->rrec.comp != NULL)
		{
		Free(s->s3->rrec.comp);
		s->s3->rrec.comp=NULL;
		}
	ssl3_cleanup_tmp(s);

	memset(s->s3,0,sizeof(SSL3_CTX));
	s->s3->rbuf.alloc_size=rlen;
	s->s3->wbuf.alloc_size=wlen;
	s->s3->rbuf.buf=rp;
	s->s3->wbuf.buf=wp;
	s->s3->finish_dgst1=dgst1;
	s->s3->finish_dgst2=dgst2;
	if (s->s3->rbuf.buf != NULL)
		memset(s->s3->rbuf.buf,0,s->s3->rbuf.alloc_size);
	ssl_free_wbio_buffer(s);
	s->packet_length=0;
	s->s3->renegotiate=0;
	s->s3->total_renegotiations=0;
	s->s3->num_renegotiations=0;
	s->s3->in_read_app_data=0;
	s->version=SSL3_VERSION;
	s->s3->rrec.length=0;
	s->s3->rrec.off=0;
	}

static void ssl3_cleanup_tmp(s)
SSL *s;
	{
	ssl3_cleanup_key_block(s);
	if (s->s3->tmp.dh != NULL)
		{
		DH_free(s->s3->tmp.dh);
		s->s3->tmp.dh=NULL;
		}
	if (s->s3->tmp.ca_names != NULL)
		{
		sk_pop_free(s->s3->tmp.ca_names,SSLCERT_NAME_free);
		s->s3->tmp.ca_names=NULL;
		}
	return;
	}

long ssl3_ctrl(s,cmd,larg,parg)
SSL *s;
int cmd;
long larg;
char *parg;
	{
	int ret=0;

	switch (cmd)
		{
	case SSL_CTRL_GET_CLIENT_CERT_REQUEST:
		break;
	case SSL_CTRL_GET_NUM_RENEGOTIATIONS:
		ret=s->s3->num_renegotiations;
		break;
	case SSL_CTRL_CLEAR_NUM_RENEGOTIATIONS:
		ret=s->s3->num_renegotiations;
		s->s3->num_renegotiations=0;
		break;
	case SSL_CTRL_GET_TOTAL_RENEGOTIATIONS:
		ret=s->s3->total_renegotiations;
		break;
	case SSL_CTRL_GET_FLAGS:
		ret=(int)(s->s3->flags);
		break;
	case SSL_CTRL_EXTRA_CHAIN_CERT:
		if (s->extra_certs == NULL)
			{
			if ((s->extra_certs=sk_new(NULL)) == NULL)
				return(0);
			}
		if (sk_push(s->extra_certs,(char *)parg) > 0)
			{
			SSLCERT_reference_inc((X509 *)parg);
			ret=1;
			}
		break;
	case SSL_CTRL_CLEAR_EXTRA_CHAIN_CERTS:
		if (s->extra_certs != NULL)
			{
			sk_pop_free(s->extra_certs,SSLCERT_free);
			s->extra_certs=NULL;
			}
		ret=1;
		break;
	default:
		break;
		}
	return(ret);
	}

/* SSL-C: DH_params_dup is a library function. */
#line 683 /* the line number is part of the binary */
static int ssl3_set_tmp_dh(pp,dh)
DH **pp;
DH *dh;
	{
	DH *new;

	new=DH_params_dup(dh);
	if ((new == NULL) ||
		(!DH_generate_key(new)))
		{
		SSLerr(214,2053);
		return(0);
		}
	else
		{
		if (*pp != NULL)
			DH_free(*pp);
		*pp=new;
		return(1);
		}
	}

/* The same for RSA. */
static int ssl3_set_tmp_rsa(pp,rsa)
RSA **pp;
RSA *rsa;
	{
	RSA *new;

	new=RSA_dup(rsa);
	if (new == NULL)
		{
		SSLerr(215,2052);
		return(0);
		}
	else
		{
		if (*pp != NULL)
			RSA_free(*pp);
		*pp=new;
		return(1);
		}
	}

#line 729 /* the line number is part of the binary */
long ssl3_ctx_ctrl(ctx,cmd,larg,parg)
SSL_CTX *ctx;
int cmd;
long larg;
char *parg;
	{
	CERT *cert;

	cert=ctx->default_cert;

	switch (cmd)
		{
	case SSL_CTRL_NEED_TMP_RSA:
	case SSL_CTRL_RCOM_NEED_TMP_RSA_ALT:
		if (	(cert->rsa_tmp_b == NULL) &&
			((cert->pkeys[SSL_PKEY_RSA_ENC].privatekey == NULL) ||
			 (SSL_EVP_PKEY_size(cert->pkeys[SSL_PKEY_RSA_ENC].privatekey) > (512/8)))
			)
			return(1);
		else
			return(0);
	case SSL_CTRL_RCOM_NEED_TMP_RSA_C:
		if (	(cert->rsa_tmp_c == NULL) &&
			((cert->pkeys[SSL_PKEY_RSA_ENC].privatekey == NULL) ||
			 (SSL_EVP_PKEY_size(cert->pkeys[SSL_PKEY_RSA_ENC].privatekey) > (1024/8)))
			)
			return(1);
		else
			return(0);
	case SSL_CTRL_SET_TMP_RSA:
	case SSL_CTRL_RCOM_SET_TMP_RSA_ALT:
		{
		RSA *rsa;
		int bits;



		if (parg == NULL)
			{
			SSLerr(217,ERR_R_PASSED_NULL_PARAMETER);
			return(0);
			}
		rsa=(RSA *)parg;
		bits=RSA_size(rsa)*8;
		if (bits < 513)
			{
			if (!ssl3_set_tmp_rsa(&(cert->rsa_tmp_b),rsa)) return(0);
			if (cert->rsa_tmp == NULL)
				if (!ssl3_set_tmp_rsa(&(cert->rsa_tmp),rsa)) return(0);
			}
		else
			{
			if (!ssl3_set_tmp_rsa(&(cert->rsa_tmp),rsa)) return(0);
			}
		return(1);
		}
	case SSL_CTRL_RCOM_SET_TMP_RSA_C:
		{
		RSA *rsa;
		if (parg == NULL)
			{
			SSLerr(217,ERR_R_PASSED_NULL_PARAMETER);
			return(0);
			}
		rsa=(RSA *)parg;
		if (!ssl3_set_tmp_rsa(&(cert->rsa_tmp_c),rsa))
			return(0);
		return(1);
		}
	case SSL_CTRL_SET_TMP_RSA_CB:
		cert->rsa_tmp_cb=(RSA *(*)())parg;
		break;
	case SSL_CTRL_SET_TMP_DH:
		{


		DH *dh;
		int bits;

		if (parg == NULL)
			{
			SSLerr(217,ERR_R_PASSED_NULL_PARAMETER);
			return(0);
			}
		dh=(DH *)parg;
		bits=DH_size(dh)*8;
		if (bits < 513)
			{
			if (!ssl3_set_tmp_dh(&(cert->dh_tmp_b),dh))
				return(0);
			if (cert->dh_tmp == NULL)
				{
				if (!ssl3_set_tmp_dh(&(cert->dh_tmp),dh))
					return(0);
				}
			}
		else
			{
			if (!ssl3_set_tmp_dh(&(cert->dh_tmp),dh))
				return(0);
			}
		return(1);
		}
	case SSL_CTRL_SET_TMP_DH_CB:
		cert->dh_tmp_cb=(DH *(*)())parg;
		break;
	case SSL_CTRL_CTX_EXTRA_CHAIN_CERT:
		if (ctx->extra_certs == NULL)
			{
			if ((ctx->extra_certs=sk_new(NULL)) == NULL)
				return(0);
			}
		sk_push(ctx->extra_certs,(char *)parg);
		break;
	default:
		return(0);
		}
	return(1);
	}

SSL_CIPHER *ssl3_get_cipher_by_char(p)
unsigned char *p;
	{
	static int init=1;
	static SSL_CIPHER *sorted[SSL3_NUM_CIPHERS];
	SSL_CIPHER c,*cp= &c,**cpp;
	unsigned long id;
	int i;

	if (init)
		{
		init=0;

		for (i=0; i<(int)SSL3_NUM_CIPHERS; i++)
			sorted[i]= &(ssl3_ciphers[i]);

		qsort(	(char *)sorted,
			SSL3_NUM_CIPHERS,sizeof(SSL_CIPHER *),
			FP_ICC ssl_cipher_ptr_id_cmp);
		}

	id=0x03000000L|((unsigned long)p[0]<<8L)|(unsigned long)p[1];
	c.id=id;
	cpp=(SSL_CIPHER **)OBJ_bsearch((char *)&cp,
		(char *)sorted,
		SSL3_NUM_CIPHERS,sizeof(SSL_CIPHER *),
		(int (*)())ssl_cipher_ptr_id_cmp);
	if ((cpp == NULL) || !(*cpp)->valid)
		return(NULL);
	else
		return(*cpp);
	}

int ssl3_put_cipher_by_char(c,p)
SSL_CIPHER *c;
unsigned char *p;
	{
	long l;

	if (p != NULL)
		{
		l=c->id;
		if ((l & 0xff000000) != 0x03000000) return(0);
		p[0]=((unsigned char)(l>> 8L))&0xFF;
		p[1]=((unsigned char)(l     ))&0xFF;
		}
	return(2);
	}

SSL_CIPHER *ssl3_choose_cipher(s,have,pref)
SSL *s;
STACK *have,*pref;
	{
	SSL_CIPHER *c,*ret=NULL;
	int i,j,ok;
	CERT *cert;
	unsigned long alg,mask,emask;

	/* Lets see which ciphers we can supported */
	if (s->cert != NULL)
		cert=s->cert;
	else
		cert=s->ctx->default_cert;

	ssl_set_cert_masks(cert);
	mask=cert->mask;
	emask=cert->export_mask;
			
	sk_set_cmp_func(pref,ssl_cipher_ptr_id_cmp);

	for (i=0; i<sk_num(have); i++)
		{
		c=(SSL_CIPHER *)sk_value(have,i);
		alg=c->algorithms&(SSL_MKEY_MASK|SSL_AUTH_MASK);
		if (c->algorithms & SSL_EXPORT)
			{
			ok=((alg & emask) == alg)?1:0;
#ifdef CIPHER_DEBUG
			printf("%d:[%08lX:%08lX]%s\n",ok,alg,mask,c->name);
#endif
			}
		else
			{
			ok=((alg & mask) == alg)?1:0;
#ifdef CIPHER_DEBUG
			printf("%d:[%08lX:%08lX]%s\n",ok,alg,mask,c->name);
#endif
			}

		if (!ok) continue;
	
		j=sk_find(pref,(char *)c);
		if (j >= 0)
			{
			ret=(SSL_CIPHER *)sk_value(pref,j);
			break;
			}
		}
	return(ret);
	}

int ssl3_get_req_cert_type(s,p)
SSL *s;
unsigned char *p;
	{
	int ret=0;
	unsigned long alg;

	alg=s->s3->tmp.new_cipher->algorithms;

	/* SSL-C: the NO_DH / NO_DSA compile guards are runtime guards on SSL_feature_test. */
	if (SSL_feature_test(1))
		{
		if (alg & (SSL_kDHr|SSL_kEDH))
			{
			p[ret++]=SSL3_CT_RSA_FIXED_DH;
			p[ret++]=SSL3_CT_DSS_FIXED_DH;
			}
		if ((s->version == SSL3_VERSION) &&
			(alg & (SSL_kEDH|SSL_kDHd|SSL_kDHr)))
			{
			p[ret++]=SSL3_CT_RSA_EPHEMERAL_DH;
			p[ret++]=SSL3_CT_DSS_EPHEMERAL_DH;
			}
		}
	p[ret++]=SSL3_CT_RSA_SIGN;
	if (SSL_feature_test(2))
		p[ret++]=SSL3_CT_DSS_SIGN;
	return(ret);
	}

int ssl3_shutdown(s)
SSL *s;
	{
	int ret;

	/* Don't do anything much if we have not done the handshake or
	 * we don't want to send messages :-) */
	if ((SSL_ctrl(s,SSL_CTRL_OPTIONS,0,NULL) & 0x00020000L) ||
		((s->state == SSL_ST_BEFORE) &&
		(SSL_ctrl(s,SSL_CTRL_OPTIONS,0,NULL) & 0x00040000L)))
		{
		s->shutdown=(SSL_SENT_SHUTDOWN|SSL_RECEIVED_SHUTDOWN);
		return(1);
		}

	if (!(s->shutdown & SSL_SENT_SHUTDOWN))
		{
		s->shutdown|=SSL_SENT_SHUTDOWN;
		ret=ssl3_send_alert(s,SSL3_AL_WARNING,SSL_AD_CLOSE_NOTIFY);
		if (ret <= 0) return(ret);
		/* our shutdown alert has been sent now, and if it still needs
	 	 * to be written, s->s3->alert_dispatch will be true */
		}
	else if (s->s3->alert_dispatch)
		{
		/* resend it if not sent */
		ret=ssl3_dispatch_alert(s);
		if (ret <= 0) return(ret);
		}

	if ((s->shutdown & SSL_SENT_SHUTDOWN) &&
		(SSL_ctrl(s,SSL_CTRL_OPTIONS,0,NULL) & 0x00040000L))
		return(1);

	if (!(s->shutdown & SSL_RECEIVED_SHUTDOWN))
		{
		/* If we are waiting for a close from our peer, we are closed */
		ret=ssl3_read_bytes(s,0,NULL,0);
		if (ret <= 0) return(ret);
		}

	if ((s->shutdown == (SSL_SENT_SHUTDOWN|SSL_RECEIVED_SHUTDOWN)) &&
		!s->s3->alert_dispatch)
		return(1);
	else
		return(0);
	}

int ssl3_write(s,buf,len)
SSL *s;
char *buf;
int len;
	{
	int ret,n;

#if 0
	if (s->shutdown & SSL_SEND_SHUTDOWN)
		{
		s->rwstate=SSL_NOTHING;
		return(0);
		}
#endif
	clear_sys_error();
	if (s->s3->renegotiate) ssl3_renegotiate_check(s);

	/* This is an experimental flag that sends the
	 * last handshake message in the same packet as the first
	 * use data - used to see if it helps the TCP protocol during
	 * session-id reuse */
	/* The second test is because the buffer may have been removed */
	if ((s->s3->flags & SSL3_FLAGS_POP_BUFFER) && (s->wbio == s->bbio))
		{
		/* First time through, we write into the buffer */
		if (s->s3->delay_buf_pop_ret == 0)
			{
			ret=ssl3_write_bytes(s,SSL3_RT_APPLICATION_DATA,
				(char *)buf,len);
			if (ret <= 0) return(ret);

			s->s3->delay_buf_pop_ret=ret;
			}
flush:
		s->rwstate=SSL_WRITING;
		n=BIO_flush(s->wbio);
		if (n <= 0) return(n);
		s->rwstate=SSL_NOTHING;

		/* We have flushed the buffer, so remove it */
		ssl_free_wbio_buffer(s);
		s->s3->flags&= ~SSL3_FLAGS_POP_BUFFER;

		ret=s->s3->delay_buf_pop_ret;
		s->s3->delay_buf_pop_ret=0;
		}
	else
		{
		ret=ssl3_write_bytes(s,SSL3_RT_APPLICATION_DATA,
			(char *)buf,len);
		if (ret <= 0) return(ret);
		if ((s->s3->flags & SSL3_FLAGS_POP_BUFFER) &&
			(s->wbio == s->bbio))
			{
			s->s3->delay_buf_pop_ret=ret;
			goto flush;
			}
		}

	return(ret);
	}

int ssl3_read(s,buf,len)
SSL *s;
char *buf;
int len;
	{
	int ret;
	
	clear_sys_error();
	if (s->s3->renegotiate) ssl3_renegotiate_check(s);
	s->s3->in_read_app_data=1;
	ret=ssl3_read_bytes(s,SSL3_RT_APPLICATION_DATA,buf,len);
	if ((ret == -1) && (s->s3->in_read_app_data == 2))
		{
		ERR_clear_error();
		s->s3->in_read_app_data=0;
		s->in_handshake++;
		ret=ssl3_read_bytes(s,SSL3_RT_APPLICATION_DATA,buf,len);
		s->in_handshake--;
		}
	else
		s->s3->in_read_app_data=0;

	return(ret);
	}

int ssl3_peek(s,buf,len)
SSL *s;
char *buf;
int len;
	{
	SSL3_RECORD *rr;
	int n;

	rr= &(s->s3->rrec);
	if ((rr->length == 0) || (rr->type != SSL3_RT_APPLICATION_DATA))
		{
		n=ssl3_read(s,buf,1);
		if (n <= 0) return(n);
		rr->length++;
		rr->off--;
		}

	if ((unsigned int)len > rr->length)
		n=rr->length;
	else
		n=len;
	memcpy(buf,&(rr->data[rr->off]),(unsigned int)n);
	return(n);
	}


