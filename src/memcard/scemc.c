/*
 * sceMc*: memory card library of the PS2 Linux Kit. Requests go to the mcserv
 * IOP module through ioctls on /dev/ps2mc00, with their buffers in two pages
 * mapped from /dev/ps2mem. g_mcResult holds the operation in flight: every
 * new request returns it until sceMcSync collects the result.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/ps2/dev.h>
#include <linux/ps2/mcio.h>
#include "lib/ps2dev_ext.h"
#include "memcard/scemc.h"

/* Driver requests missing from the kit headers. */
#define PS2MC_IOCSIFCALLRPC  _IO(PS2MC_IOCTL, 7)     /* send a SIF RPC request packet */

/* Request without a path (mcClientID.reqSmall). GetInfo passes its three
 * requests as flags in origin (type), offset (free) and size (format); Write
 * sends the first `origin` bytes, up to the 16-byte alignment of the buffer,
 * inline in data and the rest from `buffer`. */
typedef struct {
	/* 0x00 */ int fd;
	/* 0x04 */ int port;
	/* 0x08 */ int slot;
	/* 0x0C */ int size;
	/* 0x10 */ int offset;
	/* 0x14 */ int origin;  /* whence of Seek, priority of ChangeThreadPriority */
	/* 0x18 */ unsigned int buffer;  /* physical address of the data */
	/* 0x1C */ unsigned int param;   /* physical address of mcClientID.paramNext */
	/* 0x20 */ char data[16];
} McReqSmall; /* size = 0x30 */

/* Request with a path (mcClientID.reqBig): five words, then the path. */
typedef struct {
	int port, slot, mode, arg, bufAddr;   /* bufAddr: physical address of the reply buffer */
	char path[1024];
} McReqBig;

/* Packet handed to the mcserv driver by sceSifCallRpc. */
typedef struct SifRpcPacket {
	/* 0x00 */ int flag;              /* set to 1 last */
	/* 0x04 */ int fno;
	/* 0x08 */ unsigned int sendAddr;   /* physical address of sendbuf */
	/* 0x0C */ int ssize;
	/* 0x10 */ unsigned int recvAddr;   /* physical address of recvbuf */
	/* 0x14 */ int rsize;
} SifRpcPacket; /* size = 0x18 */

typedef struct McState McState;
struct McState {
	/* 0x00 */ int mcFd;   /* /dev/ps2mc00 */
	/* 0x04 */ int memFd;   /* /dev/ps2mem */
	/* 0x08 */ int initFlag;
	/* 0x0C */ SifRpcPacket *rpcreq; /* RPC packet, 24 bytes */
	/* 0x10 */ int *recvBuf;    /* RPC reply ("retval") */
	/* 0x14 */ void *scratchBuf; /* "buffFileInfo": directory entries, Rename/SetFileInfo names */
	/* 0x18 */ int maxDirEntries; /* entries per sceMcGetDir chunk */
	/* 0x1C */ void *stagingBuf;   /* data buffer of chunked Read/Write */
	/* 0x20 */ int maxChunkSize;   /* largest Read/Write chunk */
	/* 0x24 */ McReqSmall *reqSmall; /* "ParamOrd": request without a path, 48 bytes */
	/* 0x28 */ McReqBig *reqBig; /* "ParamFname": request with a path, 1044 bytes */
	/* 0x2C */ void *paramNext; /* "ParamNext", 192 bytes */
	/* 0x30 */ char *currentDir; /* "currentDir", 1024 bytes */
	/* 0x34 */ char *mapBase;      /* the two pages mapped from /dev/ps2mem */
	/* 0x38 */ char *pageAddr[2];  /* virtual address of each page */
	/* 0x40 */ unsigned int phys[2]; /* physical address of each page */
};

McState mcClientID;
/* Page size cached by sceMcInit; two pages are mapped. */
static struct McCfg {
	unsigned int pageSize;
} g_mcCfg;
#define g_mcPageSize g_mcCfg.pageSize
static volatile int g_mcResult;

static unsigned int McVtoPhys(register char *vaddr);

static char *McPtoVirt(register unsigned int addr);

/* Transfer descriptor of McCopyOut: two (count, physical destination) pairs
 * and their inline source buffers. */
