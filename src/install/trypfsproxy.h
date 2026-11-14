/*
 * TRYPFSProxy: the TRYAbstFS file system interface of the title installer
 * (TRY), implemented on the PFS driver (scepfs*).
 */

#ifndef TRYPFSPROXY_H
#define TRYPFSPROXY_H

#include "tryabstfs.h"

class TRYPFSProxy : public TRYAbstFS {
public:
	/* 0x004 */ int openFileCount;     // files open: Open++, Close--, checked by UMount
	/* 0x008 */ char mountPoint[16];
	/* 0x018 */ int mounted;           // tested by Mount, cleared by UMount; never set
	/* 0x01C */ char pathBuffer[1024];  // nested directory path, see push_path/pop_path
	/* 0x41C */ int pathLen;
	/* 0x420 */ int partExists;        // MkMainPart/AddSubPart: the partition already existed

	TRYPFSProxy();
	virtual ~TRYPFSProxy();

	virtual char *GetMountPoint();

	virtual int Mount(const char *dev, char *pass, char *opt, const char *blockdev, const char *part);
	virtual int UMount();

	int getAvailMountPointID();
	void releaseMountPointID();

	virtual int MkMainPart(int unit, int size, char *name, char *name1, char *name2, char *name3);
	virtual int AddSubPart(int unit, int part, int size, char *name0, char *name1, char *name2, char *name3);
	virtual int Format(int unit, int part, char *unused, char *partName);
	virtual int RmPart(const char *unused, const char *partName, const char *password);

	virtual int Open(const char *path, int flags, unsigned short mode);
	virtual int Write(int fd, void *buf, int nbyte);
	virtual int Read(int fd, void *buf, int nbyte);
	virtual int Close(int fd);
	virtual int Seek(int fd, unsigned long long offset, int whence);

	virtual int Mkdir(const char *path, unsigned short mode);
	virtual int Rmdir(const char *path);
	virtual int RmdirALL(const char *path);
	virtual int Remove(const char *path);
	virtual int Rename(const char *oldname, const char *newname);
	virtual int Chdir(const char *path);
	virtual int Getstat(const char *path, TRY_stat *stat);
	virtual int Chstat(const char *path, TRY_stat *stat, unsigned int statmask);

	void push_path(char *path);
	void pop_path(int len);

	// in-use flags of mount point IDs 10..19
	static char MountFlag[10];
}; /* size = 0x424 */

#endif
