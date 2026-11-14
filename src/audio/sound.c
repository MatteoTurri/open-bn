/*
 * Sound: system sound start/stop (sample banks and sequences built into the
 * program, played through libsdr) and the midi* API for BGM and SE banks
 * loaded from files or memory.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "audio/msin.h"
#include "audio/scesd.h"
#include "audio/sound.h"
#include "natives/sndextension.h"

/* Built-in sound data, 64-byte aligned for DMA to the IOP: sample banks
   (bd), bank headers (SShd) and sequences (SSsq), copied to SPU2 and IOP
   memory by soundSetup. */
#define SD_DMA_ALIGN __attribute__((aligned(64)))

/* SSsq, IOP buffer +20480 */
static unsigned char sdSeqA[320] SD_DMA_ALIGN = {
#include "../../assets/sound/sdSeqA.inc"
};
/* SSsq, IOP buffer +28672 */
static unsigned char sdSeqB[320] SD_DMA_ALIGN = {
#include "../../assets/sound/sdSeqB.inc"
};
/* SSsq, IOP buffer +0x9000 */
static unsigned char sdSeqC[448] SD_DMA_ALIGN = {
#include "../../assets/sound/sdSeqC.inc"
};
/* SE samples, SPU2 0x75010 */
unsigned char bbsebd[177136] SD_DMA_ALIGN = {
#include "../../assets/sound/bbsebd.inc"
};
/* BGM samples, SPU2 0x5010 */
static unsigned char sdBgmBd[424240] SD_DMA_ALIGN = {
#include "../../assets/sound/sdBgmBd.inc"
};
/* SE SShd, IOP buffer +24576 */
unsigned char bbsehdo[2784] SD_DMA_ALIGN = {
#include "../../assets/sound/bbsehdo.inc"
};
/* BGM SShd, IOP buffer +0 */
static unsigned char sdBgmHd[1394] SD_DMA_ALIGN = {
#include "../../assets/sound/sdBgmHd.inc"
};
/* SSsq, IOP buffer +4096 */
static unsigned char sdSeq1[1495] SD_DMA_ALIGN = {
#include "../../assets/sound/sdSeq1.inc"
};
/* IOP buffer +8192 */
static unsigned char sdSeq2[1276] SD_DMA_ALIGN = {
#include "../../assets/sound/sdSeq2.inc"
};
/* IOP buffer +12288 */
static unsigned char sdSeq3[402] SD_DMA_ALIGN = {
#include "../../assets/sound/sdSeq3.inc"
};
/* IOP buffer +16384 */
static unsigned char sdSeq4[307] SD_DMA_ALIGN = {
#include "../../assets/sound/sdSeq4.inc"
};
/* IOP buffer +0x8000 */
static unsigned char sdSeq5[1100] SD_DMA_ALIGN = {
#include "../../assets/sound/sdSeq5.inc"
};

/* IOP buffer handle, -1 = not allocated. */
static int sdBuf = -1;

/* Sends a sample bank to SPU2 in 64 KB blocks through the IOP buffer
   (sceSdRemote 4192 = transfer, 5456 = wait). */
static __inline__ void sdTransBank(unsigned char *src, int spu, unsigned int size)
{
	int buf = sdBuf;
	int n = 0x10000;
	unsigned int cnt = size / n;
	unsigned int rest = size % n;
	unsigned int i;

	for (i = 0; i < cnt; i++) {
		sceSdTransToIOP(src + i * n, buf, n);
		sceSdRemote(1, 4192, buf, spu + i * n, n);
		sceSdRemote(1, 5456);
	}
	sceSdTransToIOP(src + i * n, buf, rest);
	sceSdRemote(1, 4192, buf, spu + i * n, rest);
	sceSdRemote(1, 5456);
}

/* Allocates the 64 KB IOP buffer, loads both sample banks into SPU2 and
   copies the headers and sequences into the buffer. */
