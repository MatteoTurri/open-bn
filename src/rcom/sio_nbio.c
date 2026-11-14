/*
 * SIO_socket_nbio: puts a socket in non-blocking mode (RCOM fork of
 * BIO_socket_nbio).
 */

#include "sio.h"

#define FIONBIO 0x667e

int SIO_socket_nbio(int s, int mode)
{
	int ret = -1;
	int l;

	l = mode;
	ret = SIO_socket_ioctl(s, FIONBIO, (unsigned long *)&l);
	return (ret == 0);
}
