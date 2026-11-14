/*
 * scepfsd*: client of the PFS (PlayStation File System) daemon pfsd. Each call
 * takes a SysV semaphore lock, sends a command over an AF_UNIX socket, reads
 * the reply and moves bulk data through a SysV shared memory buffer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include "scepfsd.h"

static int fsd_lock(void);
static int fsd_unlock(void);
static int fsd_sendcmd(int cmd, void *arg, int argsize);
static int fsd_reply(void);
static long fsd_reply_seek(void);
static long long fsd_reply_seek64(void);

static int fsd_init_conn(void);
static int fsd_exit_conn(void);

/* Connection state shared by all calls. */
static int g_fsdSemId;      /* SysV semaphore used as the lock */
static int g_fsdShmId;      /* shared memory segment id */
static void *g_fsdShmBuf;   /* shared buffer (shmat) */
static int g_fsdSock;       /* socket to the daemon */
static int g_fsdMaxChunk;   /* largest block moved through the shared buffer */

/* Message header sent to the daemon: only cmd is written, the first word
 * keeps whatever malloc left there. */
struct fsd_msg {
	int pad0;
	int cmd;
};

/* 288-byte address buffer; only its 110-byte sockaddr_un part is cleared. */
struct fsd_sockaddr_un {
	short sun_family;
	char sun_path[286];
};

/* iox_stat_t and iox_dirent_t moved as raw bytes between the caller's buffer
 * and the shared buffer, neither of which is known to be aligned. */
typedef struct {
	unsigned char b[sizeof(iox_stat_t)];
} fsd_stat_bytes;
typedef struct {
	unsigned char b[sizeof(iox_dirent_t)];
} fsd_dirent_bytes;

/* Argument block of the extended Ioctl2 subcommand 0x6832. */
struct fsd_ioctl2_ext {
	int unk_00;
	int unk_04;
	int count;   /* buffer size in 512-byte units */
	int unk_0C;
	void *ptr;   /* in/out buffer, also receives the reply */
};

static int fsd_connect(void)
{
	struct fsd_sockaddr_un sa;

	g_fsdSock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (g_fsdSock < 0) {
		perror("socket");
		return -1;
	}
	memset(&sa, 0, 110);
	sa.sun_family = 1;
	if (bind(g_fsdSock, (struct sockaddr *)&sa, 2) < 0) {
		perror("bind");
		return -1;
	}
	memset(&sa, 0, 110);
	sa.sun_family = 1;
	memcpy(sa.sun_path, "/tmp/pfsd", 10);
	if (connect(g_fsdSock, (struct sockaddr *)&sa, 12) < 0) {
		perror("connect");
		return -1;
	}
	return 0;
}

static int fsd_shm_size(void)
{
	struct shmid_ds ds;

	shmctl(g_fsdShmId, 2, &ds);
	return ds.shm_segsz;
}

static int fsd_init_conn(void)
{
	if (fsd_connect() < 0) {
		printf("Can't connect to pfsd.\n");
		return -1;
	}
	g_fsdSemId = semget(0x7c724e1d, 1, 0);
	if (g_fsdSemId < 0) {
		perror("semget");
		return -2;
	}
	g_fsdShmId = shmget(0x7c724e1d, 0, 0);
	if (g_fsdShmId < 0) {
		perror("shmget");
		return -3;
	}
	g_fsdShmBuf = shmat(g_fsdShmId, 0, 0);
	/* NOTE: bug: shmat returns (void *) -1 on failure and a pointer is never < 0 */
	if (g_fsdShmBuf < 0) {
		perror("shmat");
		return -4;
	}
	g_fsdMaxChunk = fsd_shm_size();
	if (g_fsdMaxChunk <= 0) {
		fprintf(stderr, "GetShmSize error.\n");
		return -5;
	}
	return 0;
}

static int fsd_exit_conn(void)
{
	shmdt(g_fsdShmBuf);
	close(g_fsdSock);
}

