/*
 * Chloe library: the device A constructor and ChloeSnacks.
 */

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include "chloe.h"

/* Device A constructor: installs the libc table in field order and mixes the hash. */
int ChloeLemon(ChloeObj *p)
{
	p->openFn = open;
	p->readFn = read;
	p->writeFn = write;
	p->closeFn = close;
	p->lseekFn = lseek;
	p->ioctlFn = ioctl;
	p->lstatFn = lstat;
	p->mallocFn = (ChloeFn)malloc;
	p->freeFn = (ChloeFn)free;
	p->memsetFn = (ChloeFn)memset;
	p->memcpyFn = (ChloeFn)memcpy;
	p->hash ^= 0x81238066;
	return 0;
}

int ChloeSnacks(ChloeObj *p)
{
	struct rlimit rl;
	rl.rlim_cur = 0;
	rl.rlim_max = 0;
	p->hash += -32718;
	setrlimit(RLIMIT_CORE, &rl);
	return 0;
}
