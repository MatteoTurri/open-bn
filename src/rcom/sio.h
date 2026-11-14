/*
 * SIO_*: the socket helpers of RCOM (sio_ioctl.c, sio_nbio.c, sio_retry.c).
 */

#ifndef RCOM_SIO_H
#define RCOM_SIO_H

int SIO_socket_ioctl(int s, long type, unsigned long *arg);
int SIO_socket_nbio(int s, int mode);
int SIO_sock_should_retry(int s);
int SIO_sock_non_fatal_error(int err);

#endif
