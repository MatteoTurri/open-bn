/*
 * sceSd*: EE side of libsdr, the remote interface to the IOP sound driver
 * through ioctls on /dev/dsp2, with optional printf tracing.
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include "audio/scesd.h"
#include <stdarg.h>

typedef struct {
	/* 0x00 */ int fd;
	/* 0x04 */ int active;
} SdRemoteState;

/* Fallback name "???(%x)" built by sceSdRemoteName. */
static char g_sdUnknownBuf[32];
static SdRemoteState g_sdRemote;

/* Enables the libsdr trace messages. */
int sce_sdr_debug = 0;

typedef struct {
	int id;
	const char *name;
} SdDeviceEntry;

/* Argument of the transfer and heap ioctls (12 bytes). */
typedef struct {
	/* 0x00 */ unsigned int iopaddr;
	/* 0x04 */ int size;
	/* 0x08 */ void *buf;
} SdTransArg;

/* Sends COMMAND_INIT to the IOP module (tail of sceSdRemoteInit and sceSdRemoteInitFd). */
static int SdRemoteHelperInit(void)
{
	SdRemoteState *sd = &g_sdRemote;
	int ret;

	do {
		if (sce_sdr_debug)
			printf("libsdr: COMMAND_INIT\n");
	} while (0);

	ret = ioctl(sd->fd, 0x20005608, 0);
	if (ret < 0) {
		do {
			if (sce_sdr_debug)
				printf("libsdr: COMMAND_INIT failed\n");
		} while (0);

		return -1;
	}

	return 0;
}

int sceSdRemoteInit(void)
{
	SdRemoteState *sd = &g_sdRemote;

	if ((sd->fd = open("/dev/dsp2", O_WRONLY)) < 0)
		return -1;

	sd->active = 1;

	return SdRemoteHelperInit();
}

int sceSdRemoteInitFd(int fd)
{
	SdRemoteState *sd = &g_sdRemote;

	sd->fd = fd;
	sd->active = 0;

	return SdRemoteHelperInit();
}

int sceSdRemoteEnd(void)
{
	SdRemoteState *sd = &g_sdRemote;

	do {
		if (sce_sdr_debug)
			printf("libsdr: COMMAND_END\n");
	} while (0);

	if (ioctl(sd->fd, 0x2000560a, 0) < 0) {
		do {
			if (sce_sdr_debug)
				printf("libsdr: COMMAND_END failed\n");
		} while (0);

		return -1;
	}

	if (sd->active) {
		close(sd->fd);
	}

	sd->fd = -1;

	return 0;
}

/* Variadic dispatcher of libsdr: builds a 128-word packet for the IOP
   module, sends it with one ioctl(0xC0005609) and returns the first word
   (the result). Command 0x1010 copies its reply back to the caller. */

typedef struct {
	/* 0x00 */ int result;  /* written back by the ioctl */
	/* 0x04 */ int cmd;  /* command code (first vararg) */
	/* 0x08 */ int args[126];
} SdRemotePkt;

/* Block passed by command 0x10000C0: `flag` is set to 1 before the block is
   copied into the packet. */
typedef struct {
	/* 0x00 */ char pad_00[0xfc];
	/* 0xFC */ int flag;
} SdRemoteBlk;

int sceSdRemote(int type, ...)
{
	SdRemoteState *sd = &g_sdRemote;
	unsigned int i;
	SdRemoteBlk *p;
	va_list ap;
	SdRemotePkt pkt;

	if (type != 1)
		return -1;

	memset(&pkt, 0, 512);

	va_start(ap, type);
	pkt.cmd = va_arg(ap, int);

	switch (pkt.cmd) {
	case 0x1000:
		pkt.args[0] = va_arg(ap, int);
		p = va_arg(ap, SdRemoteBlk *);
		memcpy((char *)&pkt + 12, p, 20);
		break;
	case 0x1010:
		p = va_arg(ap, SdRemoteBlk *);
		break;
	case 0x1009050:
		pkt.args[0] = va_arg(ap, int);
		p = va_arg(ap, SdRemoteBlk *);
		memcpy((char *)&pkt + 12, p, 64);
		break;
	case 0x1000070:
		p = va_arg(ap, SdRemoteBlk *);
		memcpy((char *)&pkt + 12, p, 64);
		break;
	case 0x10000c0:
		p = va_arg(ap, SdRemoteBlk *);
		p->flag = 1;
		memcpy((char *)&pkt + 8, p, 256);
		break;
	default:
		for (i = 0; i < 126; i++)
			pkt.args[i] = va_arg(ap, int);
		break;
	}

	do {
		if (sce_sdr_debug)
			printf("libsdr: %s(%x, %x, %x, %x...) = ", sceSdRemoteName(pkt.cmd), pkt.args[0], pkt.args[1],
			    pkt.args[2], pkt.args[3]);
	} while (0);

	if (ioctl(sd->fd, 0xC0005609, &pkt) < 0) {
		do {
			if (sce_sdr_debug)
				printf("ERROR\n");
		} while (0);
		return -1;
	}

	do {
		if (sce_sdr_debug)
			printf("%d\n", pkt.result);
	} while (0);

	switch (pkt.cmd) {
	case 0x1010:
		memcpy(p, (char *)&pkt + 8, 20);
		break;
	}

	return pkt.result;
}

