/*
 * Chloe library: the device B constructor and small setters.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include "chloe.h"

int ChloeOrange(ChloeObj *p, const char *name)
{
	p->hash = (p->hash >> 1) + 0x12342;
	return 0;
}

int ChloeApple(ChloeObj *p)
{
	p->pageSize = getpagesize();
	return 0;
}

int ChloeChips(ChloeObj *p)
{
	struct rlimit rl;
	rl.rlim_cur = 0;
	rl.rlim_max = 0;
	setrlimit(RLIMIT_CORE, &rl);
	return 0;
}

/* Device B constructor: the same eleven libc functions as ChloeLemon in a
   different slot order (only openFn and writeFn coincide), so the field names
   of ChloeObj are positional, not semantic. Mixes the hash. */
int ChloeYogurt(ChloeObj *p)
{
	p->openFn = open;
	p->readFn = (ChloeFn)lstat;
	p->writeFn = write;
	p->closeFn = (ChloeFn)read;
	p->ioctlFn = (ChloeFn)memcpy;
	p->lseekFn = (ChloeFn)free;
	p->lstatFn = (ChloeFn)memset;
	p->mallocFn = (ChloeFn)close;
	p->freeFn = (ChloeFn)malloc;
	p->memsetFn = (ChloeFn)lseek;
	p->memcpyFn = (ChloeFn)ioctl;
	p->hash ^= 0x90212312;
	return 0;
}

int ChloeWatermelon(ChloeObj *p)
{
	tmpnam("/tmp/sce");
	return 0;
}
