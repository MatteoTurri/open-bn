/*
 * Install history of the TRY installer: a file with a package count followed
 * by 68-byte {name, version} records, checked and updated on each install;
 * plus the PFS partition counters of the size check.
 */

#include "tryinsthistory.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lib/scepfsd.h"

/* Parses the two package versions of a title sub-ID "pkg1:pkg2"; on failure
   both are zeroed and -27901 is returned. */
int try_parseTitleSubID(char *titleId, TRYInstPkg *pkg1, TRYInstPkg *pkg2)
{
	char buf[1024];
	char *p;
	char *colon;
	int r;

	strcpy(buf, titleId);
	p = buf;
	colon = strchr(p, ':');
	if (colon) {
		*colon++ = 0;
		r = parseInstVer(p, pkg1);
		if (r >= 0) {
			r = parseInstVer(colon, pkg2);
			if (r >= 0)
				return 0;
		}
	}
	memset(pkg1, 0, 72);
	memset(pkg2, 0, 72);
	return -27901;
}

/* Parses "?N[+];name": ver = N (the first character is skipped), isPlus if a
   '+' is present, name after the ';' (empty without one). */
int parseInstVer(char *s, TRYInstPkg *pkg)
{
	char *p = strchr(s, ';');
	if (p == 0)
		pkg->name[0] = 0;
	else
		strcpy(pkg->name, p + 1);
	if (strrchr(s, '+'))
		pkg->isPlus = 1;
	else
		pkg->isPlus = 0;
	pkg->ver = atoi(s + 1);
	return 0;
}

/* Rewrites the history with the record of pkg updated or appended, through
   "tmp_verfile" and a rename; a missing history is created empty. Returns -1
   on any error. */
int try_writeInstHistory(TRYAbstFS *fs, char *path, TRYInstPkg *pkg)
{
	TRYInstPackage entry;
	int fdSrc;
	int fdTmp;
	int num;
	int missing;
	int r;
	int i;
	char *tmpName;

	if (pkg->ver == 0)
		return 0;

	fdSrc = -1;
	fdTmp = -1;
	missing = 1;
	tmpName = "tmp_verfile";
	r = readNumberOfPackage(fs, path, &num);
	if (r < 0) {
		if (r != -2)
			goto fail;
		num = 0;
		r = fs->Open(path, 1538, 438);
		if (r < 0)
			goto fail;
		fdSrc = r;
		r = fs->Write(fdSrc, &num, 4);
		if (r != 4)
			goto fail;
		r = fdSrc;
	}
	fdSrc = r;

	r = openTmpVerFile(fs, tmpName);
	if (r < 0)
		goto fail;
	fdTmp = r;

	for (i = 0; i < num; i++) {
		r = readPackageEntry(fs, fdSrc, &entry);
		if (r < 0)
			goto fail;
		if (strcmp(entry.name, pkg->name) == 0) {
			missing = 0;
			entry.ver = pkg->ver;
		}
		r = writePackageEntry(fs, fdTmp, &entry);
		if (r < 0)
			goto fail;
	}

	if (missing == 1) {
		memcpy(entry.name, pkg->name, 64);
		entry.ver = pkg->ver;
		r = writePackageEntry(fs, fdTmp, &entry);
		if (r < 0)
			goto fail;
		num = num + 1;
	}

	r = writeNumberOfPackage(fs, fdTmp, num);
	if (r < 0)
		goto fail;

	fs->Close(fdSrc);
	fdSrc = -1;
	fs->Close(fdTmp);
	fdTmp = -1;
	if (fs->Rename(tmpName, path) >= 0)
		return 0;
	fs->Remove(tmpName);
fail:
	if (fdSrc >= 0)
		fs->Close(fdSrc);
	if (fdTmp >= 0)
		fs->Close(fdTmp);
	return -1;
}

/* Compares pkg with its record in the history: 0 if absent or the recorded
   version is older, 2 if newer, (pkg->isPlus != 1) if equal; 1 when pkg has no
   version, -27902 if the history cannot be opened. */
int try_checkInstHistory(TRYAbstFS *fs, char *path, TRYInstPkg *pkg)
{
	TRYInstPackage entry;
	int fd = -1;
	int num = -1;
	int ret;
	int notfound = 1;
	int i;

	if (pkg->ver == 0)
		return 1;

	ret = readNumberOfPackage(fs, path, &num);
	if (ret < 0) {
		if (ret != -2) {
			ret = -27902;
			goto out;
		}
		ret = 0;
		goto out;
	}
	fd = ret;
	if (num == 0) {  /* NOTE: bug: fd is closed here and again at out */
		fs->Close(ret);
		fs->Remove(path);
		ret = 0;
		goto out;
	}
	for (i = 0; i < num; i++) {
		ret = readPackageEntry(fs, fd, &entry);
		if (ret < 0)
			goto out;
		if (strcmp(entry.name, pkg->name) == 0) {
			if (entry.ver < pkg->ver)
				ret = 0;
			if (entry.ver == pkg->ver)
				ret = pkg->isPlus != 1;
			if (entry.ver > pkg->ver)
				ret = 2;
			notfound = 0;
			break;
		}
	}
	if (notfound)
		ret = 0;
out:
	if (fd >= 0)
		fs->Close(fd);
	return ret;
}