static int soundSetup(void)
{
	if (sdBuf >= 0)
		sceSdFreeIopHeap(sdBuf);
	sdBuf = sceSdAllocIopHeap(0x10000);
	if (sdBuf < 0)
		return -1;

	sdTransBank(sdBgmBd, 0x5010, sizeof(sdBgmBd));
	sdTransBank(bbsebd, 0x75010, sizeof(bbsebd));

	sceSdTransToIOP(sdBgmHd, sdBuf, sizeof(sdBgmHd));
	sceSdTransToIOP(bbsehdo, sdBuf + 24576, sizeof(bbsehdo));
	sceSdTransToIOP(sdSeq1, sdBuf + 4096, sizeof(sdSeq1));
	sceSdTransToIOP(sdSeq2, sdBuf + 8192, sizeof(sdSeq2));
	sceSdTransToIOP(sdSeq3, sdBuf + 12288, sizeof(sdSeq3));
	sceSdTransToIOP(sdSeq4, sdBuf + 16384, sizeof(sdSeq4));
	sceSdTransToIOP(sdSeqA, sdBuf + 20480, sizeof(sdSeqA));
	sceSdTransToIOP(sdSeqB, sdBuf + 28672, sizeof(sdSeqB));
	sceSdTransToIOP(sdSeq5, sdBuf + 0x8000, sizeof(sdSeq5));
	sceSdTransToIOP(sdSeqC, sdBuf + 0x9000, sizeof(sdSeqC));
	return 0;
}

static short sdVoiceParam0;
static short sdVoiceParam1;
static short sdVoiceParam2;
static short sdVoiceParam3;
static short sdVoiceParam4;
static short sdVoiceParam5;
static short sdVoiceParam6;
static short sdVoiceParam7;
static short sdVoiceParam8;
static short sdVoiceParam9;

int soundstart(void)
{
	sceSdRemote(1, 4496, 0, 0, 0);
	sceSdRemote(1, 4496, 1, 0, 0);
	sceSdRemote(1, 5488, 0, 1);
	sceSdRemote(1, 5488, 1, 1);
	sceSdRemote(1, 4384, 0, 260);
	sceSdRemote(1, 4384, 1, 260);
	sceSdRemote(1, 4464, sdVoiceParam0, 66);
	sceSdRemote(1, 4464, sdVoiceParam1, 42);
	sceSdRemote(1, 4464, sdVoiceParam2, 45);
	sceSdRemote(1, 4464, sdVoiceParam3, 42);
	sceSdRemote(1, 4464, sdVoiceParam4, 27);
	sceSdRemote(1, 4464, sdVoiceParam5, 27);
	sceSdRemote(1, 4464, sdVoiceParam6, 54);
	sceSdRemote(1, 4464, sdVoiceParam7, 54);
	sceSdRemote(1, 5376, sdVoiceParam9, 28);
	sceSdRemote(1, 4656, 0, 16383, 16383);
	sceSdRemote(1, 4656, 1, 16383, 16383);
	sceSdRemote(1, 4688);
	return 0;
}

int soundinit(void)
{
	if (sceSdRemoteInitFd(g_dspFd) < 0)
		return -1;

	sceSdRemote(1, 4624, 1);
	soundSetup();

	sdVoiceParam8 = sceSdRemote(1, 4160, sdBuf, 20496);
	sdVoiceParam9 = sceSdRemote(1, 4160, sdBuf + 24576, 0x75010);
	sceSdRemote(1, 4480, 60);

	sdVoiceParam0 = sceSdRemote(1, 4176, sdVoiceParam8, sdBuf + 4096);
	sdVoiceParam1 = sceSdRemote(1, 4176, sdVoiceParam8, sdBuf + 8192);
	sdVoiceParam2 = sceSdRemote(1, 4176, sdVoiceParam8, sdBuf + 12288);
	sdVoiceParam3 = sceSdRemote(1, 4176, sdVoiceParam8, sdBuf + 16384);
	sdVoiceParam4 = sceSdRemote(1, 4176, sdVoiceParam8, sdBuf + 20480);
	sdVoiceParam5 = sceSdRemote(1, 4176, sdVoiceParam8, sdBuf + 28672);
	sdVoiceParam6 = sceSdRemote(1, 4176, sdVoiceParam8, sdBuf + 32768);
	sdVoiceParam7 = sceSdRemote(1, 4176, sdVoiceParam8, sdBuf + 36864);

	soundstart();
	return 0;
}

