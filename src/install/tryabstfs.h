/*
 * TRYAbstFS: the abstract file system interface of the title installer (TRY),
 * with the PFS stat and directory records it passes around.
 */

#ifndef TRYABSTFS_H
#define TRYABSTFS_H

/* Record of scepfsGetstat/scepfsChstat; mode & 0xF000: 0x1000 directory,
   0x2000 file. */
struct TRY_stat {
	/* 0x00 */ unsigned int mode;
	/* 0x04 */ unsigned int attr;
	/* 0x08 */ unsigned int size;
	/* 0x0C */ unsigned char ctime[8];
	/* 0x14 */ unsigned char atime[8];
	/* 0x1C */ unsigned char mtime[8];
	/* 0x24 */ unsigned int hisize;
	/* 0x28 */ unsigned int uid;
	/* 0x2C */ unsigned int gid;
	/* 0x30 */ unsigned int reserved[4];
}; /* size = 0x40 */

/* Directory entry of scepfsDread. In a listing of the partitions of a device,
   stat.mode is 0 for an empty partition and stat.size is in sectors. */
struct TRY_dirent {
	/* 0x00 */ TRY_stat stat;
	/* 0x40 */ char name[272];
}; /* size = 0x150 */

class TRYAbstFS {
public:
	virtual char *GetMountPoint() = 0;
	virtual int Mount(const char *dev, char *pass, char *opt, const char *blockdev, const char *part) = 0;
	virtual int UMount() = 0;
	virtual int MkMainPart(int unit, int size, char *name, char *name1, char *name2, char *name3) = 0;
	virtual int AddSubPart(int unit, int part, int size, char *name0, char *name1, char *name2, char *name3) = 0;
	virtual int Format(int unit, int part, char *unused, char *partName) = 0;
	virtual int RmPart(const char *unused, const char *partName, const char *password) = 0;
	virtual int Open(const char *path, int flags, unsigned short mode) = 0;
	virtual int Write(int fd, void *buf, int nbyte) = 0;
	virtual int Read(int fd, void *buf, int nbyte) = 0;
	virtual int Close(int fd) = 0;
	virtual int Seek(int fd, unsigned long long offset, int whence) = 0;
	virtual int Mkdir(const char *path, unsigned short mode) = 0;
	virtual int Rmdir(const char *path) = 0;
	virtual int RmdirALL(const char *path) = 0;
	virtual int Remove(const char *path) = 0;
	virtual int Rename(const char *oldname, const char *newname) = 0;
	virtual int Chdir(const char *path) = 0;
	virtual int Getstat(const char *path, TRY_stat *stat) = 0;
	virtual int Chstat(const char *path, TRY_stat *stat, unsigned int statmask) = 0;
	/* The destructor is the last slot of the vtable. */
	virtual ~TRYAbstFS() {}
}; /* size = 0x04 */

#endif
