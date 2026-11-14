/*
 * sceRm2*: "Remote 2" API of the PS2 SDK (IR remote on /dev/ps2rm20).
 * rm2_read_raw reads one 48-byte packet from the port into a static buffer;
 * sceRm2GetState/SetReqState/GetReqState use its state and request words.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "input/scerm2.h"

typedef struct {
	/* 0x00 */ int opened;
	/* 0x04 */ int portOpen;
	/* 0x08 */ int fd;
} Rm2State;

Rm2State sceRm2Lib_ctx;

int sceRm2Lib_debug = 0;
/* Port packet (48 bytes): 32 bytes of data, validity flag at +32 (read by
 * sceRm2Read), state at +36 and request at +40. */
typedef struct {
	unsigned char data[32];
	/* 0x20 */ int valid;
	/* 0x24 */ int state;  /* sceRm2GetState */
	/* 0x28 */ int reqState;  /* sceRm2SetReqState/GetReqState */
	unsigned char pad[4];
} Rm2Packet;

static Rm2Packet rm2_rawbuf;

/* RPC request (GetModVersion/GetIRFeature/IgnoreSIRCS), 128 bytes; the
 * "client" passed in a0 is the static buffer that follows it. */
typedef struct {
	/* 0x00 */ int fno;
	/* 0x04 */ int result;
	/* 0x08 */ unsigned char byteArg;
	char pad[128 - 9];
} Rm2Req;

static Rm2Req rm2_req;
static char rm2_client[16];

int sceRm2Init(int mode)
{
	(void)mode;
	if (sceRm2Lib_ctx.opened)
		return 1;

	sceRm2Lib_ctx.portOpen = 0;
	sceRm2Lib_ctx.opened = 1;
	return 1;
}

int sceRm2End(void)
{
	if (!sceRm2Lib_ctx.opened)
		return 1;

	if (sceRm2Lib_ctx.portOpen)
		sceRm2PortClose();

	sceRm2Lib_ctx.opened = 0;
	return 1;
}

int sceRm2PortOpen(void *port)
{
	const char *devName = "/dev/ps2rm20";
	int fd;

	(void)port;

	if (sceRm2Lib_ctx.opened && !sceRm2Lib_ctx.portOpen)
		goto do_open;

	if (sceRm2Lib_debug)
		fprintf(stderr, "sceRm2Lib: sceRm2PortOpen: Rm2 is not initialized yet\n");
	return 0;

do_open:
	fd = open(devName, O_RDONLY);
	if (fd < 0) {
		if (sceRm2Lib_debug)
			fprintf(stderr, "sceRm2Lib: sceRm2PortOpen: can't open %s, %s\n", devName, strerror(errno));
		return 0;
	}

	sceRm2Lib_ctx.portOpen = 1;
	sceRm2Lib_ctx.fd = fd;
	return 1;
}

int sceRm2PortClose(void)
{
	if (!sceRm2Lib_ctx.opened || !sceRm2Lib_ctx.portOpen) {
		if (sceRm2Lib_debug)
			fprintf(stderr, "sceRm2Lib: sceRm2PortClose: Port isn't opned\n");
		return 0;
	}

	close(sceRm2Lib_ctx.fd);
	sceRm2Lib_ctx.portOpen = 0;
	return 1;
}

/* Returns 0 if the module is not initialised, the port is closed or the read
 * does not return 48 bytes. */
static Rm2Packet *rm2_read_raw(void)
{
	int n;

	if (!sceRm2Lib_ctx.opened || !sceRm2Lib_ctx.portOpen)
		return 0;
	n = read(sceRm2Lib_ctx.fd, &rm2_rawbuf, 48);
	if (n < 0 && sceRm2Lib_debug)
		fprintf(stderr, "sceRm2Lib: read error, %s\n", strerror(errno));
	if (n != 48) {
		if (sceRm2Lib_debug)
			fprintf(stderr, "sceRm2Lib: read %d bytes while %d bytes was expected\n", n, 48);
		return 0;
	}
	return &rm2_rawbuf;
}

/* The "RPC dispatch" of this port does not talk to the IOP: it serves only
 * fno 8 (GetIRFeature), with an ioctl on the remote port fd and
 * &req->byteArg as argument. Every other fno returns 0; the other parameters
 * (the sceSifCallRpc shape of the callers) are not read. */
