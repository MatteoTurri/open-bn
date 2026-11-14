/*
 * SSL_SESSION_hash and SSL_SESSION_cmp, from ssl/ssl_lib.c.
 */
#include <stdio.h>
#include "objects.h"
#include "lhash.h"
#include "ssl_locl.h"

unsigned long SSL_SESSION_hash(a)
SSL_SESSION *a;
	{
	unsigned long l;

	l=      (a->session_id[0]     )|(a->session_id[1]<< 8L)|
		(a->session_id[2]<<16L)|(a->session_id[3]<<24L);
	return(l);
	}
