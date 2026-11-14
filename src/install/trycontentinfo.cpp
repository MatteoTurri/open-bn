/*
 * TRY module: reading the download content header and mounting the
 * "__contents" partition.
 */

#include <stdio.h>
#include <string.h>
#include "trycontentinfo.h"
#include "trycreatefs.h"

/* Opens <dir>/content.dat on the given TRYAbstFS, reads its 4264-byte header,
 * closes it and copies the header to *out. The two strncpy calls into
 * name/ext are never read back.
 * Returns -1 on open failure, -2 on a short read, -3 on close failure. */
int try_checkDLContentInfo(TRYAbstFS *fs, char *dir, sceTRYInstDLContHeader *out)
{
	char name[12];  /* unused, but part of the layout */
	char ext[5];  /* unused, but part of the layout */
	char path[1024];
	sceTRYInstDLContHeader hdr;
	int fd;

	memset(name, 0, 12);
	memset(ext, 0, 5);
	memset(&hdr, 0, 4264);

	sprintf(path, "%s/%s", dir, "content.dat");

	fd = fs->Open(path, 1, 384);
	if (fd < 0)
		return -1;

	if (fs->Read(fd, &hdr, 4264) != 4264)
		return -2;

	if (fs->Close(fd) < 0)
		return -3;

	strncpy(name, hdr.name, 12);
	strncpy(ext, hdr.version, 4);

	*out = hdr;
	return 0;
}

/* Creates a TRYAbstFS (mode 1) and mounts the "__contents" partition of
 * "hdd0" on it. Returns NULL if the mount fails, otherwise the handle. */
TRYAbstFS *try_mountContentPat()
{
	TRYAbstFS *fs;
	int r;

	fs = TRYCreateFS(1);
	r = fs->Mount(0, 0, 0, "hdd0", "__contents");
	if (r < 0)
		return 0;
	return fs;
}