int soundstop(void)
{
	sceSdRemote(1, 4928, sdVoiceParam0, 1, 0);
	sceSdRemote(1, 4928, sdVoiceParam1, 1, 0);
	sceSdRemote(1, 4928, sdVoiceParam2, 1, 0);
	sceSdRemote(1, 4928, sdVoiceParam3, 1, 0);
	sceSdRemote(1, 4928, sdVoiceParam4, 1, 0);
	sceSdRemote(1, 4928, sdVoiceParam5, 1, 0);
	sceSdRemote(1, 4928, sdVoiceParam6, 1, 0);
	sceSdRemote(1, 4928, sdVoiceParam7, 1, 0);
	sceSdRemote(1, 4496, 0, 0, 0);
	sceSdRemote(1, 4496, 1, 0, 0);
	sceSdRemote(1, 5120, 0, 0, 8);
	sceSdRemote(1, 5120, 1, 0, 8);
	sceSdRemote(1, 4944);
	return 0;
}

int soundquit(void)
{
	soundstop();
	sceSdRemote(1, 5440, sdVoiceParam0);
	sceSdRemote(1, 5440, sdVoiceParam1);
	sceSdRemote(1, 5440, sdVoiceParam2);
	sceSdRemote(1, 5440, sdVoiceParam3);
	sceSdRemote(1, 5440, sdVoiceParam4);
	sceSdRemote(1, 5440, sdVoiceParam5);
	sceSdRemote(1, 5440, sdVoiceParam6);
	sceSdRemote(1, 5440, sdVoiceParam7);
	sceSdRemote(1, 4992, sdVoiceParam8);
	sceSdRemote(1, 4992, sdVoiceParam9);
	return 0;
}

/* soundinit without the sceSdRemoteInitFd guard followed by soundstart,
   with 4 instead of 260 in the two 4384 commands. */
void soundStart(void)
{
	sceSdRemote(1, 4624, 1);
	soundSetup();
	sceSdRemote(1, 5488, 0, 1);
	sceSdRemote(1, 5488, 1, 1);
	sceSdRemote(1, 4384, 0, 4);
	sceSdRemote(1, 4384, 1, 4);

	sdVoiceParam8 = sceSdRemote(1, 4160, sdBuf, 20496);
	sdVoiceParam9 = sceSdRemote(1, 4160, sdBuf + 24576, 0x75010);
	sceSdRemote(1, 4480, 60);

	sdVoiceParam0 = sceSdRemote(1, 4176, sdVoiceParam8, sdBuf + 4096);
	sdVoiceParam1 = sceSdRemote(1, 4176, sdVoiceParam8, sdBuf + 8192);
	sdVoiceParam2 = sceSdRemote(1, 4176, sdVoiceParam8, sdBuf + 12288);
	sdVoiceParam3 = sceSdRemote(1, 4176, sdVoiceParam8, sdBuf + 16384);
	sdVoiceParam4 = sceSdRemote(1, 4176, sdVoiceParam8, sdBuf + 20480);
	sdVoiceParam5 = sceSdRemote(1, 4176, sdVoiceParam8, sdBuf + 28672);
	sdVoiceParam6 = sceSdRemote(1, 4176, sdVoiceParam8, sdBuf + 32768);
	sdVoiceParam7 = sceSdRemote(1, 4176, sdVoiceParam8, sdBuf + 36864);

	sceSdRemote(1, 4464, sdVoiceParam0, 66);
	sceSdRemote(1, 4464, sdVoiceParam1, 42);
	sceSdRemote(1, 4464, sdVoiceParam2, 45);
	sceSdRemote(1, 4464, sdVoiceParam3, 42);
	sceSdRemote(1, 4464, sdVoiceParam4, 27);
	sceSdRemote(1, 4464, sdVoiceParam5, 27);
	sceSdRemote(1, 4464, sdVoiceParam6, 54);
	sceSdRemote(1, 4464, sdVoiceParam7, 54);
	sceSdRemote(1, 5376, sdVoiceParam9, 28);
	sceSdRemote(1, 4656, 0, 16383, 16383);
	sceSdRemote(1, 4656, 1, 16383, 16383);
	sceSdRemote(1, 4688);
}