static int fsd_lock(void)
{
	struct sembuf sb;

	sb.sem_num = 0;
	sb.sem_op = -1;
	sb.sem_flg = 4096;
	if (semop(g_fsdSemId, &sb, 1) < 0)
		perror("semop require lock");
}

static int fsd_unlock(void)
{
	struct sembuf sb;

	sb.sem_num = 0;
	sb.sem_op = 1;
	sb.sem_flg = 4096;
	if (semop(g_fsdSemId, &sb, 1) < 0)
		perror("semop release lock");
}

static int fsd_sendcmd(int cmd, void *arg, int argsize)
{
	struct fsd_msg *m;
	int len = argsize + 8;

	m = (struct fsd_msg *)malloc(len);
	m->cmd = cmd;
	memcpy((char *)m + 8, arg, argsize);
	if (write(g_fsdSock, m, len) < 0)
		perror("Send msg failed.\n");
	free(m);
}

static int fsd_reply(void)
{
	int r;

	if (read(g_fsdSock, &r, 4) <= 0)
		perror("Receive msg failed.\n");
	return r;
}

static long fsd_reply_seek(void)
{
	long r;

	if (read(g_fsdSock, &r, 4) <= 0)
		perror("Receive msg failed.\n");
	return r;
}

static long long fsd_reply_seek64(void)
{
	long long r;

	if (read(g_fsdSock, &r, 8) <= 0)
		perror("Receive msg failed.\n");
	return r;
}

int scepfsdInit(void)
{
	return fsd_init_conn();
}

void scepfsdExit(void)
{
	fsd_exit_conn();
}

int scepfsdOpen(const char *path, int flags, ...)
{
	va_list ap;
	int mode;
	struct {
		char path[1024];
		int flags;
		int mode;
	} req;
	int r;

	va_start(ap, flags);
	mode = va_arg(ap, int);
	va_end(ap);

	strcpy(req.path, path);
	req.flags = flags;
	req.mode = mode;
	fsd_lock();
	fsd_sendcmd(0, &req, sizeof(req));
	r = fsd_reply();
	fsd_unlock();
	return r;
}

long scepfsdLseek(int fd0, long offset0, int whence0)
{
	struct {
		int fd, offset, whence;
	} req;
	long r;
	req.fd = fd0;
	req.offset = offset0;
	req.whence = whence0;
	fsd_lock();
	fsd_sendcmd(1, &req, sizeof(req));
	r = fsd_reply_seek();
	fsd_unlock();
	return r;
}

long long scepfsdLseek64(int fd0, long long offset0, int whence0)
{
	struct {
		int fd;
		long long offset;
		int whence;
	} req;
	long long r;
	req.fd = fd0;
	req.offset = offset0;
	req.whence = whence0;
	fsd_lock();
	fsd_sendcmd(2, &req, sizeof(req));
	r = fsd_reply_seek64();
	fsd_unlock();
	return r;
}

int scepfsdRead(int fd, void *buf0, unsigned int size0)
{
	void *buf = buf0;
	unsigned int size = size0;
	int total = 0;
	int n;
	struct {
		int fd;
		void *buf;
		unsigned int size;
	} req;

	if (g_fsdMaxChunk < size) {
		do {
			n = scepfsdRead(fd, buf, g_fsdMaxChunk);
			if (n < 0)
				return n;
			buf = (char *)buf + n;
			size -= n;
			total += n;
			if (n < g_fsdMaxChunk)
				return total;
		} while (g_fsdMaxChunk < size);
	}

	req.fd = fd;
	req.buf = buf;
	req.size = size;
	fsd_lock();
	fsd_sendcmd(3, &req, sizeof(req));
	n = fsd_reply();
	if (buf)
		memcpy(buf, g_fsdShmBuf, size);
	fsd_unlock();
	return n < 0 ? n : total + n;
}

