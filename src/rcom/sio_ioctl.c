/*
 * SIO_socket_ioctl: RCOM fork of BIO_socket_ioctl; a failure is reported as
 * ERR_LIB_SYS / SYS_F_IOCTLSOCKET with errno as reason.
 */


#include <stdio.h>
#include <errno.h>
#include "sio.h"
#include <sys/ioctl.h>
#include "cryptlib.h"


int SIO_socket_ioctl(int s, long type, unsigned long *arg)
{
	int i;

	i = ioctl(s, type, arg);
	if (i < 0)
	/* clang-format off */
#line 324
		ERR_put_error(ERR_LIB_SYS, SYS_F_IOCTLSOCKET, errno, __FILE__, __LINE__);  /* the line number is part of the binary */
	/* clang-format on */
	return i;
}
