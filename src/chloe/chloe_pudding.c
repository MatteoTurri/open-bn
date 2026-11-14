/*
 * Chloe library: MIPS_TASKFLAGS set and a block loader.
 */

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "chloe.h"

/* Sets MIPS_TASKFLAGS to the value saved in sysmipsArg by ChloeLime. */
int ChloePudding(ChloeObj *p)
{
	sysmips(2061, 1, p->sysmipsArg);
	return 0;
}

/* Loads a block from a file with a 512-byte header: the words at 308 and 304
   are the block count and the first block, in 512-byte units. Returns the
   malloc'ed data (size in *out_size if given), or 0 on any failure. */
void *ChloeBanana(const char *path, int *out_size)
{
	int fd;
	char hdr[512];
	int count, startblk, size;
	void *data;

	if (out_size != 0)
		*out_size = 0;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return 0;

	if (read(fd, hdr, 512) != 512) {
		close(fd);
		return 0;
	}

	count = *(int *)(hdr + 308);
	startblk = *(int *)(hdr + 304);
	size = count << 9;

	data = malloc(size);
	if (data == 0) {
		close(fd);
		return 0;
	}

	if (lseek(fd, startblk << 9, SEEK_SET) < 0) {
		close(fd);
		return 0;
	}

	if (read(fd, data, size) < 0) {
		close(fd);
		return 0;
	}

	close(fd);
	if (out_size != 0)
		*out_size = size;
	return data;
}