void soundStop(void)
{
	sceSdRemote(1, 4928, sdVoiceParam0, 1, 0);
	sceSdRemote(1, 4928, sdVoiceParam1, 1, 0);
	sceSdRemote(1, 4928, sdVoiceParam2, 1, 0);
	sceSdRemote(1, 4928, sdVoiceParam3, 1, 0);
	sceSdRemote(1, 4928, sdVoiceParam4, 1, 0);
	sceSdRemote(1, 4928, sdVoiceParam5, 1, 0);
	sceSdRemote(1, 4928, sdVoiceParam6, 1, 0);
	sceSdRemote(1, 4928, sdVoiceParam7, 1, 0);
	sceSdRemote(1, 5440, sdVoiceParam0);
	sceSdRemote(1, 5440, sdVoiceParam1);
	sceSdRemote(1, 5440, sdVoiceParam2);
	sceSdRemote(1, 5440, sdVoiceParam3);
	sceSdRemote(1, 5440, sdVoiceParam4);
	sceSdRemote(1, 5440, sdVoiceParam5);
	sceSdRemote(1, 5440, sdVoiceParam6);
	sceSdRemote(1, 5440, sdVoiceParam7);
	sceSdRemote(1, 4992, sdVoiceParam8);
	sceSdRemote(1, 4992, sdVoiceParam9);
	sceSdRemote(1, 4944);
}


/* IOP heap handles of the BGM and SE banks, -1 = none. */
int gDataTopAddrForCslMidi = -1;
int gDataTopAddrForCslMidiSe = -1;

/* Parameter block of sceSdRemote(1, 0x1009050, ...), 64 bytes. */
typedef struct SdMidiParam {
	unsigned int addr;    /* IOP address of the loaded data */
	unsigned int unk_04;
	unsigned int vagSize;   /* size of the wave bank in SPU2 memory */
	unsigned int vagAddr;   /* SPU2 address of the wave bank */
	unsigned int pad[12];
} SdMidiParam;

/* Size of a file, via open/lseek/close.
   NOTE: bug: leaks the descriptor when lseek fails. */
static __inline__ int midiFileSize(const char *name)
{
	int fd;
	int size;

	fd = open(name, O_RDONLY);
	if (fd < 0)
		return -1;
	size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	if (size < 0)
		return -1;
	close(fd);
	return size;
}

/* Reads the file into `buf` and sends it to the IOP at `iopaddr`. */
static __inline__ int midiLoadToIop(const char *name, void *buf, unsigned int iopaddr)
{
	int size;
	int fd;
	int n;

	size = midiFileSize(name);
	if (size <= 0 || size > 0x20000)
		return -1;
	fd = open(name, O_RDONLY);
	if (fd < 0)
		return -1;
	n = read(fd, buf, size);
	close(fd);
	if (n <= 0)
		return -1;
	return sceSdTransToIOP(buf, iopaddr, size);
}

/* Block of sceSdRemote(1, 0x1000070, ...): IOP to SPU2 transfer. */
typedef struct SdSpuTrans {
	unsigned int unk_00;
	unsigned int iopaddr;   /* source IOP heap */
	unsigned int size;
	unsigned int spuaddr;
	unsigned int pad[12];
} SdSpuTrans;