int scepfsdWrite(int fd, const void *buf0, unsigned int size0)
{
	const void *buf = buf0;
	unsigned int size = size0;
	int total = 0;
	int n;
	struct {
		int fd;
		const void *buf;
		unsigned int size;
	} req;

	if (g_fsdMaxChunk < size) {
		do {
			n = scepfsdWrite(fd, buf, g_fsdMaxChunk);
			if (n < 0)
				return n;
			buf = (const char *)buf + n;
			size -= n;
			total += n;
			if (n < g_fsdMaxChunk)
				return total;
		} while (g_fsdMaxChunk < size);
	}

	req.fd = fd;
	req.buf = buf;
	req.size = size;
	fsd_lock();
	if (buf)
		memcpy(g_fsdShmBuf, buf, size);
	fsd_sendcmd(4, &req, sizeof(req));
	n = fsd_reply();
	fsd_unlock();
	return n < 0 ? n : total + n;
}

int scepfsdClose(int fd0)
{
	int fd = fd0;
	int r;
	fsd_lock();
	fsd_sendcmd(5, &fd, sizeof(fd));
	r = fsd_reply();
	fsd_unlock();
	return r;
}

int scepfsdDopen(const char *path)
{
	char buf[1024];
	int r;
	strcpy(buf, path);
	fsd_lock();
	fsd_sendcmd(6, buf, sizeof(buf));
	r = fsd_reply();
	fsd_unlock();
	return r;
}

int scepfsdDread(int fd0, void *dirent)
{
	int fd = fd0;
	struct {
		int fd;
		void *dirent;
	} req;
	int r;
	req.fd = fd;
	req.dirent = dirent;
	fsd_lock();
	fsd_sendcmd(7, &req, sizeof(req));
	r = fsd_reply();
	*(fsd_dirent_bytes *)dirent = *(fsd_dirent_bytes *)g_fsdShmBuf;
	fsd_unlock();
	return r;
}

int scepfsdDclose(int fd0)
{
	int fd = fd0;
	int r;
	fsd_lock();
	fsd_sendcmd(8, &fd, sizeof(fd));
	r = fsd_reply();
	fsd_unlock();
	return r;
}

int scepfsdChstat(const char *path, void *stat0, unsigned int statmask0)
{
	void *stat = stat0;
	unsigned int statmask = statmask0;
	struct {
		char path[1024];
		void *stat;
		unsigned int statmask;
	} req;
	int r;
	strcpy(req.path, path);
	req.stat = stat;
	req.statmask = statmask;
	fsd_lock();
	if (stat)
		*(fsd_stat_bytes *)g_fsdShmBuf = *(fsd_stat_bytes *)stat;
	fsd_sendcmd(9, &req, sizeof(req));
	r = fsd_reply();
	fsd_unlock();
	return r;
}

int scepfsdGetstat(const char *path, void *stat0)
{
	void *stat = stat0;
	struct {
		char path[1024];
		void *stat;
	} req;
	int r;
	strcpy(req.path, path);
	req.stat = stat;
	fsd_lock();
	fsd_sendcmd(10, &req, sizeof(req));
	r = fsd_reply();
	if (stat)
		*(fsd_stat_bytes *)stat = *(fsd_stat_bytes *)g_fsdShmBuf;
	fsd_unlock();
	return r;
}

int scepfsdMkdir(const char *path, int mode0)
{
	int mode = mode0;
	struct {
		char path[1024];
		int mode;
	} req;
	int r;
	strcpy(req.path, path);
	req.mode = mode;
	fsd_lock();
	fsd_sendcmd(11, &req, sizeof(req));
	r = fsd_reply();
	fsd_unlock();
	return r;
}

int scepfsdRmdir(const char *path)
{
	char buf[1024];
	int r;
	strcpy(buf, path);
	fsd_lock();
	fsd_sendcmd(12, buf, sizeof(buf));
	r = fsd_reply();
	fsd_unlock();
	return r;
}

