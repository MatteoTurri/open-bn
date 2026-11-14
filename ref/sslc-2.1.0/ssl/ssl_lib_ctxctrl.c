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
 * SSL_CTX_ctrl, ssl_cipher_id_cmp and ssl_cipher_ptr_id_cmp, from ssl/ssl_lib.c.
 */

#include <stdio.h>
#include "objects.h"
#include "lhash.h"
#include "ssl_locl.h"


/* SSL-C: struct ssl_rw_st (the two symmetric blocks of ssl_st) is in ssl.h: ssl3_change_cipher_state
 * uses it too. */


/* SSL-C: a switch(cmd) dispatcher, not the stock one-liner. */
long SSL_CTX_ctrl(ctx,cmd,larg,parg)
SSL_CTX *ctx;
int cmd;
long larg;
char *parg;
	{
	long ret;

	switch (cmd)
		{
	case SSL_CTRL_GET_READ_AHEAD:
		return(ctx->default_read_ahead);
	case SSL_CTRL_SET_READ_AHEAD:
		ret=ctx->default_read_ahead;
		ctx->default_read_ahead=larg;
		return(ret);
	case SSL_CTRL_SET_RCOM_LIMIT:
		ret=0;
		if (((long)parg == SSL_CTRL_RCOM_SEL_FIRST) ||
			((long)parg == SSL_CTRL_RCOM_SEL_BOTH))
			{
			ctx->rcom_limit=larg;
			ret=1;
			}
		if (((long)parg == SSL_CTRL_RCOM_SEL_SECOND) ||
			((long)parg == SSL_CTRL_RCOM_SEL_BOTH))
			{
			ctx->max_cert_list=larg;
			ret=1;
			}
		return(ret);
	case SSL_CTRL_GET_RCOM_LIMIT:
		ret=102400;
		if ((long)parg == SSL_CTRL_RCOM_SEL_FIRST)
			{
			if (ctx->rcom_limit != 0)
				ret=ctx->rcom_limit;
			}
		else if ((long)parg == SSL_CTRL_RCOM_SEL_SECOND)
			{
			if (ctx->max_cert_list != 0)
				ret=ctx->max_cert_list;
			}
		else
			ret=0;
		return(ret);
	case SSL_CTRL_SET_SESS_CACHE_SIZE:
		ret=ctx->session_cache_size;
		ctx->session_cache_size=larg;
		return(ret);
	case SSL_CTRL_GET_SESS_CACHE_SIZE:
		return(ctx->session_cache_size);
	case SSL_CTRL_SET_SESS_CACHE_MODE:
		ret=ctx->session_cache_mode;
		ctx->session_cache_mode=larg;
		return(ret);
	case SSL_CTRL_GET_SESS_CACHE_MODE:
		return(ctx->session_cache_mode);
	case SSL_CTRL_SESS_NUMBER:
		return(ctx->sessions->num_items);
	case SSL_CTRL_SESS_CONNECT:
		return(ctx->sess_connect);
	case SSL_CTRL_SESS_CONNECT_GOOD:
		return(ctx->sess_connect_good);
	case SSL_CTRL_SESS_CONNECT_RENEGOTIATE:
		return(ctx->sess_connect_renegotiate);
	case SSL_CTRL_SESS_ACCEPT:
		return(ctx->sess_accept);
	case SSL_CTRL_SESS_ACCEPT_GOOD:
		return(ctx->sess_accept_good);
	case SSL_CTRL_SESS_ACCEPT_RENEGOTIATE:
		return(ctx->sess_accept_renegotiate);
	case SSL_CTRL_SESS_HIT:
		return(ctx->sess_hit);
	case SSL_CTRL_SESS_CB_HIT:
		return(ctx->sess_cb_hit);
	case SSL_CTRL_SESS_MISSES:
		return(ctx->sess_miss);
	case SSL_CTRL_SESS_TIMEOUTS:
		return(ctx->sess_timeout);
	case SSL_CTRL_SESS_CACHE_FULL:
		return(ctx->sess_cache_full);
	case SSL_CTRL_OPTIONS:
		return(ctx->options|=larg);
	case SSL_CTRL_GET_DEFAULT_WBUF_SIZE:
		return(ctx->default_wbuf_size);
	case SSL_CTRL_SET_DEFAULT_WBUF_SIZE:
		ret=ctx->default_wbuf_size;
		ctx->default_wbuf_size=larg;
		return(ret);
	case SSL_CTRL_GET_DEFAULT_RBUF_SIZE:
		return(ctx->default_rbuf_size);
	case SSL_CTRL_SET_DEFAULT_RBUF_SIZE:
		ret=ctx->default_rbuf_size;
		ctx->default_rbuf_size=larg;
		return(ret);
	case SSL_CTRL_GET_REFERENCES:
		return(ctx->references);
	default:
		return(ctx->method->ssl_ctx_ctrl(ctx,cmd,larg,parg));
		}
	}

int ssl_cipher_id_cmp(a,b)
SSL_CIPHER *a,*b;
	{
	long l;

	l=a->id-b->id;
	if (l == 0L)
		return(0);
	else
		return((l > 0)?1:-1);
	}

int ssl_cipher_ptr_id_cmp(ap,bp)
SSL_CIPHER **ap,**bp;
	{
	long l;

	l=(*ap)->id-(*bp)->id;
	if (l == 0L)
		return(0);
	else
		return((l > 0)?1:-1);
	}