/* Like soundLoadVag, with the data already in EE memory: sent to SPU2 in
   0x20000-byte blocks through a temporary IOP heap. Returns `size`, or -1 if
   the heap cannot be allocated. */
static __inline__ int soundLoadVagPtr(const char *src, unsigned int size, unsigned int spuaddr)
{
	SdSpuTrans p;
	int heap;
	unsigned int blk;
	unsigned int nblk;
	unsigned int rest;
	unsigned int i;

	heap = sceSdAllocIopHeap(0x20000);
	if (heap < 0)
		return -1;
	blk = 0x20000;
	nblk = size / blk;
	rest = size % blk;
	for (i = 0; i < nblk; i++) {
		sceSdTransToIOP((void *)(src + i * blk), heap, blk);
		p.spuaddr = spuaddr + i * blk;
		p.iopaddr = heap;
		p.size = blk;
		p.unk_00 = 0;
		sceSdRemote(1, 0x1000070, (int)&p);
	}
	if (rest != 0) {
		sceSdTransToIOP((void *)(src + i * blk), heap, rest);
		p.spuaddr = spuaddr + i * blk;
		p.iopaddr = heap;
		p.size = rest;
		p.unk_00 = 0;
		sceSdRemote(1, 0x1000070, (int)&p);
	}
	sceSdFreeIopHeap(heap);
	return size;
}

/* Like midiLoadToIop, with the data already in EE memory. */
static __inline__ int midiSendToIop(const void *ptr, int size, unsigned int iopaddr)
{
	if (size <= 0 || size > 0x20000)
		return -1;
	return sceSdTransToIOP((void *)ptr, iopaddr, size);
}


/* Reads a VAG/VB file into an EE buffer and sends it to SPU2 in 0x20000-byte
   blocks through a temporary IOP heap. Returns the size, or -1.
   NOTE: bug: the error paths leak the IOP heap and the buffer. */
static int soundLoadVag(const char *name, unsigned int spuaddr)
{
	SdSpuTrans p;
	int heap;
	int size;
	char *buf;
	int fd;
	int n;
	unsigned int blk;
	unsigned int nblk;
	unsigned int rest;
	unsigned int i;

	heap = sceSdAllocIopHeap(0x20000);
	if (heap < 0)
		return -1;
	size = midiFileSize(name);
	buf = malloc(size);
	if (buf == 0)
		return -1;
	fd = open(name, O_RDONLY);
	if (fd < 0)
		return -1;
	n = read(fd, buf, size);
	close(fd);
	if (n <= 0)
		return -1;

	blk = 0x20000;
	nblk = size / blk;
	rest = size % blk;
	for (i = 0; i < nblk; i++) {
		sceSdTransToIOP(buf + i * blk, heap, blk);
		p.spuaddr = spuaddr + i * blk;
		p.iopaddr = heap;
		p.size = blk;
		p.unk_00 = 0;
		sceSdRemote(1, 0x1000070, (int)&p);
	}
	if (rest != 0) {
		sceSdTransToIOP(buf + i * blk, heap, rest);
		p.spuaddr = spuaddr + i * blk;
		p.iopaddr = heap;
		p.size = rest;
		p.unk_00 = 0;
		sceSdRemote(1, 0x1000070, (int)&p);
	}
	free(buf);
	sceSdFreeIopHeap(heap);
	return size;
}