struct McXfer {
	/* 0x00 */ int count0;
	/* 0x04 */ int count1;
	/* 0x08 */ unsigned int addr0;
	/* 0x0C */ unsigned int addr1;
	/* 0x10 */ unsigned char buf0[64];
	/* 0x50 */ unsigned char buf1[64];
}; /* size = 0x90 */


/* Out pointers of sceMcGetInfo, filled by McRpcEndFunc3. */
static int *g_mcInfoType;
static int *g_mcInfoFree;
static int *g_mcInfoFormat;

/* Reply of sceMcGetInfo: the three values handed to the caller. */
struct McInfoReply {
	/* 0x00 */ int type;
	/* 0x04 */ int free;
	/* 0x08 */ char pad_08[136];
	/* 0x90 */ int format;
};


#define PS2SIF_ALLOC(dst, size)                                     \
	for (;;) {                                                  \
		(dst) = (void *)(p = (char *)(((unsigned int)p + 63) / 64 * 64)); \
		p += (size);                                        \
		break;                                              \
	}


static void McCopyOut(void *);
static void McGetCurrentDir(char *);

int sceMcExit(void)
{
	mcClientID.initFlag = 0;

	munmap(0, g_mcPageSize * 2);

	close(mcClientID.memFd);
	close(mcClientID.mcFd);

	return 0;
}

/*
 * Opens /dev/ps2mem and /dev/ps2mc00, maps two shared pages, reads their
 * physical addresses and carves the six RPC buffers out of the first page,
 * 64-byte aligned. Returns -101 on failure.
 */
int sceMcInit(void)
{
	int i;
	int unused;  /* unused, but part of the layout */
	char *base;
	char *p;
	int pagesize;

	if ((mcClientID.memFd = open("/dev/ps2mem", O_RDWR)) < 0) {
		perror("open(/dev/ps2mem)");
		return -101;
	}

	if ((mcClientID.mcFd = open("/dev/ps2mc00", O_RDWR)) < 0) {
		perror("open(/dev/ps2mc)");
		return -101;
	}

	mcClientID.initFlag = 1;

	g_mcPageSize = getpagesize();

	mcClientID.mapBase = mmap(0, g_mcPageSize * 2, PROT_READ | PROT_WRITE, MAP_SHARED, mcClientID.memFd, 0);
	if (mcClientID.mapBase == 0) {  /* NOTE: bug: mmap fails with MAP_FAILED, not 0 */
		perror("mmap");
		return -101;
	}

	for (i = 0; i < 2; i++)
		mcClientID.pageAddr[i] = mcClientID.mapBase + g_mcPageSize * i;

	if (ioctl(mcClientID.memFd, PS2IOC_PHYSADDR, &mcClientID.phys[0]) < 0) {
		perror("ioctl(PS2IOC_PHYSADDR)");
		return -101;
	}

	for (i = 0; i < 2; i++)
		printf("%d: 0x%08x 0x%08x\n", i, mcClientID.pageAddr[i],
		    (mcClientID.phys[i] & 0x3fffffff) | 0x80000000);

	if (mcClientID.pageAddr[0] != 0) {
		base = mcClientID.pageAddr[0];
		p = mcClientID.pageAddr[0];
		pagesize = g_mcPageSize;

		PS2SIF_ALLOC(mcClientID.rpcreq, 24);
		PS2SIF_ALLOC(mcClientID.recvBuf, 16);
		PS2SIF_ALLOC(mcClientID.reqSmall, 48);
		PS2SIF_ALLOC(mcClientID.reqBig, 1044);
		PS2SIF_ALLOC(mcClientID.paramNext, 192);
		PS2SIF_ALLOC(mcClientID.currentDir, 1024);

		if (pagesize < p - base) {
			fprintf(stderr, "*********************************\n");
			fprintf(stderr, "PS2SIF_ALLOC overrun %dbytes\n", p - base - pagesize);
			fprintf(stderr, "libmc\n");
			fprintf(stderr, "*********************************\n");
		}
	}

	mcClientID.scratchBuf = mcClientID.pageAddr[1];
	mcClientID.maxDirEntries = g_mcPageSize / 64;
	mcClientID.stagingBuf = mcClientID.pageAddr[1];
	mcClientID.maxChunkSize = g_mcPageSize;

	printf("rpcreq:       %08x\n", mcClientID.rpcreq);
	printf("retval:       %08x\n", mcClientID.recvBuf);
	printf("buffFileInfo: %08x (%d entries)\n", mcClientID.scratchBuf, mcClientID.maxDirEntries);
	printf("ParamOrd:     %08x\n", mcClientID.reqSmall);
	printf("ParamFname:   %08x\n", mcClientID.reqBig);
	printf("ParamNext:    %08x\n", mcClientID.paramNext);
	printf("currentDir:   %08x\n", mcClientID.currentDir);

	return 0;
}

