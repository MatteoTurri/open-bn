/*
 * TRYPFSProxy: TRYAbstFS on the PFS driver. Mount points are "pfs10:" to
 * "pfs19:"; file and directory paths are relative to the mount point.
 */

#pragma implementation "trypfsproxy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trypfsproxy.h"
#include "trycreatefs.h"
#include "lib/scepfsd.h"

char TRYPFSProxy::MountFlag[10] = { 0 };

TRYPFSProxy::TRYPFSProxy()
{
	openFileCount = 0;
	memset(mountPoint, 0, 16);
	mounted = 0;
	memset(pathBuffer, 0, 1024);
	pathLen = 0;
}

TRYPFSProxy::~TRYPFSProxy()
{}

char *TRYPFSProxy::GetMountPoint()
{
	return mountPoint;
}

/* Mount point "pfsNN:" from a free id (or the name dev), device
   "blockdev:part,..." with the password or the option, then scepfsMount; the
   id stays marked in MountFlag. */
int TRYPFSProxy::Mount(const char *dev, char *pass, char *opt, const char *blockdev, const char *part)
{
	char buf[128];

	if (mounted != 0)
		return 0;
	int id = getAvailMountPointID();
	if (dev == 0)
		sprintf(mountPoint, "%s%d:", "pfs", id);
	else
		strcpy(mountPoint, dev);
	if (pass == 0 && opt == 0)
		sprintf(buf, "%s:%s,,", blockdev, part);
	else if (pass != 0)
		sprintf(buf, "%s:%s,%s", blockdev, part, pass);
	else
		sprintf(buf, "%s:%s,,%s,", blockdev, part, opt);
	int r = scepfsMount(mountPoint, buf, 0, 0, 0);
	if (r < 0)
		return r - 27400;
	MountFlag[id - 10] = 1;
	return 0;
}

int TRYPFSProxy::UMount()
{
	releaseMountPointID();
	if (openFileCount != 0)
		return -27001;
	int result = scepfsUmount(mountPoint);
	if (result != 0)
		return result - 27500;
	mounted = 0;
	return 0;
}

int TRYPFSProxy::getAvailMountPointID()
{
	for (int id = 0; id < 10; id++) {
		if (!MountFlag[id])
			return id + 10;
	}
	return -1;
}

/* Frees the MountFlag slot of mount point "pfsNN:". */
void TRYPFSProxy::releaseMountPointID()
{
	int id = atoi(mountPoint + 3);
	MountFlag[id - 10] = 0;
}
int TRYPFSProxy::MkMainPart(int unit, int size, char *name, char *name1, char *name2, char *name3)
{
	char buf[128];
	char hdr[512];
	sprintf(buf, "hdd0:%s,%s,%s,%s,PFS", name3, name, name1, try_getStrPatSize(unit));
	int ret = scepfsOpen(buf, 3);
	int fd;
	if (ret == -2) {
		partExists = 0;
	} else {
		partExists = 1;
		if (ret >= 0)
			scepfsClose(ret);
	}
	if (partExists)
		goto done;
	ret = scepfsOpen(buf, 0x203);
	if (ret < 0) {
		switch (-ret) {
		case 28:
			ret = -27003;
			break;
		case 2:
			ret = ret - 27100;
			break;
		case 3:
			ret = ret - 27100;
			break;
		case 4:
			ret = ret - 27100;
			break;
		case 5:
			ret = ret - 27100;
			break;
		case 6:
			ret = ret - 27100;
			break;
		case 7:
			ret = ret - 27100;
			break;
		case 8:
			ret = ret - 27100;
			break;
		case 9:
			ret = ret - 27100;
			break;
		case 10:
			ret = ret - 27100;
			break;
		case 11:
			ret = ret - 27100;
			break;
		case 12:
			ret = ret - 27100;
			break;
		case 13:
			ret = ret - 27100;
			break;
		case 14:
			ret = ret - 27100;
			break;
		case 15:
			ret = ret - 27100;
			break;
		case 16:
			ret = ret - 27100;
			break;
		case 17:
			ret = ret - 27100;
			break;
		case 18:
			ret = ret - 27100;
			break;
		case 19:
			ret = ret - 27100;
			break;
		case 20:
			ret = ret - 27100;
			break;
		case 21:
			ret = ret - 27100;
			break;
		case 22:
			ret = ret - 27100;
			break;
		case 23:
			ret = ret - 27100;
			break;
		case 24:
			ret = ret - 27100;
			break;
		case 25:
			ret = ret - 27100;
			break;
		case 26:
			ret = ret - 27100;
			break;
		case 27:
			ret = ret - 27100;
			break;
		default:
			ret = ret - 27100;
			break;
		}
	} else {
		fd = ret;
		memset(hdr, 0, 4);
		ret = scepfsWrite(fd, hdr, 512);
		if (ret != 512) {
			scepfsClose(fd);
			ret = -27626;
		} else {
			scepfsClose(fd);
done:
			return partExists != 0;
		}
	}
	return ret;
}

int TRYPFSProxy::AddSubPart(int unit, int part, int size, char *name0, char *name1, char *name2, char *name3)
{
	char buf[128];
	char *pat = try_getStrPatSize(unit);
	sprintf(buf, "hdd0:%s,%s", name3, name0);
	int fd = scepfsOpen(buf, 3);
	if (fd < 0)
		return -27004;
	if (!partExists) {
		for (int i = 0; i < part; i++) {
			int r = scepfsIoctl2(fd, 0x6801, pat, strlen(pat) + 1, 0, 0);
			if (r < 0) {
				switch (-r) {
				case 12:
					return r - 27200;
				case 27:
					return -27002;
				case 28:
					return -27003;
				case 2:
				default:
					return r - 27200;
				}
			}
		}
	}
	partExists = 1;
	scepfsClose(fd);
	return 0;
}