/* Opens the history and reads its count; returns the fd, -2 if the file does
   not exist, -1 on any other error. */
int readNumberOfPackage(TRYAbstFS *fs, char *path, int *num)
{
	int fd = fs->Open(path, 1, 438);

	if (fd < 0) {
		if (fd != -2)
			goto err;
		return fd;
	}
	if (fs->Read(fd, num, 4) == 4)
		return fd;
	*num = -1;
err:
	return -1;
}

/* Rewinds and rewrites the count at the head of the file. */
int writeNumberOfPackage(TRYAbstFS *fs, int fd, int num)
{
	if (fs->Seek(fd, 0, 0) >= 0) {
		if (fs->Write(fd, &num, 4) == 4)
			return 0;
	}
	return -1;
}

/* Reads one record; 0 if complete, -1 otherwise. */
int readPackageEntry(TRYAbstFS *fs, int fd, TRYInstPackage *pkg)
{
	return fs->Read(fd, pkg, 68) == 68 ? 0 : -1;
}

/* Writes one record; 0 if complete, -1 otherwise. */
int writePackageEntry(TRYAbstFS *fs, int fd, TRYInstPackage *pkg)
{
	return fs->Write(fd, pkg, 68) == 68 ? 0 : -1;
}

/* Creates or truncates the file (O_WRONLY | O_CREAT | O_TRUNC, 0666) and writes
   a zero count; returns the open fd or -1. */
int openTmpVerFile(TRYAbstFS *fs, char *path)
{
	int fd = fs->Open(path, 1538, 438);

	if (fd >= 0) {
		int zero = 0;
		if (fs->Write(fd, &zero, 4) == 4)
			return fd;
	}
	return -1;
}

int CountNumberOfEmptyPartition(char *path, unsigned int size)
/* Number of empty partitions of the given size under path; -1 on error. */
{
	TRY_dirent ent;
	int fd = scepfsDopen(path);
	int count = 0;

	if (fd < 0)
		goto err;
	while (scepfsDread(fd, &ent) > 0) {
		if (ent.stat.mode == 0 && ent.stat.size == size)
			count++;
	}
	scepfsDclose(fd);
	return count;
err:
	return -1;
}

/* Free space in 128 MB units (0x40000 sectors), summed over the empty
   partitions of each size from 0x4000000 sectors down to 0x40000. */
int try_GetEmptyPartitionCount(char *devPath)
{
	int count = 0;
	unsigned int size = 0x04000000;
	char buf[16];  /* unused, but part of the layout */

	for (;;) {
		int shift, scaled, r;

		if (0x1FFFFF < size) {
			shift = size >> 21;
			sprintf(buf, "%4dGB", shift);
			scaled = shift << 3;
		} else {
			shift = size >> 11;
			sprintf(buf, "%4dMB", shift);
			scaled = size >> 18;
		}

		r = CountNumberOfEmptyPartition(devPath, size);
		if (r < 0)
			return -1;

		size = size >> 1;
		count = count + r * scaled;

		if (!(0x3FFFF < size))
			return count;
	}
}

/* Sum of the sizes (in sectors) of the partitions listed under path; -1 if it
   cannot be opened. */
int GetTotalUsedSectorCount(char *path)
{
	int fd;
	int total;
	TRY_dirent ent;

	fd = scepfsDopen(path);
	if (fd < 0)
		goto err;
	total = 0;
	while (scepfsDread(fd, &ent) > 0)
		total += ent.stat.size;
	scepfsDclose(fd);
	return total;
err:
	return -1;
}

/* Number of aligned 0x40000-sector partitions that still fit between the used
   sectors and the device size (devctl 18434). */
int try_GetUnusedPartitionCount(char *devPath)
{
	int count = 0;
	int size = scepfsDevctl(devPath, 18434, 0, 0, 0, 0);
	int used = GetTotalUsedSectorCount(devPath);
	if (used < 0)
		goto err;
	do {
		if (used & 0x3FFFF)
			break;
		used += 0x40000;
		if (used >= size)
			break;
		count++;
	} while (1);
	return count;
err:
	return -1;
}