/*
 * Not a SIF RPC: builds the packet in client->rpcreq, passes it to an ioctl on
 * /dev/ps2mc00 and then calls endfunc(efarg) synchronously; mode is unused.
 * Returns -2 if the ioctl fails, 0 otherwise.
 */
int sceSifCallRpc(void *client, int fno, int mode, void *sendbuf, int ssize, void *recvbuf, int rsize, void *endfunc,
    void *efarg)
{
	int r;
	SifRpcPacket *pkt;

	pkt = ((McState *)client)->rpcreq;

	pkt->fno = fno;
	pkt->sendAddr = McVtoPhys(sendbuf);
	pkt->ssize = ssize;
	pkt->recvAddr = McVtoPhys(recvbuf);
	pkt->rsize = rsize;
	pkt->flag = 1;

	r = ioctl(mcClientID.mcFd, PS2MC_IOCSIFCALLRPC, (void *)McVtoPhys((char *)pkt));

	if (endfunc)
		((void (*)(void *))endfunc)(efarg);

	return (r < 0) ? -2 : 0;
}

/*
 * Reads up to maxEntries 64-byte directory entries, maxDirEntries at a time;
 * mode becomes 1 after the first chunk to continue the scan.
 */
int sceMcGetDir(int port, int slot, const char *name, unsigned int mode, int maxEntries, sceMcTblGetDir *table)
{
	int chunk;
	int r;
	int totalEntries = 0;

	while (maxEntries > 0) {
		chunk = (maxEntries < mcClientID.maxDirEntries) ? maxEntries : mcClientID.maxDirEntries;

		r = sceMcGetDir_sub(port, slot, name, mode, chunk, mcClientID.scratchBuf);
		if (r != 0)
			return r;

		r = *mcClientID.recvBuf;
		g_mcResult = 0;

		if (r < 0)
			return r;

		if (r > 0) {
			totalEntries += r;
			memcpy(table, mcClientID.scratchBuf, r * 64);
		}

		if (r < chunk)
			break;

		mode = 1;
		table += r;
		maxEntries -= r;
	}

	*mcClientID.recvBuf = totalEntries;
	g_mcResult = 13;
	return 0;
}

/* Reads count bytes in chunks through the shared staging buffer; the total
 * read is left in the reply buffer. */
int sceMcRead(int fd, void *buf, int count)
{
	int r;
	int chunk;
	int written = 0;

	while (count > 0) {
		chunk = (count < mcClientID.maxChunkSize) ? count : mcClientID.maxChunkSize;

		r = sceMcRead_sub(fd, mcClientID.stagingBuf, chunk);
		if (r != 0) {
			if (written > 0)
				goto finish;
			else
				return r;
		}

		r = *mcClientID.recvBuf;
		g_mcResult = 0;

		if (r <= 0) {
			if (written > 0)
				goto finish;
			else
				goto pending_only;
		}

		memcpy(buf, mcClientID.stagingBuf, r);
		buf = (char *)buf + r;
		count -= r;
		written += r;
	}

finish:
	*mcClientID.recvBuf = written;
pending_only:
	g_mcResult = 5;
	return 0;
}

/* Writes count bytes in chunks through the shared staging buffer. */
int sceMcWrite(int fd, const void *buf, int count)
{
	int r;
	int chunk;
	int written = 0;

	while (count > 0) {
		chunk = (count < mcClientID.maxChunkSize) ? count : mcClientID.maxChunkSize;

		memcpy(mcClientID.stagingBuf, buf, chunk);

		r = sceMcWrite_sub(fd, mcClientID.stagingBuf, chunk);
		if (r != 0) {
			if (written > 0)
				goto finish;
			else
				return r;
		}

		r = *mcClientID.recvBuf;
		g_mcResult = 0;

		if (r <= 0) {
			if (written > 0)
				goto finish;
			else
				goto pending_only;
		}

		buf = (const char *)buf + r;
		count -= r;
		written += r;
		*mcClientID.recvBuf = written;
	}

finish:
	*mcClientID.recvBuf = written;
pending_only:
	g_mcResult = 6;
	return 0;
}

