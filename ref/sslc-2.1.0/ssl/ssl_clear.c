/*
 * SSL_clear and ssl_clear_internal: SSL_clear is a wrapper of ssl_clear_internal in SSL-C.
 */
#include <stdio.h>
#include "objects.h"
#include "lhash.h"
#include "ssl_locl.h"


int SSL_clear(s)
SSL *s;
	{
	return(ssl_clear_internal(s,0));
	}

int ssl_clear_internal(s,flag)
SSL *s;
int flag;
	{
	int state;

	if (s->method == NULL)
		{
#line 162 /* the line number is part of the binary */
		SSLerr(SSL_F_SSL_CLEAR,SSL_R_NO_METHOD_SPECIFIED);
		return(0);
		}

	if (ssl_clear_bad_session(s))
		{
		SSL_SESSION_free(s->session);
		s->session=NULL;
		}

	s->error=0;
	s->hit=0;
	s->shutdown=0;

	state=s->state;
	s->type=0;

	s->state=(s->new_session)?(SSL_ST_BEFORE|SSL_ST_ACCEPT):
		(SSL_ST_BEFORE|SSL_ST_CONNECT);
	s->version=s->method->version;
	s->client_version=s->version;
	s->rwstate=SSL_NOTHING;
	s->rstate=SSL_ST_READ_HEADER;
	s->read_ahead=s->ctx->default_read_ahead;

	if (s->init_buf != NULL)
		{
		BUF_MEM_free(s->init_buf);
		s->init_buf=NULL;
		}

	ssl_clear_cipher_ctx(s);
	s->first_packet=0;

	if (!flag && ((s->session == NULL) || (s->handshake_func == SSL_accept)))
		{
		if (!SSL_set_ssl_method(s,s->ctx->method))
			return(0);
		}
	else
		s->method->ssl_clear(s);
	return(1);
	}