static int rm2_rpc_call(void *client, int fno, int mode, Rm2Req *req, int ssize, void *recvbuf, int rsize,
    void *endfunc, void *efarg)
{
	int cmd;

	(void)client;
	(void)fno;
	(void)mode;
	(void)ssize;
	(void)recvbuf;
	(void)rsize;
	(void)endfunc;
	(void)efarg;

	cmd = req->fno;
	switch (cmd) {
	case 8:
		if (!sceRm2Lib_ctx.opened || !sceRm2Lib_ctx.portOpen) {
			if (sceRm2Lib_debug)
				fprintf(stderr, "sceRm2Lib: sceRm2GetIRFeature: failed to get irfeature\n");
			return 0;
		}
		if (ioctl(sceRm2Lib_ctx.fd, 0x40047601, &req->byteArg) != 0) {
			perror("sceRm2GetIRFeature: ");
			return 0;
		}
		return 1;
	}
	return 0;
}

/* Reads a raw IR packet with rm2_read_raw, copies it to a 32-byte local
 * buffer, checks the validity flag at +32 of the source packet (not of the
 * copy), then decodes by type (buf[0]): 12/15/20 are protocol variants with
 * different bit widths, 0 clears the outputs, anything else leaves them
 * unwritten. Returns -1 if the port is closed or the packet is invalid,
 * otherwise buf[0]. */
int sceRm2Read(unsigned short *out1, unsigned char *out2)
{
	int ret;  /* unused, but part of the layout */
	unsigned char buf[32];
	Rm2Packet *raw;

	if (!sceRm2Lib_ctx.portOpen)
		return -1;

	raw = rm2_read_raw();

	memcpy(buf, raw, 32);

	if (raw->valid == 0)
		return -1;

	switch (buf[0]) {
	case 0:
		*out1 = 0;
		*out2 = 0;
		break;
	case 12:
		*out1 = (buf[1] >> 3) & 0xff;
		*out2 = ((buf[1] & 7) << 4) + (buf[2] >> 4);
		break;
	case 15:
		*out1 = buf[1];
		*out2 = buf[2] >> 1;
		break;
	case 20:
		*out1 = (buf[1] << 5) + ((buf[2] >> 3) & 0xff);
		*out2 = ((buf[2] & 7) << 4) + (buf[3] >> 4);
		break;
	}

	return buf[0];
}

int sceRm2GetState(void)
{
	int state; /* unused, but part of the layout */
	Rm2Packet *p;

	if (!sceRm2Lib_ctx.portOpen)
		return 4;

	p = rm2_read_raw();
	if (p->state == 3 && p->reqState == 2)
		return 2;
	return p->state;
}

int sceRm2GetModVersion(void)
{
	int version; /* unused, but part of the layout */
	int r;
	Rm2Req *req = &rm2_req;

	req->fno = 7;
	r = rm2_rpc_call(rm2_client, 0, 0, req, 128, req, 128, 0, 0);
	if (r < 0)
		return 0;
	return req->result;
}

/* sceRm2CodeToStr: key names for the DVD remotes (codes 1114 and 0x93a,
 * table A) and the controller (7002, table B), indexed by the key code byte;
 * "" for unnamed codes. */
/* clang-format off */
static char *rm2_dvd_key_names[128] = {
    "1", "2", "3", "4",
    "5", "6", "7", "8",
    "9", "0", "", "ENTER",
    "DISC EXPLORER", "CHECK", "RETURN", "CLEAR",
    "REV SEARCH", "FWD SEARCH", "", "",
    "", "POWER ON/OFF", "OPEN/CLOSE", "FILE ERASE",
    "VOCAL SELECT", "DISC FILE", "TITLE", "DVDMENU",
    "KARAOKE MODE", "CONTINUOUS MODE", "FUNCTION MEMORY", "PROGRAM MODE",
    "RVS SEARCH/STEP", "FWD SEARCH/STEP", "RVS SCAN/SLOW", "FWD SCAN/SLOW",
    "MULTI CONTROL +", "MULTI CONTROL -", "PREVIOUS INDEX", "NEXT INDEX",
    "TIME", "STILL", "REPEAT A - B", "PLAY MODE",
    "REPEAT MODE", "RESERVE", "POWER ON", "POWER OFF",
    "PREVIOUS", "NEXT", "PLAY", "REV SCAN",
    "FWD SCAN", "SHUFFLE MODE", "RVS x2", "FWD x2",
    "STOP", "PAUSE", "REV STEP", "FWD STEP",
    "DISC", "", "", "DISC CHANGE",
    "DISC1", "DISC2", "DISC3", "DISC4",
    "DISC5", "RESUME", "VOCAL", "1 / ALL DISCS",
    "DNR", "LOAD", "EFFECT DO", "TIME/SCENE",
    "EFFECT MODE", "AUTO PAUSE ON/OFF", "MEGA STORAGE", "DISC MEMO INPUT MODE",
    "L/R STEREO", "DISC/TV", "EASY PLAY", "SET UP",
    "DISPLAY ON/OFF", "VIDEO ON/OFF", "FL ON/OFF", "BOOK MARK",
    "R^2 DIGITAL", "SELECT", "VES", "RGB",
    "REPLAY", "KARAOKE SURROUND", "DVD", "SACD/CD",
    "REV SLOW", "FWD SLOW", "SUBTITLE ON/OFF", "SUBTITLE SELECT",
    "AUDIO SELECT", "ANGLE SELECT", "GUIDE MELODY", "KEYCON ON/OFF",
    "DOUBLIN", "KEY LEVEL NORMAL", "VOCAL SUPPORT", "KEY LEVEL UP",
    "KEY LEVEL DOWN", "", "RVS SPEED 3", "FWD SPEEK 3",
    "", "", "RVS SPEED 1", "FWD SPEED 1",
    "FIRST VERSE", "CHORUS MEDLEY", "PICTURE MEMORY", "KARAOKE PON",
    "NEXT DISC RESERVE", "UP", "DOWN", "LEFT",
    "RIGHT", "", "", "",
};