int sceMcChangeThreadPriority(int prio)
{
	int r;

	if (g_mcResult != 0)
		return g_mcResult;

	if (!mcClientID.initFlag)
		return -100;

	mcClientID.reqSmall->origin = prio;

	r = sceSifCallRpc(&mcClientID, 20, 0, mcClientID.reqSmall, 48, mcClientID.recvBuf, 4, 0, 0);
	if (r == 0)
		g_mcResult = 20;

	return r;
}

int sceMcGetSlotMax(int port)
{
	int r;

	if (g_mcResult != 0)
		return -100 - g_mcResult;

	if (!mcClientID.initFlag)
		return -100;

	mcClientID.reqSmall->port = port;

	r = sceSifCallRpc(&mcClientID, 21, 0, mcClientID.reqSmall, 48, mcClientID.recvBuf, 4, 0, 0);
	if (r != 0)
		return r;

	return *mcClientID.recvBuf;
}

int sceMcOpen(int port, int slot, const char *name, int mode)
{
	int r;

	if (g_mcResult != 0)
		return g_mcResult;

	if (!mcClientID.initFlag)
		return -100;

	strncpy(mcClientID.reqBig->path, name, 1023);

	mcClientID.reqBig->path[1023] = 0;
	mcClientID.reqBig->mode = mode;
	mcClientID.reqBig->port = port;
	mcClientID.reqBig->slot = slot;

	r = sceSifCallRpc(&mcClientID, 2, 0, mcClientID.reqBig, 1044, mcClientID.recvBuf, 4, 0, 0);
	if (r == 0)
		g_mcResult = 2;

	return r;
}

/* Mkdir is Open with mode 64. */
int sceMcMkdir(int port, int slot, const char *name)
{
	int r;

	r = sceMcOpen(port, slot, name, 64);
	if (r == 0)
		g_mcResult = 11;

	return r;
}

int sceMcClose(int fd)
{
	int r;

	if (g_mcResult != 0)
		return g_mcResult;

	if (!mcClientID.initFlag)
		return -100;

	mcClientID.reqSmall->fd = fd;

	r = sceSifCallRpc(&mcClientID, 3, 0, mcClientID.reqSmall, 48, mcClientID.recvBuf, 4, 0, 0);
	if (r == 0)
		g_mcResult = 3;

	return r;
}

int sceMcSeek(int fd, int offset, int whence)
{
	int r;

	if (g_mcResult != 0)
		return g_mcResult;

	if (!mcClientID.initFlag)
		return -100;

	mcClientID.reqSmall->fd = fd;
	mcClientID.reqSmall->offset = offset;
	mcClientID.reqSmall->origin = whence;

	r = sceSifCallRpc(&mcClientID, 4, 0, mcClientID.reqSmall, 48, mcClientID.recvBuf, 4, 0, 0);
	if (r == 0)
		g_mcResult = 4;

	return r;
}

/* Copies the two inline buffers of a transfer descriptor to their physical
 * destinations; a pair with count 0 is skipped. */
static void McCopyOut(void *arg)
{
	struct McXfer *p = (struct McXfer *)arg;
	char *dst;
	int i;

	if (p->count0 != 0) {
		dst = McPtoVirt(p->addr0);
		for (i = 0; i < p->count0; i++)
			*dst++ = p->buf0[i];
	}
	if (p->count1 != 0) {
		dst = McPtoVirt(p->addr1);
		for (i = 0; i < p->count1; i++)
			*dst++ = p->buf1[i];
	}
}

