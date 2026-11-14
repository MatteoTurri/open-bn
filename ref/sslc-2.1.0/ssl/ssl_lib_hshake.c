/* ssl/ssl_lib.c */
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
 * SSL_set_ssl_method, SSL_get_error and SSL_do_handshake, from ssl/ssl_lib.c.
 */
#include <stdio.h>
#include "objects.h"
#include "lhash.h"
#include "ssl_locl.h"


/* SSL-C: struct ssl_rw_st (the two symmetric blocks of ssl_st) is in ssl.h: ssl3_change_cipher_state
 * uses it too. */

/* Used to change an SSL_CTXs default SSL method type */

int SSL_set_ssl_method(s,meth)
SSL *s;
SSL_METHOD *meth;
	{
	int ret=1;

	if (s->method != meth)
		{
		if (s->method->version == meth->version)
			s->method=meth;
		else
			{
			s->method->ssl_free(s);
			s->method=meth;
			ret=s->method->ssl_new(s);
			}
		}
	s->method->ssl_clear(s);
	return(ret);
	}

int SSL_get_error(s,i)
SSL *s;
int i;
	{
	int reason;
	unsigned long l;
	BIO *bio;

	if (i > 0) return(SSL_ERROR_NONE);

	/* Make things return SSL_ERROR_SYSCALL when doing SSL_do_handshake
	 * etc, where we do encode the error */
	if ((l=ERR_peek_error()) != 0)
		{
		if (ERR_GET_LIB(l) == ERR_LIB_SYS)
			return(SSL_ERROR_SYSCALL);
		else
			return(SSL_ERROR_SSL);
		}

	if ((i < 0) && SSL_want_read(s))
		{
		bio=SSL_get_rbio(s);
		if (BIO_should_read(bio))
			return(SSL_ERROR_WANT_READ);
		else if (BIO_should_write(bio))
			return(SSL_ERROR_WANT_WRITE);
		else if (BIO_should_io_special(bio))
			{
			reason=BIO_get_retry_reason(bio);
			if (reason == BIO_RR_CONNECT)
				return(SSL_ERROR_WANT_CONNECT);
			else
				return(SSL_ERROR_SYSCALL); /* unknown */
			}
		}

	if ((i < 0) && SSL_want_write(s))
		{
		bio=SSL_get_wbio(s);
		if (BIO_should_write(bio))
			return(SSL_ERROR_WANT_WRITE);
		else if (BIO_should_read(bio))
			return(SSL_ERROR_WANT_READ);
		else if (BIO_should_io_special(bio))
			{
			reason=BIO_get_retry_reason(bio);
			if (reason == BIO_RR_CONNECT)
				return(SSL_ERROR_WANT_CONNECT);
			else
				return(SSL_ERROR_SYSCALL);
			}
		}
	if ((i < 0) && SSL_want_x509_lookup(s))
		{
		return(SSL_ERROR_WANT_X509_LOOKUP);
		}

	if (i == 0)
		{
		if (s->version == SSL2_VERSION)
			{
			/* assume it is the socket being closed */
			return(SSL_ERROR_ZERO_RETURN);
			}
		else
			{
			if ((s->shutdown & SSL_RECEIVED_SHUTDOWN) &&
				(s->s3->warn_alert == SSL_AD_CLOSE_NOTIFY))
				return(SSL_ERROR_ZERO_RETURN);
			}
		}
	return(SSL_ERROR_SYSCALL);
	}

int SSL_do_handshake(s)
SSL *s;
	{
	int ret=1;

	if (s->handshake_func == NULL)
		{
#line 1911 /* the line number is part of the binary */
		SSLerr(SSL_F_SSL_DO_HANDSHAKE,SSL_R_CONNECTION_TYPE_NOT_SET);
		return(-1);
		}
	s->method->ssl_renegotiate_check(s);
	if (SSL_in_init(s) || SSL_in_before(s))
		{
		ret=s->handshake_func(s);
		}
	return(ret);
	}