int midiInitBGM(const char *vbfile, const char *hdfile, const char *sqfile)
{
	SdMidiParam param;
	char buf[0x20000];
	int top;
	unsigned int addr2;

	memset(&param, 0, sizeof(param));

	if (gDataTopAddrForCslMidi > 0)
		sceSdFreeIopHeap(gDataTopAddrForCslMidi);
	top = sceSdAllocIopHeap(0x20000);
	gDataTopAddrForCslMidi = top;
	if (top < 0)
		return -1;

	if (midiLoadToIop(sqfile, buf, top) < 0) {
		printf("SQ load error\n");
		return -1;
	}
	addr2 = top + midiFileSize(sqfile);
	if (midiLoadToIop(hdfile, buf, addr2) < 0) {
		printf("HD load error\n");
		return -1;
	}

	param.addr = addr2;
	param.vagAddr = 0xa5010;
	param.vagSize = soundLoadVag(vbfile, 0xa5010);
	sceSdRemote(1, 0x1009050, 0, &param);
	sceSdRemote(1, 0x10000a0, 0, 0x3010);
	sceSdRemote(1, 0x10000b0, 0, 0xcc);
	sceSdRemote(1, 0x1000040, top);
	return 0;
}

/* sceMSIn context of the SE bank: no input group, one output group with one
   256-byte MIDI stream. */
static sceCslCtx g_seCtx;
static sceCslBuffGrp g_seBuffGrp[2];
static sceCslBuffCtx g_seBuffCtx;
static struct {
	sceCslMidiStream head;
	unsigned char body[248];
} g_seStream;

int midiInitSE(const char *vbfile, const char *hdfile)
{
	SdMidiParam param;
	char buf[0x20000];
	int top;

	memset(&param, 0, sizeof(param));

	if (gDataTopAddrForCslMidiSe > 0)
		sceSdFreeIopHeap(gDataTopAddrForCslMidiSe);
	top = sceSdAllocIopHeap(0x20000);
	gDataTopAddrForCslMidiSe = top;
	if (top < 0)
		return -1;

	if (midiLoadToIop(hdfile, buf, top) < 0) {
		printf("HD load error\n");
		return -1;
	}

	param.addr = top;
	param.vagAddr = 0x17d010;
	param.vagSize = soundLoadVag(vbfile, 0x17d010);
	sceSdRemote(1, 0x1009050, 1, &param);
	sceSdRemote(1, 0x10000a0, 1, 0x840);
	sceSdRemote(1, 0x10000b0, 1, 0x100);

	g_seCtx.extmod = 0;
	g_seCtx.callBack = 0;
	g_seCtx.conf = 0;
	g_seCtx.buffGrpNum = 2;
	g_seCtx.buffGrp = g_seBuffGrp;
	g_seBuffGrp[0].buffNum = 0;
	g_seBuffGrp[0].buffCtx = 0;
	g_seBuffGrp[1].buffNum = 1;
	g_seBuffGrp[1].buffCtx = &g_seBuffCtx;
	g_seBuffCtx.sema = 0;
	g_seBuffCtx.buff = &g_seStream;
	g_seStream.head.buffsize = sizeof(g_seStream);
	g_seStream.head.validsize = 0;
	if (sceMSIn_Init(&g_seCtx) != 0) {
		printf("sceMSIn_Init Error\n");
		return 1;
	}
	sceMSIn_PutMsg(&g_seCtx, 0, 192);
	return 0;
}

/* Like midiInitBGM, with the VB, HD and SQ blocks already in EE memory. */
int midiInitBGMPtr(const char *vbptr, unsigned int vbsize, const void *hdptr, int hdsize, const void *sqptr, int sqsize)
{
	SdMidiParam param;
	int top;
	unsigned int addr2;

	memset(&param, 0, sizeof(param));

	if (gDataTopAddrForCslMidi > 0)
		sceSdFreeIopHeap(gDataTopAddrForCslMidi);
	top = sceSdAllocIopHeap(0x20000);
	gDataTopAddrForCslMidi = top;
	if (top < 0)
		return -1;

	if (midiSendToIop(sqptr, sqsize, top) < 0) {
		printf("SQ load error\n");
		return -1;
	}
	addr2 = top + sqsize;
	if (midiSendToIop(hdptr, hdsize, addr2) < 0) {
		printf("HD load error\n");
		return -1;
	}

	param.addr = addr2;
	param.vagAddr = 0xa5010;
	param.vagSize = soundLoadVagPtr(vbptr, vbsize, 0xa5010);
	sceSdRemote(1, 0x1009050, 0, &param);
	sceSdRemote(1, 0x10000a0, 0, 0x3010);
	sceSdRemote(1, 0x10000b0, 0, 0xcc);
	sceSdRemote(1, 0x1000040, top);
	return 0;
}