int scepfsdChdir(const char *path)
{
	char buf[1024];
	int r;
	strcpy(buf, path);
	fsd_lock();
	fsd_sendcmd(13, buf, sizeof(buf));
	r = fsd_reply();
	fsd_unlock();
	return r;
}

int scepfsdRemove(const char *path)
{
	char buf[1024];
	int r;
	strcpy(buf, path);
	fsd_lock();
	fsd_sendcmd(14, buf, sizeof(buf));
	r = fsd_reply();
	fsd_unlock();
	return r;
}

int scepfsdRename(const char *oldpath, const char *newpath)
{
	struct {
		char oldpath[1024];
		char newpath[1024];
	} req;
	int r;
	strcpy(req.oldpath, oldpath);
	strcpy(req.newpath, newpath);
	fsd_lock();
	fsd_sendcmd(15, &req, sizeof(req));
	r = fsd_reply();
	fsd_unlock();
	return r;
}

int scepfsdIoctl(int fd0, int cmd0, int arg0)
{
	struct {
		int fd, cmd, arg;
	} req;
	int r;
	req.fd = fd0;
	req.cmd = cmd0;
	req.arg = arg0;
	fsd_lock();
	fsd_sendcmd(16, &req, sizeof(req));
	r = fsd_reply();
	fsd_unlock();
	return r;
}

int scepfsdIoctl2(int fd, int subcmd, void *arg0, int arglen0, void *buf, int buflen)
{
	int arglen = arglen0;
	void *arg = arg0;
	int r;

	if (subcmd != 0x6832) {
		struct {
			int fd, subcmd;
			void *arg;
			int arglen;
			void *buf;
			int buflen;
		} req;
		req.fd = fd;
		req.subcmd = subcmd;
		req.arg = arg;
		req.arglen = arglen;
		req.buf = buf;
		req.buflen = buflen;
		fsd_lock();
		if (arg && arglen)
			memcpy(g_fsdShmBuf, arg, arglen);
		fsd_sendcmd(17, &req, sizeof(req));
		r = fsd_reply();
		if (buf && buflen)
			memcpy(buf, g_fsdShmBuf, buflen);
		fsd_unlock();
		return r;
	} else {
		struct fsd_ioctl2_ext *p = (struct fsd_ioctl2_ext *)arg;
		int count = p->count;
		void *ptr = p->ptr;
		int size = count << 9;
		struct {
			int fd;
			struct fsd_ioctl2_ext ext;
		} req2;
		int t;

		if (ptr == 0 || 0x20000 < size)
			return -22;

		req2.fd = fd;
		req2.ext.unk_00 = p->unk_00;
		req2.ext.unk_04 = p->unk_04;
		req2.ext.count = count;
		req2.ext.unk_0C = p->unk_0C;
		req2.ext.ptr = ptr;

		fsd_lock();
		memcpy(g_fsdShmBuf, ptr, size);
		fsd_sendcmd(28, &req2, sizeof(req2));
		t = fsd_reply();
		memcpy(ptr, g_fsdShmBuf, size);
		r = t;
		fsd_unlock();
		return r;
	}
}

int scepfsdSymlink(const char *target, const char *linkpath)
{
	struct {
		char target[1024];
		char linkpath[1024];
	} req;
	int r;
	strcpy(req.target, target);
	strcpy(req.linkpath, linkpath);
	fsd_lock();
	fsd_sendcmd(18, &req, sizeof(req));
	r = fsd_reply();
	fsd_unlock();
	return r;
}

int scepfsdReadlink(const char *path, char *buf0, int bufsize0)
{
	char *buf = buf0;
	int bufsize = bufsize0;
	struct {
		char path[1024];
		char *unused;
		int bufsize;
	} req;
	int r;
	strcpy(req.path, path);
	strcpy(req.unused, buf);
	req.bufsize = bufsize;
	fsd_lock();
	fsd_sendcmd(19, &req, sizeof(req));
	r = fsd_reply();
	if (buf) {
		if (bufsize)
			memcpy(buf, g_fsdShmBuf, bufsize);
	}
	fsd_unlock();
	return r;
}