/* Function 5: reads a chunk into buf. */
int sceMcRead_sub(int fd, void *buf, int size)
{
	int r;

	if (g_mcResult != 0)
		return g_mcResult;

	if (!mcClientID.initFlag)
		return -100;

	mcClientID.reqSmall->fd = fd;
	mcClientID.reqSmall->buffer = McVtoPhys(buf);
	mcClientID.reqSmall->size = size;
	mcClientID.reqSmall->param = McVtoPhys(mcClientID.paramNext);

	do {  /* redundant as written */
	} while (0);
	do {  /* redundant as written */
	} while (0);

	r = sceSifCallRpc(&mcClientID, 5, 0, mcClientID.reqSmall, 48, mcClientID.recvBuf, 4, McCopyOut,
	    mcClientID.paramNext);
	if (r == 0)
		g_mcResult = 5;

	return r;
}

/* Function 6: writes a chunk. Above 16 bytes the head, up to the next 16-byte
   boundary, goes inline in the request (offset 32) and the aligned rest by
   address; smaller writes go inline entirely. */
int sceMcWrite_sub(int fd, void *buf, int size)
{
	unsigned int i;
	int r;

	if (g_mcResult != 0)
		return g_mcResult;

	if (!mcClientID.initFlag)
		return -100;

	mcClientID.reqSmall->fd = fd;

	if (size < 17) {
		mcClientID.reqSmall->origin = size;
		mcClientID.reqSmall->size = 0;
		mcClientID.reqSmall->buffer = 0;
	} else {
		mcClientID.reqSmall->origin = 16 + ((((unsigned int)buf - 1) & ~15)) - (unsigned int)buf;
		mcClientID.reqSmall->size = size - mcClientID.reqSmall->origin;
		mcClientID.reqSmall->buffer = McVtoPhys((char *)buf + mcClientID.reqSmall->origin);
	}

	for (i = 0; i < (unsigned int)mcClientID.reqSmall->origin; i++)
		mcClientID.reqSmall->data[i] = ((char *)buf)[i];

	do {  /* redundant as written */
	} while (0);

	r = sceSifCallRpc(&mcClientID, 6, 0, mcClientID.reqSmall, 48, mcClientID.recvBuf, 4, 0, 0);
	if (r == 0)
		g_mcResult = 6;

	return r;
}

/* Returns 1 and clears g_mcResult when an operation is pending (its result
 * in *dataOut), -1 if none is. The busy wait of mode 0 is never reached. */
int sceMcSync(int mode, int *statusOut, int *dataOut)
{
	int pending;
	int i;

	if (g_mcResult == 0)
		return -1;

	pending = 0;
	if (mode == 0) {
		if (pending) {  /* redundant as written */
			while (0) {  /* redundant as written */
				for (i = 0; i <= 99999; i++)
					;
			}
			pending = 0;
		}
	}

	pending = !pending;

	if (statusOut)
		*statusOut = g_mcResult;

	if (pending) {
		g_mcResult = 0;
		if (dataOut)
			*dataOut = *mcClientID.recvBuf;
	}

	return pending;
}

/* Completion of sceMcGetInfo: copies the reply to the non-null out pointers. */
static void McRpcEndFunc3(void *arg)
{
	struct McInfoReply *p = (struct McInfoReply *)arg;

	if (g_mcInfoType)
		*g_mcInfoType = p->type;
	if (g_mcInfoFree)
		*g_mcInfoFree = p->free;
	if (g_mcInfoFormat)
		*g_mcInfoFormat = p->format;
}

int sceMcGetInfo(int port, int slot, int *type, int *free, int *format)
{
	int r;

	if (g_mcResult != 0)
		return g_mcResult;

	if (!mcClientID.initFlag)
		return -100;

	mcClientID.reqSmall->port = port;
	mcClientID.reqSmall->slot = slot;

	mcClientID.reqSmall->param = McVtoPhys(mcClientID.paramNext);

	if (type)
		mcClientID.reqSmall->origin = 1;
	else
		mcClientID.reqSmall->origin = 0;

	if (free)
		mcClientID.reqSmall->offset = 1;
	else
		mcClientID.reqSmall->offset = 0;

	if (format)
		mcClientID.reqSmall->size = 1;
	else
		mcClientID.reqSmall->size = 0;

	g_mcInfoType = type;
	g_mcInfoFree = free;
	g_mcInfoFormat = format;

	do {  /* redundant as written */
	} while (0);

	r = sceSifCallRpc(&mcClientID, 1, 0, mcClientID.reqSmall, 48, mcClientID.recvBuf, 4, McRpcEndFunc3,
	    mcClientID.paramNext);
	if (r == 0)
		g_mcResult = 1;

	return r;
}