static char *rm2_pad_key_names[128] = {
    "1", "2", "3", "4",
    "5", "6", "7", "8",
    "9", "0", "", "ENTER",
    "", "", "RETURN", "CLEAR",
    "CH+", "CH-", "", "",
    "", "POWER ON/OFF", "EJECT", "",
    "", "", "TITLE", "DVD MENU",
    "", "REC", "", "PROGRAM MDOE",
    "", "", "", "",
    "", "", "", "",
    "TIME", "", "REPEAT A-B", "",
    "REPEAT MODE", "", "POWER ON", "POWER OFF",
    "PREV", "NEXT", "PLAY", "SCAN-",
    "SCAN+", "SHUFFLE MODE", "", "",
    "STOP", "PAUSE", "", "",
    "", "", "", "",
    "", "", "", "",
    "", "", "", "",
    "", "", "", "",
    "", "", "", "INPUT SELECT",
    "SELECT", "L3", "R3", "START",
    "L_UP", "L_RIGHT", "L_DOWN", "L_LEFT",
    "L2", "R2", "L1", "R1",
    "R_UP", "R_RIGHT", "R_DOWN", "R_LEFT",
    "SLOW-", "SLOW+", "", "SUBTITLE SELECT",
    "AUDIO SELECT", "ANGLE SELECT", "VCR+", "",
    "", "", "", "",
    "", "", "mode select(DVD1)", "mode select(DVD2)",
    "DISPLAY", "RESET", "", "",
    "", "", "", "",
    "", "", "", "",
    "", "", "", "",
};
/* clang-format on */

void sceRm2CodeToStr(unsigned short code, signed char idx, char *buf)
{
	if (code == 1114 || code == 0x93a) {
		if (idx >= 0)
			strcpy(buf, rm2_dvd_key_names[(unsigned char)idx]);
		else
			strcpy(buf, "");
	} else if (code == 7002) {
		if (idx >= 0)
			strcpy(buf, rm2_pad_key_names[(unsigned char)idx]);
		else
			strcpy(buf, "");
	} else {
		strcpy(buf, "NOT_DVDCATEGORY");
	}
}

int sceRm2SetReqState(int state)
{
	Rm2Packet *p = rm2_read_raw();
	p->reqState = state;
	return 1;
}

int sceRm2GetReqState(void)
{
	Rm2Packet *p;

	if (!sceRm2Lib_ctx.portOpen)
		return 1;

	p = rm2_read_raw();
	return p->reqState;
}

/* sceRm2GetState states (4 = port closed) and requests. sceRm2StateIntToStr
 * accepts only state < 4, so "CLOSED" is never returned. */
static char *rm2_state_names[5] = {
	"ERROR",
	"",
	"EXECCMD",
	"STABLE",
	"CLOSED",
};

void sceRm2StateIntToStr(int state, char *buf)
{
	if (state >= 0 && state < 4)
		strcpy(buf, rm2_state_names[state]);
	else
		strcpy(buf, "");
}

static char *rm2_req_names[3] = {
	"COMPLETE",
	"FAILED",
	"BUSY",
};

void sceRm2ReqIntToStr(int state, char *buf)
{
	if (state >= 0 && state < 3)
		strcpy(buf, rm2_req_names[state]);
	else
		strcpy(buf, "");
}

int sceRm2GetIRFeature(sceRm2IRFeature *out)
{
	int r;
	Rm2Req *req = &rm2_req;

	req->fno = 8;
	r = rm2_rpc_call(rm2_client, 0, 0, req, 128, req, 128, 0, 0);
	if (r < 0)
		return 0;

	out->flags = req->byteArg;
	return req->result;
}

int sceRm2IgnoreSIRCS(char flag)
{
	int r;
	Rm2Req *req = &rm2_req;

	req->fno = 9;
	req->byteArg = flag;
	r = rm2_rpc_call(rm2_client, 0, 0, req, 128, req, 128, 0, 0);
	if (r < 0)
		return 0;
	return req->result;
}