int scepfsdSync(const char *path, int flag0)
{
	int flag = flag0;
	struct {
		char path[1024];
		int flag;
	} req;
	int r;
	strcpy(req.path, path);
	req.flag = flag;
	fsd_lock();
	fsd_sendcmd(20, &req, sizeof(req));
	r = fsd_reply();
	fsd_unlock();
	return r;
}

int scepfsdFormat(const char *dev, const char *blockdev, void *arg0, unsigned int arglen0)
{
	void *arg = arg0;
	unsigned int arglen = arglen0;
	struct {
		char dev[1024];
		char blockdev[1024];
		void *arg;
		unsigned int arglen;
	} req;
	int r;
	strcpy(req.dev, dev);
	strcpy(req.blockdev, blockdev);
	req.arg = arg;
	req.arglen = arglen;
	fsd_lock();
	if (arg && arglen)
		memcpy(g_fsdShmBuf, arg, arglen);
	fsd_sendcmd(21, &req, sizeof(req));
	r = fsd_reply();
	fsd_unlock();
	return r;
}

int scepfsdMount(const char *dev, const char *mp, int flags0, void *data0, unsigned int datalen0)
{
	int flags = flags0;
	void *data = data0;
	unsigned int datalen = datalen0;
	struct {
		char dev[1024];
		char mp[1024];
		int flags;
		void *data;
		unsigned int datalen;
	} req;
	int r;
	strcpy(req.dev, dev);
	strcpy(req.mp, mp);
	req.flags = flags;
	req.data = data;
	req.datalen = datalen;
	fsd_lock();
	if (data && datalen)
		memcpy(g_fsdShmBuf, data, datalen);
	fsd_sendcmd(22, &req, sizeof(req));
	r = fsd_reply();
	fsd_unlock();
	return r;
}

int scepfsdUmount(const char *mp)
{
	char buf[1024];
	int r;
	strcpy(buf, mp);
	fsd_lock();
	fsd_sendcmd(23, buf, sizeof(buf));
	r = fsd_reply();
	fsd_unlock();
	return r;
}

int scepfsdDevctl(const char *name, int cmd0, void *args0, unsigned int arglen0, void *buf0, unsigned int buflen0)
{
	int cmd = cmd0;
	void *args = args0;
	unsigned int arglen = arglen0;
	void *buf = buf0;
	unsigned int buflen = buflen0;
	struct {
		char path[1024];
		int cmd;
		void *args;
		unsigned int arglen;
		void *buf;
		unsigned int buflen;
	} req;
	int r;
	strcpy(req.path, name);
	req.cmd = cmd;
	req.args = args;
	req.arglen = arglen;
	req.buf = buf;
	req.buflen = buflen;
	fsd_lock();
	if (args && arglen)
		memcpy(g_fsdShmBuf, args, arglen);
	fsd_sendcmd(24, &req, sizeof(req));
	r = fsd_reply();
	if (buf && buflen)
		memcpy(buf, g_fsdShmBuf, buflen);
	fsd_unlock();
	return r;
}

int scepfsdSetReadAhead(int val0)
{
	int val = val0;
	int r;
	fsd_lock();
	fsd_sendcmd(25, &val, sizeof(val));
	r = fsd_reply();
	fsd_unlock();
	return r;
}

int scepfsdGetReadAhead(void)
{
	int r;
	fsd_lock();
	fsd_sendcmd(26, 0, 0);
	r = fsd_reply();
	fsd_unlock();
	return r;
}

