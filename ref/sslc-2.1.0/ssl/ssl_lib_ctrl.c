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
 * SSL_shutdown and SSL_ctrl, from ssl/ssl_lib.c.
 */
#include <stdio.h>
#include "objects.h"
#include "lhash.h"
#include "ssl_locl.h"


/* SSL-C: struct ssl_rw_st (the two symmetric blocks of ssl_st) is in ssl.h: ssl3_change_cipher_state
 * uses it too. */

/* Used to change an SSL_CTXs default SSL method type */

int SSL_shutdown(s)
SSL *s;
	{
	if ((s != NULL) && (!SSL_in_init(s) || (s->options & 0x00040000L)))
		return(s->method->ssl_shutdown(s));
	else
		return(1);
	}

/* SSL-C: a switch(cmd) dispatcher, not the stock one-liner. */
long SSL_ctrl(s,cmd,larg,parg)
SSL *s;
int cmd;
long larg;
char *parg;
	{
	long ret;

	switch (cmd)
		{
	case SSL_CTRL_GET_READ_AHEAD:
		return(s->read_ahead);
	case SSL_CTRL_SET_READ_AHEAD:
		ret=s->read_ahead;
		s->read_ahead=larg;
		return(ret);
	case SSL_CTRL_SET_RCOM_LIMIT:
		ret=0;
		if (((long)parg == SSL_CTRL_RCOM_SEL_FIRST) ||
			((long)parg == SSL_CTRL_RCOM_SEL_BOTH))
			{
			s->rcom_limit=larg;
			ret=1;
			}
		if (((long)parg == SSL_CTRL_RCOM_SEL_SECOND) ||
			((long)parg == SSL_CTRL_RCOM_SEL_BOTH))
			{
			s->max_cert_list=larg;
			ret=1;
			}
		return(ret);
	case SSL_CTRL_GET_RCOM_LIMIT:
		ret=102400;
		if ((long)parg == SSL_CTRL_RCOM_SEL_FIRST)
			{
			if (s->rcom_limit != 0)
				ret=s->rcom_limit;
			}
		else if ((long)parg == SSL_CTRL_RCOM_SEL_SECOND)
			{
			if (s->max_cert_list != 0)
				ret=s->max_cert_list;
			}
		else
			ret=0;
		return(ret);
	case SSL_CTRL_OPTIONS:
		return(s->options|=larg);
	case SSL_CTRL_GET_DEFAULT_WBUF_SIZE:
		ret=s->wbuf_size;
		return(ret);
	case SSL_CTRL_SET_DEFAULT_WBUF_SIZE:
		ret=s->wbuf_size;
		s->wbuf_size=larg;
		return(ret);
	case SSL_CTRL_GET_DEFAULT_RBUF_SIZE:
		ret=s->rbuf_size;
		return(ret);
	case SSL_CTRL_SET_DEFAULT_RBUF_SIZE:
		ret=s->rbuf_size;
		s->rbuf_size=larg;
		return(ret);
	case SSL_CTRL_SET_DEBUG:
		ret=s->debug;
		s->debug=larg;
		return(ret);
	case SSL_CTRL_GET_REFERENCES:
		return(s->references);
	default:
		return(s->method->ssl_ctrl(s,cmd,larg,parg));
		}
	}
