/*
 * The PFS client (scepfs*, scepfsd.c) and its types.
 */

#ifndef SCEPFSD_H
#define SCEPFSD_H

/* IOP iox_stat_t: the record scepfsGetstat fills and scepfsChstat applies.
   The dates have the layout of sceMcStDateTime. */
typedef struct {
	/* 0x00 */ unsigned int mode;
	/* 0x04 */ unsigned int attr;
	/* 0x08 */ unsigned int size;
	/* 0x0C */ unsigned char ctime[8];
	/* 0x14 */ unsigned char atime[8];
	/* 0x1C */ unsigned char mtime[8];
	/* 0x24 */ unsigned int hisize;
	/* 0x28 */ unsigned int privateData[6];
} iox_stat_t; /* size = 0x40 */

/* IOP iox_dirent_t: the record scepfsDread fills. */
typedef struct {
	/* 0x000 */ iox_stat_t stat;
	/* 0x040 */ char name[256];
	/* 0x140 */ void *privateData;
} iox_dirent_t; /* size = 0x144 */

/* One file of a PS2ICON3D header. */
typedef struct {
	/* 0x00 */ int offset; /* from the start of the header */
	/* 0x04 */ int size;
} PS2Icon3DFile; /* size = 0x8 */

/* Header of the attribute area of a PS2 game partition (the first 512 bytes
   of its root, read through PFS). */
typedef struct {
	/* 0x00 */ char magic[16]; /* "PS2ICON3D" */
	/* 0x10 */ PS2Icon3DFile systemCnf;
	/* 0x18 */ PS2Icon3DFile iconSys;
	/* 0x20 */ PS2Icon3DFile listIco;
	/* 0x28 */ PS2Icon3DFile delIco;
	/* 0x30 */ char pad_30[0x1D0];
} PS2Icon3DHeader; /* size = 0x200 */

#ifdef __cplusplus
extern "C" {
#endif

/* The daemon client; the scepfs* names are weak aliases of these. */
int scepfsdInit(void);
void scepfsdExit(void);
int scepfsdOpen(const char *path, int flags, ...);
long scepfsdLseek(int fd, long offset, int whence);
long long scepfsdLseek64(int fd, long long offset, int whence);
int scepfsdRead(int fd, void *buf, unsigned int size);
int scepfsdWrite(int fd, const void *buf, unsigned int size);
int scepfsdClose(int fd);
int scepfsdDopen(const char *path);
int scepfsdDread(int fd, void *dirent);
int scepfsdDclose(int fd);
int scepfsdChstat(const char *path, void *stat, unsigned int statmask);
int scepfsdGetstat(const char *path, void *stat);
int scepfsdMkdir(const char *path, int mode);
int scepfsdRmdir(const char *path);
int scepfsdChdir(const char *path);
int scepfsdRemove(const char *path);
int scepfsdRename(const char *oldpath, const char *newpath);
int scepfsdIoctl(int fd, int cmd, int arg);
int scepfsdIoctl2(int fd, int subcmd, void *arg, int arglen, void *buf, int buflen);
int scepfsdSymlink(const char *target, const char *linkpath);
int scepfsdReadlink(const char *path, char *buf, int bufsize);
int scepfsdSync(const char *path, int flag);
int scepfsdFormat(const char *dev, const char *blockdev, void *arg, unsigned int arglen);
int scepfsdMount(const char *dev, const char *mp, int flags, void *data, unsigned int datalen);
int scepfsdUmount(const char *mp);
int scepfsdDevctl(const char *name, int cmd, void *args, unsigned int arglen, void *buf, unsigned int buflen);
int scepfsdSetReadAhead(int val);
int scepfsdGetReadAhead(void);
int scepfsdGetMountPoint(const char *name, char *buf);

/* The PFS interface: weak aliases of the scepfsd* functions. */
int scepfsInit(void);
void scepfsExit(void);
int scepfsOpen(const char *path, int flags, ...);
long scepfsLseek(int fd, long offset, int whence);
long long scepfsLseek64(int fd, long long offset, int whence);
int scepfsRead(int fd, void *buf, unsigned int size);
int scepfsWrite(int fd, const void *buf, unsigned int size);
int scepfsClose(int fd);
int scepfsDopen(const char *path);
int scepfsDread(int fd, void *dirent);
int scepfsDclose(int fd);
int scepfsChstat(const char *path, void *stat, unsigned int statmask);
int scepfsGetstat(const char *path, void *stat);
int scepfsMkdir(const char *path, int mode);
int scepfsRmdir(const char *path);
int scepfsChdir(const char *path);
int scepfsRemove(const char *path);
int scepfsRename(const char *oldpath, const char *newpath);
int scepfsIoctl(int fd, int cmd, int arg);
int scepfsIoctl2(int fd, int subcmd, void *arg, int arglen, void *buf, int buflen);
int scepfsSymlink(const char *target, const char *linkpath);
int scepfsReadlink(const char *path, char *buf, int bufsize);
int scepfsSync(const char *path, int flag);
int scepfsFormat(const char *dev, const char *blockdev, void *arg, unsigned int arglen);
int scepfsMount(const char *dev, const char *mp, int flags, void *data, unsigned int datalen);
int scepfsUmount(const char *mp);
int scepfsDevctl(const char *name, int cmd, void *args, unsigned int arglen, void *buf, unsigned int buflen);
int scepfsSetReadAhead(int val);
int scepfsGetReadAhead(void);
int scepfsGetMountPoint(const char *name, char *buf);

#ifdef __cplusplus
}
#endif

#endif