int scepfsdGetMountPoint(const char *name, char *buf0)
{
	char *buf = buf0;
	char req[1024];
	int r;
	strcpy(req, name);
	fsd_lock();
	fsd_sendcmd(27, req, sizeof(req));
	r = fsd_reply();
	if (buf)
		strcpy(buf, g_fsdShmBuf);
	fsd_unlock();
	return r;
}

/* The public scepfs* names are weak aliases, so another implementation of the
   scepfs interface can replace this client without touching the callers. */
extern __typeof(scepfsdInit) scepfsInit __attribute__((weak, alias("scepfsdInit")));
extern __typeof(scepfsdExit) scepfsExit __attribute__((weak, alias("scepfsdExit")));
extern __typeof(scepfsdOpen) scepfsOpen __attribute__((weak, alias("scepfsdOpen")));
extern __typeof(scepfsdLseek) scepfsLseek __attribute__((weak, alias("scepfsdLseek")));
extern __typeof(scepfsdLseek64) scepfsLseek64 __attribute__((weak, alias("scepfsdLseek64")));
extern __typeof(scepfsdRead) scepfsRead __attribute__((weak, alias("scepfsdRead")));
extern __typeof(scepfsdWrite) scepfsWrite __attribute__((weak, alias("scepfsdWrite")));
extern __typeof(scepfsdClose) scepfsClose __attribute__((weak, alias("scepfsdClose")));
extern __typeof(scepfsdDopen) scepfsDopen __attribute__((weak, alias("scepfsdDopen")));
extern __typeof(scepfsdDread) scepfsDread __attribute__((weak, alias("scepfsdDread")));
extern __typeof(scepfsdDclose) scepfsDclose __attribute__((weak, alias("scepfsdDclose")));
extern __typeof(scepfsdChstat) scepfsChstat __attribute__((weak, alias("scepfsdChstat")));
extern __typeof(scepfsdGetstat) scepfsGetstat __attribute__((weak, alias("scepfsdGetstat")));
extern __typeof(scepfsdMkdir) scepfsMkdir __attribute__((weak, alias("scepfsdMkdir")));
extern __typeof(scepfsdRmdir) scepfsRmdir __attribute__((weak, alias("scepfsdRmdir")));
extern __typeof(scepfsdChdir) scepfsChdir __attribute__((weak, alias("scepfsdChdir")));
extern __typeof(scepfsdRemove) scepfsRemove __attribute__((weak, alias("scepfsdRemove")));
extern __typeof(scepfsdRename) scepfsRename __attribute__((weak, alias("scepfsdRename")));
extern __typeof(scepfsdIoctl) scepfsIoctl __attribute__((weak, alias("scepfsdIoctl")));
extern __typeof(scepfsdIoctl2) scepfsIoctl2 __attribute__((weak, alias("scepfsdIoctl2")));
extern __typeof(scepfsdSymlink) scepfsSymlink __attribute__((weak, alias("scepfsdSymlink")));
extern __typeof(scepfsdReadlink) scepfsReadlink __attribute__((weak, alias("scepfsdReadlink")));
extern __typeof(scepfsdSync) scepfsSync __attribute__((weak, alias("scepfsdSync")));
extern __typeof(scepfsdFormat) scepfsFormat __attribute__((weak, alias("scepfsdFormat")));
extern __typeof(scepfsdMount) scepfsMount __attribute__((weak, alias("scepfsdMount")));
extern __typeof(scepfsdUmount) scepfsUmount __attribute__((weak, alias("scepfsdUmount")));
extern __typeof(scepfsdDevctl) scepfsDevctl __attribute__((weak, alias("scepfsdDevctl")));
extern __typeof(scepfsdSetReadAhead) scepfsSetReadAhead __attribute__((weak, alias("scepfsdSetReadAhead")));
extern __typeof(scepfsdGetReadAhead) scepfsGetReadAhead __attribute__((weak, alias("scepfsdGetReadAhead")));
extern __typeof(scepfsdGetMountPoint) scepfsGetMountPoint __attribute__((weak, alias("scepfsdGetMountPoint")));