/* Copies `size` bytes from EE memory to the IOP heap. */
int sceSdTransToIOP(void *buf, unsigned int iopaddr, int size)
{
	SdRemoteState *sd = &g_sdRemote;
	SdTransArg arg;

	do {
		if (sce_sdr_debug)
			printf("libsdr: EE -> IOP %x, %d bytes \n", iopaddr, size);
	} while (0);

	memset(&arg, 0, sizeof(arg));
	arg.iopaddr = iopaddr;
	arg.size = size;
	arg.buf = buf;

	return ioctl(sd->fd, 0x800c5607, &arg);
}

/* Same as sceSdTransToIOP, but to SPU2 memory and without tracing. */
int sceSdTransToSPU2(void *buf, unsigned int spuaddr, int size)
{
	SdRemoteState *sd = &g_sdRemote;
	SdTransArg arg;

	memset(&arg, 0, sizeof(arg));
	arg.iopaddr = spuaddr;
	arg.size = size;
	arg.buf = buf;

	return ioctl(sd->fd, 0x800c5601, &arg);
}

int sceSdAllocIopHeap(int size)
{
	SdRemoteState *sd = &g_sdRemote;
	SdTransArg arg;

	arg.size = size;

	do {
		if (sce_sdr_debug)
			printf("libsdr: AllocIopHeap(%d bytes)...", size);
	} while (0);

	if (ioctl(sd->fd, 0xc00c5604, &arg) < 0) {
		do {
			if (sce_sdr_debug)
				printf("failed\n");
		} while (0);

		return -1;
	}

	do {
		if (sce_sdr_debug)
			printf("succeeded, addr=%x\n", arg.iopaddr);
	} while (0);

	return (int)arg.iopaddr;
}

int sceSdFreeIopHeap(int addr)
{
	SdRemoteState *sd = &g_sdRemote;

	return ioctl(sd->fd, 0x20005605, (void *)addr);
}

char *sceSdRemoteName(int id)
{
	unsigned int i;
	/* command id -> name */
	SdDeviceEntry table[] = { { 0x1200, "SdInit" }, { 0x1300, "SdSetParam" }, { 0x1470, "SdGetParam" },
		{ 0x1290, "SdSetSwitch" }, { 0x1110, "SdGetSwitch" }, { 0x1360, "SdSetAddr" }, { 0x1330, "SdGetAddr" },
		{ 0x1280, "SdSetCoreAttr" }, { 0x1150, "SdGetCoreAttr" }, { 0x1460, "SdNote2Pitch" },
		{ 0x1240, "SdPitch2Note" }, { 0x1160, "SdProcBatch" }, { 0x1220, "SdProcBatchEx" },
		{ 0x1490, "SdVoiceTrans" }, { 0x1100, "SdBlockTrans" }, { 0x1520, "SdVoiceTransStatus" },
		{ 0x1080, "SdBlockTransStatus" }, { 0x1000, "SdSetEffectAttr" }, { 0x1010, "SdGetEffectAttr" },
		{ 0x1400, "SdClearEffectWorkArea" }, { 0x1480, "SdSetTransIntrHandler" },
		{ 0x1260, "SdSetSpu2IntrHandler" }, { 0x1430, "SdChangeThreadPriority" }, { 0x1440, "StIntrStart" },
		{ 0x1210, "StInit" }, { 0x1020, "StQuit" }, { 0x1420, "StCalledVSync" },
		{ 0x1090, "StSetTransferCallback" }, { 0x1030, "StVabOpen" }, { 0x1040, "StVabOpenFakeBody" },
		{ 0x1550, "StVabOpenCompleted" }, { 0x1380, "StVabClose" }, { 0x1050, "StBgmOpen" },
		{ 0x1180, "StSetTickMode" }, { 0x1540, "StBgmClose" }, { 0x1120, "StSetReverbType" },
		{ 0x1190, "StSetReverbDepth" }, { 0x1560, "StSetReverbDelaytime" }, { 0x1320, "StSetReverbFeedback" },
		{ 0x1450, "StGetSlotStatus" }, { 0x1510, "StSetSbClear" }, { 0x1230, "StSetMasterVol" },
		{ 0x1170, "StSetBgmVol" }, { 0x1390, "StBgmPlay" }, { 0x1340, "StBgmStop" },
		{ 0x1130, "StSetBgmTempo" }, { 0x1410, "StGetBgmTempo" }, { 0x1140, "StGetBgmStatus" },
		{ 0x1310, "StGetBgmChStatus" }, { 0x1060, "StDmaWrite" }, { 0x1070, "StDmaRead" },
		{ 0x1250, "StSetThIntrStart" }, { 0x1350, "StSetThIntrEnd" }, { 0x1530, "StSePlay" },
		{ 0x1500, "StSetSeVol" }, { 0x1570, "StSetEffectEnable" }, { 0x1580, "StSetSpdifMode" },
		{ 0x1000000, "MIDI_START" }, { 0x1008010, "MIDI_INIT" }, { 0x1000020, "MIDI_STOP" },
		{ 0x1000030, "MIDI_SEEK" }, { 0x1000040, "MIDI_SETSQ" }, { 0x1009050, "MIDI_SETHD" },
		{ 0x1009060, "MIDI_TRANSBDPACKET" }, { 0x1000070, "MIDI_TRANSBD" }, { 0x1008090, "MIDI_GETSTATUS" },
		{ 0x10000a0, "MIDI_SETATTR" }, { 0x10000b0, "MIDI_SETVOL" }, { 0x10000c0, "MIDI_SENDSQSTREAM" },
		{ 0x10000d0, "MIDI_EXIT" } };

	i = 0;
	while (i < 70) {
		if (table[i].id == id)
			return (char *)table[i].name;
		i++;
	}

	sprintf(g_sdUnknownBuf, "???(%x)", id);

	return g_sdUnknownBuf;
}
