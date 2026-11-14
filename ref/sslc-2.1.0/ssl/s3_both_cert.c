/* ssl/s3_both.c */
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


/*
 * ssl_cert_type and ssl_verify_alarm_type, from ssl/s3_both.c.
 */
#include <stdio.h>
#include "buffer.h"
#include "rand.h"
#include "objects.h"
#include "evp.h"
#include "x509.h"
#include "ssl_locl.h"


int ssl_cert_type(x,pkey)
X509 *x;
EVP_PKEY *pkey;
	{
	EVP_PKEY *pk;
	int ret= -1,i,j;

	if (pkey == NULL)
		pk=SSLCERT_get_pubkey(x);
	else
		pk=pkey;
	if (pk == NULL) goto err;

	i=pk->type;
	if (i == EVP_PKEY_RSA)
		{
		ret=SSL_PKEY_RSA_ENC;
		}
	else if (i == EVP_PKEY_DSA)
		{
		ret=SSL_PKEY_DSA_SIGN;
		}
	else if (i == EVP_PKEY_DH)
		{
		/* if we just have a key, we needs to be guess */

		if (x == NULL)
			ret=SSL_PKEY_DH_DSA;
		else
			{
			j=SSLCERT_get_signature_type(x);
			if (j == EVP_PKEY_RSA)
				ret=SSL_PKEY_DH_RSA;
			else if (j== EVP_PKEY_DSA)
				ret=SSL_PKEY_DH_DSA;
			else ret= -1;
			}
		}
	else
		ret= -1;

err:
	return(ret);
	}

int ssl_verify_alarm_type(type)
long type;
	{
	int al;

	switch(type)
		{
	case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
	case X509_V_ERR_UNABLE_TO_GET_CRL:
		al=SSL_AD_UNKNOWN_CA;
		break;
	case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
	case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE:
	case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
	case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
	case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
	case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD:
	case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD:
	case X509_V_ERR_CERT_NOT_YET_VALID:
	case X509_V_ERR_CRL_NOT_YET_VALID:
		al=SSL_AD_BAD_CERTIFICATE;
		break;
	case X509_V_ERR_CERT_SIGNATURE_FAILURE:
	case X509_V_ERR_CRL_SIGNATURE_FAILURE:
		al=SSL_AD_DECRYPT_ERROR;
		break;
	case X509_V_ERR_CERT_HAS_EXPIRED:
	case X509_V_ERR_CRL_HAS_EXPIRED:
		al=SSL_AD_CERTIFICATE_EXPIRED;
		break;
	case X509_V_ERR_CERT_REVOKED:
		al=SSL_AD_CERTIFICATE_REVOKED;
		break;
	case X509_V_ERR_OUT_OF_MEM:
		al=SSL_AD_INTERNAL_ERROR;
		break;
	case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
	case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
	case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
	case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
	case X509_V_ERR_CERT_CHAIN_TOO_LONG:
		al=SSL_AD_UNKNOWN_CA;
		break;
	case X509_V_ERR_APPLICATION_VERIFICATION:
		al=SSL_AD_HANDSHAKE_FAILURE;
		break;
	default:
		al=SSL_AD_CERTIFICATE_UNKNOWN;
		break;
		}
	return(al);
	}
