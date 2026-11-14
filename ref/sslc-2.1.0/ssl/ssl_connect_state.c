/*
 * SSL_set_connect_state, from ssl/ssl_lib.c.
 */
#include <stdio.h>
#include "objects.h"
#include "lhash.h"
#include "ssl_locl.h"


void SSL_set_connect_state(s)
SSL *s;
	{
	s->new_session=0;
	s->shutdown=0;
	s->state=SSL_ST_CONNECT|SSL_ST_BEFORE;
	s->handshake_func=SSL_connect;
	ssl_clear_internal(s,0);
	}