int TRYPFSProxy::Format(int unit, int part, char *unused, char *partName)
{
	char buf[128];
	sprintf(buf, "hdd0:%s", partName);
	int flag = 0x800 << (unit - 1);
	int r = scepfsFormat("pfs:", buf, &flag, 4);
	if (r >= 0)
		return 0;
	return r - 27300;
}

int TRYPFSProxy::RmPart(const char *unused, const char *partName, const char *password)
{
	char buf[1024];
	sprintf(buf, "hdd0:%s,%s", partName, password);
	int r = scepfsRemove(buf);
	if (r < 0)
		return -1;
	return 0;
}

int TRYPFSProxy::Open(const char *path, int flags, unsigned short mode)
{
	char buf[1024];
	sprintf(buf, "%s/%s", mountPoint, path);
	int r = scepfsOpen(buf, flags, mode);
	if (r >= 0)
		openFileCount++;
	return r;
}

int TRYPFSProxy::Write(int fd, void *buf, int nbyte)
{
	return scepfsWrite(fd, buf, nbyte);
}

int TRYPFSProxy::Read(int fd, void *buf, int nbyte)
{
	return scepfsRead(fd, buf, nbyte);
}

int TRYPFSProxy::Close(int fd)
{
	int r = scepfsClose(fd);
	if (r == 0)
		openFileCount--;
	return r;
}

int TRYPFSProxy::Seek(int fd, unsigned long long offset, int whence)
{
	return scepfsLseek64(fd, offset, whence);
}

int TRYPFSProxy::Mkdir(const char *path, unsigned short mode)
{
	char buf[1024];
	sprintf(buf, "%s/%s", mountPoint, path);
	return scepfsMkdir(buf, mode);
}

int TRYPFSProxy::Rmdir(const char *path)
{
	char buf[1024];
	sprintf(buf, "%s/%s", mountPoint, path);
	return scepfsRmdir(buf);
}

/* "." empties the current directory, recursing on each entry; otherwise a
   directory is entered, emptied, left and removed, and a file is removed. */
int TRYPFSProxy::RmdirALL(const char *path)
{
	TRY_stat st;
	TRY_dirent ent;
	char buf[16 + 1024];
	int fd, r;

	if (strcmp(".", path) == 0) {
		sprintf(buf, "%s%s", mountPoint, path);
		fd = scepfsDopen(buf);
		if (fd < 0)
			goto fail;
		while ((r = scepfsDread(fd, &ent)) != 0 && r > 0) {
			if (strcmp(".", ent.name) == 0)
				continue;
			if (strcmp("..", ent.name) == 0)
				continue;
			if (RmdirALL(ent.name) < 0)
				goto close_fail;
		}
		scepfsDclose(fd);
	} else {
		sprintf(buf, "%s%s", mountPoint, path);
		if (scepfsGetstat(buf, &st) < 0)
			goto fail;
		if ((st.mode & 0xf000) == 0x1000) {
			push_path((char *)path);
			if (Chdir(path) < 0)
				goto fail;
			if (RmdirALL(".") < 0)
				goto fail;
			if (Chdir("..") < 0)
				goto fail;
			Rmdir(pathBuffer);
			pop_path(strlen(path));
		}
		if ((st.mode & 0xf000) == 0x2000) {
			sprintf(buf, "%s%s", pathBuffer, path);
			Remove(buf);
		}
	}
	return 0;
close_fail:
	scepfsDclose(fd);
fail:
	return -1;
}

int TRYPFSProxy::Remove(const char *path)
{
	char buf[1024];
	sprintf(buf, "%s/%s", mountPoint, path);
	return scepfsRemove(buf);
}

int TRYPFSProxy::Rename(const char *oldname, const char *newname)
{
	char oldPath[1024];
	char newPath[1024];
	sprintf(oldPath, "%s/%s", mountPoint, oldname);
	sprintf(newPath, "%s/%s", mountPoint, newname);
	return scepfsRename(oldPath, newPath);
}

int TRYPFSProxy::Chdir(const char *path)
{
	char buf[1024];
	sprintf(buf, "%s%s", mountPoint, path);
	return scepfsChdir(buf);
}

int TRYPFSProxy::Getstat(const char *path, TRY_stat *stat)
{
	char buf[1024];
	sprintf(buf, "%s%s", mountPoint, path);
	return scepfsGetstat(buf, stat);
}

int TRYPFSProxy::Chstat(const char *path, TRY_stat *stat, unsigned int statmask)
{
	char buf[1024];
	sprintf(buf, "%s%s", mountPoint, path);
	return scepfsChstat(buf, stat, statmask);
}

/* Appends "path/" to pathBuffer if it fits (1024 bytes with the NUL). */
void TRYPFSProxy::push_path(char *path)
{
	unsigned int len = strlen(path);
	if (pathLen + len + 1 < 1025) {
		pathLen += 1 + strlen(path);
		strcat(pathBuffer, path);
		strcat(pathBuffer, "/");
	}
}

void TRYPFSProxy::pop_path(int len)
{
	pathLen--;
	pathLen -= len;
	pathBuffer[pathLen] = '\0';
}