/* Like midiInitSE, with the VB and HD blocks already in EE memory. */
int midiInitSEPtr(const char *vbptr, unsigned int vbsize, const void *hdptr, int hdsize)
{
	SdMidiParam param;
	int top;

	memset(&param, 0, sizeof(param));

	if (gDataTopAddrForCslMidiSe > 0)
		sceSdFreeIopHeap(gDataTopAddrForCslMidiSe);
	top = sceSdAllocIopHeap(0x20000);
	gDataTopAddrForCslMidiSe = top;
	if (top < 0)
		return -1;

	if (midiSendToIop(hdptr, hdsize, top) < 0) {
		printf("HD load error\n");
		return -1;
	}

	param.addr = top;
	param.vagAddr = 0x17d010;
	param.vagSize = soundLoadVagPtr(vbptr, vbsize, 0x17d010);
	sceSdRemote(1, 0x1009050, 1, &param);
	sceSdRemote(1, 0x10000a0, 1, 0x840);
	sceSdRemote(1, 0x10000b0, 1, 0x100);

	g_seCtx.extmod = 0;
	g_seCtx.callBack = 0;
	g_seCtx.conf = 0;
	g_seCtx.buffGrpNum = 2;
	g_seCtx.buffGrp = g_seBuffGrp;
	g_seBuffGrp[0].buffNum = 0;
	g_seBuffGrp[0].buffCtx = 0;
	g_seBuffGrp[1].buffNum = 1;
	g_seBuffGrp[1].buffCtx = &g_seBuffCtx;
	g_seBuffCtx.sema = 0;
	g_seBuffCtx.buff = &g_seStream;
	g_seStream.head.buffsize = sizeof(g_seStream);
	g_seStream.head.validsize = 0;
	if (sceMSIn_Init(&g_seCtx) != 0) {
		printf("sceMSIn_Init Error\n");
		return 1;
	}
	sceMSIn_PutMsg(&g_seCtx, 0, 192);
	return 0;
}

int midiQuitBGM(void)
{
	int addr = gDataTopAddrForCslMidi;

	if (addr > 0)
		sceSdFreeIopHeap(addr);
	gDataTopAddrForCslMidi = -1;
	return 0;
}

int midiQuitSE(void)
{
	int addr = gDataTopAddrForCslMidiSe;

	if (addr > 0)
		sceSdFreeIopHeap(addr);
	gDataTopAddrForCslMidiSe = -1;
	return 0;
}

/* Queues a MIDI note-on (status 0x90) on the SE bank, then sends any pending
   bytes to the IOP (command 0x010000C0) and empties the queue. */
int midiSePlay(int note, int vel)
{
	sceCslMidiStream *e = &g_seStream.head;
	unsigned int n = (note & 0x7f) << 8;
	unsigned int m = ((vel & 0x7f) << 16) | 0x90;

	sceMSIn_PutMsg(&g_seCtx, 0, n | m);
	if (e->validsize != 0) {
		sceSdRemote(1, 0x010000c0, (int)e);
		e->validsize = 0;
	}
	return 0;
}

/* Bank 0 = BGM (16 voices), bank 1 = SE (64 voices); `n` goes in bits 8 and up. */
int midiSetMaxNote(int bank, int n)
{
	int max = 16;

	if ((unsigned int)bank >= 2)
		return -1;
	if (bank == 1)
		max = 64;
	max |= n << 8;
	printf("param = %x\n", max);
	sceSdRemote(1, 0x010000a0, bank, max);
	return 0;
}

/* Command 0x1500 on the SE bank. */
int SetSeVolume(int vol)
{
	sceSdRemote(1, 0x1500, sdVoiceParam9, vol);
	return 0;
}