/* Function 13: reads directory entries into buf. */
int sceMcGetDir_sub(int port, int slot, const char *name, int mode, int count, void *buf)
{
	int r;

	if (g_mcResult != 0)
		return g_mcResult;

	if (!mcClientID.initFlag)
		return -100;

	mcClientID.reqBig->port = port;
	mcClientID.reqBig->slot = slot;
	mcClientID.reqBig->mode = mode;
	mcClientID.reqBig->arg = count;
	mcClientID.reqBig->bufAddr = McVtoPhys(buf);

	strncpy(mcClientID.reqBig->path, name, 1023);
	mcClientID.reqBig->path[1023] = 0;

	do {  /* redundant as written */
	} while (0);

	r = sceSifCallRpc(&mcClientID, 13, 0, mcClientID.reqBig, 1044, mcClientID.recvBuf, 4, 0, 0);
	if (r == 0)
		g_mcResult = 13;

	return r;
}

/* Completion of sceMcChdir: copies the current directory (at most 1023
 * chars) to dst, if dst is not null. */
static void McGetCurrentDir(char *dst)
{
	int n;

	if (dst != 0) {
		n = strlen(mcClientID.currentDir) < 1024 ? strlen(mcClientID.currentDir) : 1023;
		memcpy(dst, mcClientID.currentDir, n);
		dst[n] = 0;
	}
}

int sceMcChdir(int port, int slot, const char *name, char *curDir)
{
	int r;

	if (g_mcResult != 0)
		return g_mcResult;

	if (!mcClientID.initFlag)
		return -100;

	mcClientID.reqBig->port = port;
	mcClientID.reqBig->slot = slot;

	mcClientID.reqBig->bufAddr = McVtoPhys(mcClientID.currentDir);

	strncpy(mcClientID.reqBig->path, name, 1023);
	mcClientID.reqBig->path[1023] = 0;

	do {  /* redundant as written */
	} while (0);

	r = sceSifCallRpc(&mcClientID, 12, 0, mcClientID.reqBig, 1044, mcClientID.recvBuf, 4, McGetCurrentDir, curDir);
	if (r == 0)
		g_mcResult = 12;

	return r;
}

int sceMcFormat(int port, int slot)
{
	int r;

	if (g_mcResult != 0)
		return g_mcResult;

	if (!mcClientID.initFlag)
		return -100;

	mcClientID.reqSmall->port = port;
	mcClientID.reqSmall->slot = slot;

	r = sceSifCallRpc(&mcClientID, 16, 0, mcClientID.reqSmall, 48, mcClientID.recvBuf, 4, 0, 0);
	if (r == 0)
		g_mcResult = 16;

	return r;
}

int sceMcDelete(int port, int slot, const char *name)
{
	int r;

	if (g_mcResult != 0)
		return g_mcResult;

	if (!mcClientID.initFlag)
		return -100;

	strncpy(mcClientID.reqBig->path, name, 1023);
	mcClientID.reqBig->path[1023] = 0;
	mcClientID.reqBig->mode = 0;
	mcClientID.reqBig->port = port;
	mcClientID.reqBig->slot = slot;

	r = sceSifCallRpc(&mcClientID, 15, 0, mcClientID.reqBig, 1044, mcClientID.recvBuf, 4, 0, 0);
	if (r == 0)
		g_mcResult = 15;

	return r;
}

int sceMcFlush(int fd)
{
	int r;

	if (g_mcResult != 0)
		return g_mcResult;

	if (!mcClientID.initFlag)
		return -100;

	mcClientID.reqSmall->fd = fd;

	r = sceSifCallRpc(&mcClientID, 10, 0, mcClientID.reqSmall, 48, mcClientID.recvBuf, 4, 0, 0);
	if (r == 0)
		g_mcResult = 10;

	return r;
}

