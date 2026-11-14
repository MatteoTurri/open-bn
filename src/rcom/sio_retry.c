/*
 * SIO_sock_should_retry and SIO_sock_non_fatal_error: RCOM fork of the
 * socket BIO helpers (no ENOTCONN case).
 */

#include <errno.h>
#include "sio.h"


int SIO_sock_should_retry(int s)
{
	int err;

	if ((s == 0) || (s == -1)) {
		err = errno;
		return SIO_sock_non_fatal_error(err);
	}
	return 0;
}

int SIO_sock_non_fatal_error(int err)
{
	switch (err) {
	case EWOULDBLOCK:
	case EINTR:
	case EPROTO:
	case EINPROGRESS:
	case EALREADY:
		return 1;
	default:
		break;
	}
	return 0;
}
