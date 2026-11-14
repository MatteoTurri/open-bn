/*
 * Chloe library: ChloeCake (loads a blob into /dev/akmem) and two hash setters.
 */

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "chloe.h"

/* The blob is loaded 1 MB into /dev/akmem. */
#define AKMEM_LOAD_OFFSET 0x100000

/* Requests of /dev/akmem: the window {1, &{start, end}} and the start {offset, nslots, slots}. */
#define AKMEM_IOC_SETWINDOW _IOW('W', 0, int[2])
#define AKMEM_IOC_START _IOW('W', 1, int[3])

/* Loads `buf` into /dev/akmem at AKMEM_LOAD_OFFSET: unscrambles it in place
   (buf[i] ^= ~buf[i-1]), sets the device window (the offset rounded down and
   size + offset rounded up to the page size), writes it and starts it. Returns 0, or the positive errno of the
   first failure. */
int ChloeCake(void *buf, int size, int nslots, int slots)
{
	int *p = (int *)buf;
	int fd;
	int i, n;
	int windowReq[2];
	int window[2];
	int *win;
	int startReq[3];

	for (i = 1, n = size / 4; i < n; i++)
		p[i] ^= ~p[i - 1];

	fd = open("/dev/akmem", O_WRONLY);
	if (fd < 0)
		goto err;

	win = window;
	windowReq[0] = 1;
	windowReq[1] = (int)win;
	win[0] = (AKMEM_LOAD_OFFSET / getpagesize()) * getpagesize();
	win[1] = ((size + AKMEM_LOAD_OFFSET + getpagesize() - 1) / getpagesize()) * getpagesize();
	if (ioctl(fd, AKMEM_IOC_SETWINDOW, windowReq) < 0)
		goto err;
	if (lseek(fd, AKMEM_LOAD_OFFSET, SEEK_SET) < 0)
		goto err;
	if (write(fd, buf, size) < 0)
		goto err;
	startReq[0] = AKMEM_LOAD_OFFSET;
	startReq[1] = nslots;
	startReq[2] = slots;
	if (ioctl(fd, AKMEM_IOC_START, startReq) < 0)
		goto err;
	if (close(fd) < 0)
		goto err;
	return 0;
err:
	return errno;
}

int ChloeSlices(ChloeObj *p, unsigned int addr)
{
	p->hash += 1;
	return 0;
}

int ChloePlum(ChloeObj *p, int value)
{
	p->hash += 0x12342;
	return 0;
}