int sceMcSetFileInfo(int port, int slot, const char *name, const void *info, unsigned int flags)
{
	int r;

	if (g_mcResult != 0)
		return g_mcResult;

	if (!mcClientID.initFlag)
		return -100;

	flags = flags & 7;

	mcClientID.reqBig->port = port;
	mcClientID.reqBig->slot = slot;
	mcClientID.reqBig->mode = flags;

	memcpy(mcClientID.scratchBuf, info, 64);

	mcClientID.reqBig->bufAddr = McVtoPhys(mcClientID.scratchBuf);

	strncpy(mcClientID.reqBig->path, name, 1023);
	mcClientID.reqBig->path[1023] = 0;

	do {  /* redundant as written */
	} while (0);

	r = sceSifCallRpc(&mcClientID, 14, 0, mcClientID.reqBig, 1044, mcClientID.recvBuf, 4, 0, 0);
	if (r == 0)
		g_mcResult = 14;

	return r;
}

/* Function 14 with mode 16, recorded in g_mcResult as operation 19. The new
   name goes in mcClientID.scratchBuf at offset 32, at most 31 chars. */
int sceMcRename(int port, int slot, const char *name, const char *newName)
{
	int r;

	if (g_mcResult != 0)
		return g_mcResult;

	if (!mcClientID.initFlag)
		return -100;

	mcClientID.reqBig->port = port;
	mcClientID.reqBig->slot = slot;
	mcClientID.reqBig->mode = 16;

	strncpy(mcClientID.reqBig->path, name, 1023);
	mcClientID.reqBig->path[1023] = 0;

	strncpy(((sceMcTblGetDir *)mcClientID.scratchBuf)->EntryName, newName, 32);
	((sceMcTblGetDir *)mcClientID.scratchBuf)->EntryName[31] = 0;

	mcClientID.reqBig->bufAddr = McVtoPhys(mcClientID.scratchBuf);

	do {  /* redundant as written */
	} while (0);

	r = sceSifCallRpc(&mcClientID, 14, 0, mcClientID.reqBig, 1044, mcClientID.recvBuf, 4, 0, 0);
	if (r == 0)
		g_mcResult = 19;

	return r;
}

int sceMcUnformat(int port, int slot)
{
	int r;

	if (g_mcResult != 0)
		return g_mcResult;

	if (!mcClientID.initFlag)
		return -100;

	mcClientID.reqSmall->port = port;
	mcClientID.reqSmall->slot = slot;

	r = sceSifCallRpc(&mcClientID, 17, 0, mcClientID.reqSmall, 48, mcClientID.recvBuf, 4, 0, 0);
	if (r == 0)
		g_mcResult = 17;

	return r;
}

int sceMcGetEntSpace(int port, int slot, const char *name)
{
	int r;

	if (g_mcResult != 0)
		return g_mcResult;

	if (!mcClientID.initFlag)
		return -100;

	mcClientID.reqBig->port = port;
	mcClientID.reqBig->slot = slot;

	strncpy(mcClientID.reqBig->path, name, 1023);
	mcClientID.reqBig->path[1023] = 0;

	r = sceSifCallRpc(&mcClientID, 18, 0, mcClientID.reqBig, 1044, mcClientID.recvBuf, 4, 0, 0);
	if (r == 0)
		g_mcResult = 18;

	return r;
}

/* Translation between virtual and physical addresses of the two mapped
 * pages; an address outside them is fatal. */
static unsigned int McVtoPhys(register char *vaddr)  /* redundant as written */
{
	int i;
	int off;

	i = (vaddr - mcClientID.mapBase) / (int)g_mcPageSize;
	if (i < 0 || i >= 2) {
		fprintf(stderr, "### vtopaddr failed, vaddr=%x\n", vaddr);
		exit(1);
	}
	off = vaddr - (i * (int)g_mcPageSize + mcClientID.mapBase);
	return ((mcClientID.phys[i] + off) & 0x3fffffff) | 0x80000000;
}

static char *McPtoVirt(register unsigned int addr)  /* redundant as written */
{
	int i;
	int off;

	off = addr & (g_mcPageSize - 1);
	addr = addr - off;
	for (i = 1; i >= 0; i--) {
		if (((mcClientID.phys[i] & 0x3fffffff) | 0x80000000) == addr)
			return mcClientID.pageAddr[i] + off;
	}
	fprintf(stderr, "### ptovaddr failed, addr=%x+%x\n", addr, off);
	exit(1);
	return 0;
}
