/*
 * SSL_SESSION_hash and SSL_SESSION_cmp, from ssl/ssl_lib.c.
 */
#include <stdio.h>
#include "objects.h"
#include "lhash.h"
#include "ssl_locl.h"

int SSL_SESSION_cmp(a, b)
SSL_SESSION *a;
SSL_SESSION *b;
	{
	if (a->ssl_version != b->ssl_version)
		return(1);
	if (a->session_id_length != b->session_id_length)
		return(1);
	return(memcmp(a->session_id,b->session_id,a->session_id_length));
	}
