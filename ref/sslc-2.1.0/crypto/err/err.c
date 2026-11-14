/* crypto/err/err.c */
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
#include "lhash.h"
#include "crypto.h"
#include "cryptlib.h"
#include "buffer.h"
#include "err.h"
#include "crypto.h"









#ifndef NO_ERR
static ERR_STRING_DATA ERR_str_libraries[]=
	{
{ERR_PACK(ERR_LIB_NONE,0,0)		,"unknown library"},
{ERR_PACK(ERR_LIB_SYS,0,0)		,"system library"},
{ERR_PACK(ERR_LIB_BN,0,0)		,"bignum routines"},
{ERR_PACK(ERR_LIB_RSA,0,0)		,"rsa routines"},
{ERR_PACK(ERR_LIB_DH,0,0)		,"Diffie-Hellman routines"},
{ERR_PACK(ERR_LIB_EVP,0,0)		,"digital envelope routines"},
{ERR_PACK(ERR_LIB_BUF,0,0)		,"memory buffer routines"},
{ERR_PACK(ERR_LIB_BIO,0,0)		,"BIO routines"},
{ERR_PACK(ERR_LIB_OBJ,0,0)		,"object identifier routines"},
{ERR_PACK(ERR_LIB_PEM,0,0)		,"PEM routines"},
{ERR_PACK(ERR_LIB_ASN1,0,0)		,"asn1 encoding routines"},
{ERR_PACK(ERR_LIB_X509,0,0)		,"x509 certificate routines"},
{ERR_PACK(ERR_LIB_CONF,0,0)		,"configuation file routines"},
{ERR_PACK(ERR_LIB_METH,0,0)		,"X509 lookup 'method' routines"},
{ERR_PACK(ERR_LIB_SSL,0,0)		,"SSL routines"},
{ERR_PACK(ERR_LIB_RSAREF,0,0)		,"RSAref routines"},
{ERR_PACK(ERR_LIB_PROXY,0,0)		,"Proxy routines"},
{ERR_PACK(ERR_LIB_BIO,0,0)		,"BIO routines"},
{ERR_PACK(ERR_LIB_PKCS7,0,0)		,"PKCS7 routines"},
{ERR_PACK(ERR_LIB_SIO,0,0)		,"SIO routines"},
{ERR_PACK(ERR_LIB_X509TC,0,0)		,"X509TC routines"},
{ERR_PACK(ERR_LIB_SSLCERT,0,0)		,"SSLCERT routines"},
{ERR_PACK(ERR_LIB_PKEY,0,0)		,"PKEY routines"},
{ERR_PACK(ERR_LIB_WTLSCERT,0,0)		,"WTLSCERT routines"},
{ERR_PACK(ERR_LIB_HW,0,0)		,"Hardware support routines"},
{0,NULL},
	};

static ERR_STRING_DATA ERR_str_functs[]=
	{
	{ERR_PACK(0,SYS_F_FOPEN,0),     	"fopen"},
	{ERR_PACK(0,SYS_F_CONNECT,0),		"connect"},
	{ERR_PACK(0,SYS_F_GETSERVBYNAME,0),	"getservbyname"},
	{ERR_PACK(0,SYS_F_SOCKET,0),		"socket"}, 
	{ERR_PACK(0,SYS_F_IOCTLSOCKET,0),	"ioctlsocket"},
	{ERR_PACK(0,SYS_F_BIND,0),		"bind"},
	{ERR_PACK(0,SYS_F_LISTEN,0),		"listen"},
	{ERR_PACK(0,SYS_F_ACCEPT,0),		"accept"},
#ifdef WINDOWS
	{ERR_PACK(0,SYS_F_WSASTARTUP,0),	"WSAstartup"},
#endif
	{0,NULL},
	};

static ERR_STRING_DATA ERR_str_reasons[]=
	{
{ERR_R_FATAL                             ,"fatal"},
{ERR_R_SYS_LIB				,"system lib"},
{ERR_R_BN_LIB				,"BN lib"},
{ERR_R_RSA_LIB				,"RSA lib"},
{ERR_R_DH_LIB				,"DH lib"},
{ERR_R_EVP_LIB				,"EVP lib"},
{ERR_R_BUF_LIB				,"BUF lib"},
{ERR_R_BIO_LIB				,"BIO lib"},
{ERR_R_OBJ_LIB				,"OBJ lib"},
{ERR_R_PEM_LIB				,"PEM lib"},
{ERR_R_X509_LIB				,"X509 lib"},
{ERR_R_METH_LIB				,"METH lib"},
{ERR_R_ASN1_LIB				,"ASN1 lib"},
{ERR_R_CONF_LIB				,"CONF lib"},
{ERR_R_SSL_LIB				,"SSL lib"},
{ERR_R_PROXY_LIB			,"PROXY lib"},
{ERR_R_BIO_LIB				,"BIO lib"},
{ERR_R_PKCS7_LIB			,"PKCS7 lib"},
{ERR_R_SIO_LIB				,"SIO lib"},
{ERR_R_X509TC_LIB			,"X509TC lib"},
{ERR_R_SSLCERT_LIB			,"SSLCERT lib"},
{ERR_R_PKEY_LIB				,"PKEY lib"},
{ERR_R_WTLSCERT_LIB			,"WTLSCERT lib"},
{ERR_R_MALLOC_FAILURE			,"Malloc failure"},
{ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED	,"called a fuction you should not call"},
{ERR_R_INTERNAL_ERROR			,"suffered a general internal failure"},
{ERR_R_PASSED_NULL_PARAMETER		,"passed a null parameter"},
{ERR_R_NESTED_ASN1_ERROR		,"nested asn1 error"},
{ERR_R_BAD_ASN1_OBJECT_HEADER		,"bad asn1 object header"},
{ERR_R_BAD_GET_ASN1_OBJECT_CALL		,"bad get asn1 object call"},
{ERR_R_EXPECTING_AN_ASN1_SEQUENCE	,"expecting an asn1 sequence"},
{ERR_R_ASN1_LENGTH_MISMATCH		,"asn1 length mismatch"},
{ERR_R_MISSING_ASN1_EOS			,"missing asn1 eos"},
{ERR_R_FATAL_INTERNAL_ERROR		,"fatal internal error"},
{ERR_R_EVAL_RESTRICTION			,"eval restriction"},
{ERR_R_EVAL_EXPIRED			,"eval expired"},
{ERR_R_INIT_NOT_CALLED			,"init not called"},
{ERR_R_NOT_SUPPORTED			,"not supported"},
{ERR_R_BUFFER_TOO_SMALL			,"buffer too small"},
{0,NULL},
	};
#endif

#define err_clear_data(p,i) \
	if (((p)->err_data[i] != NULL) && \
		(p)->err_data_flags[i] & ERR_TXT_MALLOCED) \
		{  \
		Free((p)->err_data[i]); \
		(p)->err_data[i]=NULL; \
		} \
	(p)->err_data_flags[i]=0;

void ERR_load_ERR_strings()
	{
	static int init=1;

	if (init)
		{
		CRYPTO_w_lock(CRYPTO_LOCK_ERR);
		if (init == 0)
			{
			CRYPTO_w_unlock(CRYPTO_LOCK_ERR);
			return;
			}
		init=0;
		CRYPTO_w_unlock(CRYPTO_LOCK_ERR);

#ifndef NO_ERR
		ERR_load_strings(0,ERR_str_libraries);
		ERR_load_strings(0,ERR_str_reasons);
		ERR_load_strings(ERR_LIB_SYS,ERR_str_functs);
#endif
		}
	}
