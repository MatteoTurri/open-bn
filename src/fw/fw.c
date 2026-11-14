/*
 * fw graphics framework (Sony "sample program framework" over libgraph): the
 * table of registered GS buffers with their five environments, the environment
 * stacks and packet setters, and the libgraph sceGsSetDef* helpers they call.
 */

#include <string.h>
#include <sys/ioctl.h>
#include <linux/ps2/gs.h>
#include <linux/ps2/dev.h>
#include <linux/ps2/ee.h>
#include "ps2sync.h"
#include "fw.h"
#include "fw_libgraph.h"
#include "fw_gsdev.h"
#include "fw_pk.h"
#include "dmem.h"

/* ---- Framework environments and buffer table ---- */
typedef unsigned long long fwGsReg64;

/* First two qwords of an environment packet. The DMAtag is not sent: the
 * packet goes to the DMA chain from `vif` on, with the tag's QWC. */
typedef struct {
	/* 0x00 */ union {
		unsigned int tag;      /* DMAtag: QWC in bits 0-15, ID in bits 28-30 */
		unsigned short qwc;
	} dma;
	/* 0x04 */ unsigned int dmaAddr;
	/* 0x08 */ unsigned int tagVif[2];
	/* 0x10 */ unsigned int vif[4];  /* NOP, NOP, FLUSH, DIRECT */
} fwGsPacketHead;  /* size = 0x20 */

/* Draw environment: the eight registers of sceGsSetDefDrawEnv and DIMX. */
struct fwGsDrawPacket {
	/* 0x00 */ fwGsPacketHead head;
	/* 0x20 */ sceGsGifTag giftag;
	/* 0x30 */ ps2_gs_drawenv1 draw;
	/* 0xB0 */ fwGsReg64 dimx;      /* dither matrix */
	/* 0xB8 */ fwGsReg64 dimxAddr;
};  /* size = 0xC0 */

/* Clear environment: the six registers of sceGsSetDefClear. */
struct fwGsClearPacket {
	/* 0x00 */ fwGsPacketHead head;
	/* 0x20 */ sceGsGifTag giftag;
	/* 0x30 */ ps2_gs_clear clear;
};  /* size = 0x90 */

/* Texture environment: TEST, the alpha registers and the texture registers. */
struct fwGsTexPacket {
	/* 0x00 */ fwGsPacketHead head;
	/* 0x20 */ unsigned int giftag[4];
	/* 0x30 */ fwGsReg64 test;
	/* 0x38 */ fwGsReg64 testAddr;
	/* 0x40 */ ps2_gs_alphaenv alpha;
	/* 0x80 */ ps2_gs_texenv tex;
};  /* size = 0xC0 */

/* Host-to-GS transfer: the LoadImage registers, then a DMAtag ref to the
 * pixel data and a DMAtag ret. */
struct fwGsTransPacket {
	/* 0x00 */ fwGsPacketHead head;
	/* 0x20 */ sceGsLoadImage1 load;
	/* 0x80 */ unsigned int imageTag[4];  /* tag, pixel data address, VIF NOP x2 */
	/* 0x90 */ unsigned int imageVif[4];  /* NOP, NOP, NOP, DIRECT */
	/* 0xA0 */ unsigned int retTag[4];
};  /* size = 0xB0 */

/* Display environment (the crtBlend environment of a buffer), sent register
 * by register through the GS driver. */
struct sceGsDispEnv {
	/* 0x00 */ fwGsReg64 pmode;
	/* 0x08 */ fwGsReg64 smode2;
	/* 0x10 */ fwGsReg64 dispfb;
	/* 0x18 */ fwGsReg64 display;
	/* 0x20 */ fwGsReg64 bgcolor;
};  /* size = 0x28 */

/* Environment stack (trans/draw/clear/tex): count, 32 handles and 32
 * environment pointers. */
typedef struct {
	unsigned int count;
	int handle[32];
	fwGsPacketHead *env[32] __attribute__((aligned(16)));
} fwGsEnvStack;

/* Registered buffers (draw/disp), indexed by handle; entries 32 and 33 are the
 * draw and disp kick base buffers. 0xffff = invalid handle. */
typedef struct {
	/* 0x00 */ fwGsDrawPacket *drawEnv;
	/* 0x04 */ fwGsClearPacket *clearEnv;
	/* 0x08 */ fwGsTexPacket *texEnv;
	/* 0x0C */ sceGsDispEnv *crtBlendEnv;
	/* 0x10 */ fwGsTransPacket *transEnv;
	/* 0x14 */ short addr;   /* frame buffer base pointer */
	/* 0x16 */ short unk_16;
	/* 0x18 */ short size;   /* GS pages of the frame buffer */
	/* 0x1A */ short zsize;  /* GS pages of the Z buffer */
	/* 0x1C */ short psm;
	/* 0x1E */ short zpsm;
	/* 0x20 */ short width;
	/* 0x22 */ short height;
	/* 0x24 */ short ztest;
	/* 0x26 */ short clear;  /* nonzero: the clear packet draws a full-buffer clear */
} fwGsBufEnvEntry;  /* size = 0x28 */

/* Register list for the GS driver ioctl of fwGsPutDispEnvCB: 4 {value,
 * register} pairs plus the {count, ptr} header. */
static struct ps2_gssreg g_fwGsCBPacket[4];
static struct {
	int num;
	void *ptr;
} g_fwGsCBHeader;
/* One flag per table entry: the allocator only uses 0..31, entries 32/33 are
 * the kick base buffers registered by fwGsInitBufEnv. */
static int g_fwGsBufEnvAlloc[34];
/* The five environment pools carved out of one DMem block by fwGsInitBufEnv,
 * 34 entries each; the draw pool is the start of the block. */
static fwGsDrawPacket *g_fwGsDrawPool;
static fwGsClearPacket *g_fwGsClearPool;          /* block + 6528 */
static fwGsTexPacket *g_fwGsTexPool;              /* block + 11424 */
static sceGsDispEnv *g_fwGsDispPool;              /* block + 23936 */
static fwGsTransPacket *g_fwGsTransPool;          /* block + 17952 */
static fwGsBufEnvEntry g_fwGsBufEnv[34];
static fwGsEnvStack g_fwGsDrawStack;
static fwGsEnvStack g_fwGsClearStack;
static fwGsEnvStack g_fwGsTexStack;
static fwGsEnvStack g_fwGsTransStack;

/* Environment blocks copied by struct assignment in
 * fwGsSetKickBase{Draw,Disp}Buf. */
typedef struct {
	int w[48];
} fwGsBlkDraw;   /* 192 B */
typedef struct {
	int w[36];
} fwGsBlkClear;  /* 144 B */
typedef struct {
	int w[10];
} fwGsBlkBlend;  /*  40 B */
typedef struct {
	int w[44];
} fwGsBlkTrans;  /* 176 B */

static int fwGs_setBufEnvImpl(int idx, int addr, short psm, short w, short h, short ztest, short zpsm, short clear);
static int fwGs_putGifReg(int code, long long value);

/* Clears the allocation flags, carve the five environment pools out of one
 * 25312-byte DMem block and registers the default draw (32) and disp (33)
 * buffers. */
int fwGsInitBufEnv(void)
{
	int i;
	char *p;

	for (i = 31; i >= 0; i--)
		g_fwGsBufEnvAlloc[i] = 0;

	p = DMemAlloc(25312);
	g_fwGsClearPool = (fwGsClearPacket *)(p + 6528);
	g_fwGsTexPool = (fwGsTexPacket *)(p + 11424);
	g_fwGsDrawPool = (fwGsDrawPacket *)p;
	g_fwGsTransPool = (fwGsTransPacket *)(p + 17952);
	g_fwGsDispPool = (sceGsDispEnv *)(p + 23936);

	fwGs_setBufEnvImpl(33, 0, 0, 256, 256, 0, 10, 1);
	fwGs_setBufEnvImpl(32, 0, 0, 256, 256, 0, 10, 1);
}

/* Frees the environment pools, clears the buffer table and the allocation flags
 * and resets the four environment stacks. Always returns 0. */
int fwGsReleaseBufEnv(void)
{
	int i;

	DMemFree(g_fwGsDrawPool);
	memset(g_fwGsBufEnv, 0, 1360);

	for (i = 31; i >= 0; i--)
		g_fwGsBufEnvAlloc[i] = 0;

	fwGsResetTexStack();
	fwGsResetTransStack();
	fwGsResetClearStack();
	fwGsResetDrawStack();
	return 0;
}

/* Writes one GS register (code, 64-bit value) through the GS driver ioctl. */
static int fwGs_putGifReg(int code, long long value)
{
	struct ps2_gssreg arg;

	arg.val = value;
	arg.reg = code;
	return ioctl(g_fd_gs, PS2IOC_SGSSREG, &arg);
}

/* Sends PMODE, DISPFB2, DISPLAY2 and BGCOLOR (codes 0/9/10/14) from the
 * crtBlend environment of the buffer, if it has one. */
void fwGsPutDispEnv(int handle)
{
	sceGsDispEnv *env;

	if (g_fwGsBufEnv[handle].crtBlendEnv) {
		env = g_fwGsBufEnv[handle].crtBlendEnv;
		fwGs_putGifReg(0, env->pmode);
		env = g_fwGsBufEnv[handle].crtBlendEnv;
		fwGs_putGifReg(9, env->dispfb);
		env = g_fwGsBufEnv[handle].crtBlendEnv;
		fwGs_putGifReg(10, env->display);
		env = g_fwGsBufEnv[handle].crtBlendEnv;
		fwGs_putGifReg(14, env->bgcolor);
	}
}

/* Sends PMODE (with bit 0 set), DISPFB1, DISPLAY1 and BGCOLOR (codes 0/7/8/14)
 * from the crtBlend environment of the buffer, if it has one. */
void fwGsPutCrtBlendEnv(int handle)
{
	sceGsDispEnv *env = g_fwGsBufEnv[handle].crtBlendEnv;
	if (env) {
		fwGs_putGifReg(0, env->pmode | 1);
		env = g_fwGsBufEnv[handle].crtBlendEnv;
		fwGs_putGifReg(7, env->dispfb);
		env = g_fwGsBufEnv[handle].crtBlendEnv;
		fwGs_putGifReg(8, env->display);
		env = g_fwGsBufEnv[handle].crtBlendEnv;
		fwGs_putGifReg(14, env->bgcolor);
	}
}

/* Same registers as fwGsPutDispEnv, sent in one ioctl as an A+D packet. */
void fwGsPutDispEnvCB(int handle)
{
	sceGsDispEnv *env = g_fwGsBufEnv[handle].crtBlendEnv;
	if (env) {
		g_fwGsCBPacket[0].val = env->pmode;
		g_fwGsCBPacket[0].reg = 0;
		g_fwGsCBPacket[1].val = env->dispfb;
		g_fwGsCBPacket[1].reg = 9;
		g_fwGsCBPacket[2].val = env->display;
		g_fwGsCBPacket[2].reg = 10;
		g_fwGsCBPacket[3].val = env->bgcolor;
		g_fwGsCBPacket[3].reg = 14;
		g_fwGsCBHeader.num = 4;
		g_fwGsCBHeader.ptr = g_fwGsCBPacket;
		ioctl(g_fd_gs, 0x8008ee35, &g_fwGsCBHeader);
	}
}

/* Reads the buffer back from the GS into host memory at `buf` (rounded up to
 * 16). Returns -1 if the driver transferred more than the buffer size (w*h*4
 * bytes). */
int _fwGsStoreImage(int handle, int buf)
{
	struct ps2_image img;
	fwGsBufEnvEntry *e;
	short w, h;
	int ret;

	img.ptr = (void *)((buf + 15) >> 4 << 4);
	e = g_fwGsBufEnv + handle;
	img.fbp = e->addr << 5;
	w = e->width;
	img.fbw = ((w + 63) >> 6) & 0x3f;
	img.psm = e->psm;
	img.x = 0;
	img.y = 0;
	h = e->height;
	img.w = w;
	img.h = h;
	ret = ps2_gs_store_image(&img);
	return (w * h * 4 < ret) ? -1 : ret;
}

/* The last three arguments are unused. In video mode 1 the half-pixel offset
 * of the current field is applied to the buffer draw environment and stored in
 * gs_env.oddEven; otherwise gs_env.oddEven is cleared. */
void fwGsSetHalfOffset(int handle, int unused1, int unused2, int unused3)
{
	int flag;
	fwGsDrawPacket *env;

	if (gs_env.ffMode == FW_FF_FRAME) {
		if (fwPkCheckBufMode() == 0) {
			flag = odev;
		} else {
			flag = (unsigned)odev < 1;
		}
		env = g_fwGsBufEnv[handle].drawEnv;
		gs_env.oddEven = flag;
		ps2_gs_set_half_offset((ps2_gs_drawenv1 *)&env->draw, flag);
	} else {
		gs_env.oddEven = 0;
	}
}

/* Makes `handle` the draw (32) / disp (33) kick base buffer: copies its five
 * environments and its fields into the reserved entry. */
int fwGsSetKickBaseDrawBuf(int handle)
{
	if (handle == 32)
		return -1;

	*(fwGsBlkDraw *)g_fwGsBufEnv[32].drawEnv = *(fwGsBlkDraw *)g_fwGsBufEnv[handle].drawEnv;
	*(fwGsBlkClear *)g_fwGsBufEnv[32].clearEnv = *(fwGsBlkClear *)g_fwGsBufEnv[handle].clearEnv;
	*(fwGsBlkBlend *)g_fwGsBufEnv[32].crtBlendEnv = *(fwGsBlkBlend *)g_fwGsBufEnv[handle].crtBlendEnv;
	*(fwGsBlkClear *)g_fwGsBufEnv[32].texEnv = *(fwGsBlkClear *)g_fwGsBufEnv[handle].texEnv;
	*(fwGsBlkTrans *)g_fwGsBufEnv[32].transEnv = *(fwGsBlkTrans *)g_fwGsBufEnv[handle].transEnv;

	g_fwGsBufEnv[32].addr = g_fwGsBufEnv[handle].addr;
	g_fwGsBufEnv[32].unk_16 = g_fwGsBufEnv[handle].unk_16;
	g_fwGsBufEnv[32].size = g_fwGsBufEnv[handle].size;
	g_fwGsBufEnv[32].zsize = g_fwGsBufEnv[handle].zsize;
	g_fwGsBufEnv[32].psm = g_fwGsBufEnv[handle].psm;
	g_fwGsBufEnv[32].zpsm = g_fwGsBufEnv[handle].zpsm;
	g_fwGsBufEnv[32].width = g_fwGsBufEnv[handle].width;
	g_fwGsBufEnv[32].height = g_fwGsBufEnv[handle].height;
	g_fwGsBufEnv[32].ztest = g_fwGsBufEnv[handle].ztest;
	g_fwGsBufEnv[32].clear = g_fwGsBufEnv[handle].clear;
	return handle;
}

int fwGsSetKickBaseDispBuf(int handle)
{
	if (handle == 33)
		return -1;

	*(fwGsBlkDraw *)g_fwGsBufEnv[33].drawEnv = *(fwGsBlkDraw *)g_fwGsBufEnv[handle].drawEnv;
	*(fwGsBlkClear *)g_fwGsBufEnv[33].clearEnv = *(fwGsBlkClear *)g_fwGsBufEnv[handle].clearEnv;
	*(fwGsBlkBlend *)g_fwGsBufEnv[33].crtBlendEnv = *(fwGsBlkBlend *)g_fwGsBufEnv[handle].crtBlendEnv;
	*(fwGsBlkClear *)g_fwGsBufEnv[33].texEnv = *(fwGsBlkClear *)g_fwGsBufEnv[handle].texEnv;
	*(fwGsBlkTrans *)g_fwGsBufEnv[33].transEnv = *(fwGsBlkTrans *)g_fwGsBufEnv[handle].transEnv;

	g_fwGsBufEnv[33].addr = g_fwGsBufEnv[handle].addr;
	g_fwGsBufEnv[33].unk_16 = g_fwGsBufEnv[handle].unk_16;
	g_fwGsBufEnv[33].size = g_fwGsBufEnv[handle].size;
	g_fwGsBufEnv[33].zsize = g_fwGsBufEnv[handle].zsize;
	g_fwGsBufEnv[33].psm = g_fwGsBufEnv[handle].psm;
	g_fwGsBufEnv[33].zpsm = g_fwGsBufEnv[handle].zpsm;
	g_fwGsBufEnv[33].width = g_fwGsBufEnv[handle].width;
	g_fwGsBufEnv[33].height = g_fwGsBufEnv[handle].height;
	g_fwGsBufEnv[33].ztest = g_fwGsBufEnv[handle].ztest;
	g_fwGsBufEnv[33].clear = g_fwGsBufEnv[handle].clear;
	return handle;
}

/* Fixed handles of the kick base buffers. */
int fwGsGetKickBaseDrawBuf(void)
{
	return 32;
}

int fwGsGetKickBaseDispBuf(void)
{
	return 33;
}

/* Copies FRAME / ZBUF of the draw environment of the current buffer to `out`. */
int fwGsGetValueFRAME(void *out)
{
	int disp, draw, idx;
	fwGsDrawPacket *env;

	if (fwGsCheckRegDBuf(&disp, &draw) == 1)
		idx = draw;
	else
		idx = disp;

	env = g_fwGsBufEnv[idx].drawEnv;
	*(long long *)out = *(long long *)&env->draw.frame1;
	return 1;
}

int fwGsGetValueZBUF(void *out)
{
	int disp, draw, idx;
	fwGsDrawPacket *env;

	if (fwGsCheckRegDBuf(&disp, &draw) == 1)
		idx = draw;
	else
		idx = disp;

	env = g_fwGsBufEnv[idx].drawEnv;
	*(long long *)out = *(long long *)&env->draw.zbuf1;
	return 1;
}

/* GS pages taken by a w x h buffer in format psm (pages of 64 lines when bit 1
 * of psm is set, else 32); same as sceGszbufaddr but returning int. */
int _getBufSize(int psm, short w, short h)
{
	int wpages = (w + 63) / 64;
	int hpages;

	if (psm & 2)
		hpages = (h + 63) / 64;
	else
		hpages = (h + 31) / 32;

	return wpages * hpages;
}

/* Sets the address of the pixel data sent by the buffer trans environment. */
void fwGsSetTransData(int handle, int value)
{
	fwGsTransPacket *e = g_fwGsBufEnv[handle].transEnv;
	e->imageTag[1] = value;
}

/* Builds a host-to-GS image transfer packet in `env`. The size in qwords
 * depends on psm; above 32767 qwords nothing is written. The GS part comes from
 * sceGsSetDefLoadImage, then the DMA tags and VIF codes around it are written
 * from the NLOOP counts of its two GIFtags. */
void fwGsSetTransEnv(fwGsTransPacket *env, short tbp, short bw, short psm, short x, short y, short w, short h)
{
	int qwc = 0;

	switch (psm) {
	case 0:
	case 48:
		qwc = (w * h) >> 2;
		break;
	case 1:
	case 49:
		qwc = (w * h * 3) >> 4;
		break;
	case 2:
	case 10:
	case 50:
	case 58:
		qwc = (w * h) >> 3;
		break;
	case 19:
	case 27:
		qwc = (w * h) >> 4;
		break;
	case 20:
	case 36:
	case 44:
		qwc = (w * h) >> 5;
		break;
	}

	if (qwc > 32767)
		return;

	sceGsSetDefLoadImage(&env->load, tbp << 5, bw / 64, psm, x, y, w, h);

	/* DMAtag cnt (qwc of the first A+D packet + 3) */
	qwc = env->load.giftag0.NLOOP;
	env->head.dma.tag = (qwc + 3) | (PS2_DMATAG_CNT << 28);
	env->head.tagVif[1] = 0;
	env->head.tagVif[0] = 0;
	env->head.dmaAddr = 0;
	env->head.vif[1] = 0;
	env->head.vif[0] = 0;
	env->head.vif[2] = PS2_VIF_SET_CODE(0, 0, PS2_VIF_FLUSH, 0);
	env->head.vif[3] = PS2_VIF_SET_CODE(qwc + 2, 0, PS2_VIF_DIRECT, 0);
	/* DMAtag ref to the pixels (qwc = NLOOP of the IMAGE GIFtag + 1) */
	qwc = env->load.giftag1.NLOOP;
	env->imageTag[0] = (qwc + 1) | (PS2_DMATAG_REF << 28);
	env->imageTag[3] = 0;
	env->imageTag[2] = 0;
	env->imageTag[1] = 0;
	env->imageVif[2] = 0;
	env->imageVif[1] = 0;
	env->imageVif[0] = 0;
	env->imageVif[3] = PS2_VIF_SET_CODE(qwc, 0, PS2_VIF_DIRECT, 0);
	/* DMAtag ret */
	env->retTag[0] = PS2_DMATAG_RET << 28;
	env->retTag[3] = 0;
	env->retTag[2] = 0;
	env->retTag[1] = 0;
}

void fwGsSetCustomDrawEnv(int handle, int fbp, short psm, short w, short h, short ztest, short zpsm)
{
	fwGsDrawPacket *env = g_fwGsBufEnv[handle].drawEnv;
	fwGsSetDrawEnv(env, fbp, psm, w, h, ztest, zpsm);
}

/* Builds a draw environment packet: sceGsSetDefDrawEnv, then FRAME_1 and
 * ZBUF_1 (ZBP = fbp + 2 * buffer size) are rewritten and the GIF register list
 * filled in. */
void fwGsSetDrawEnv(fwGsDrawPacket *env, int fbp, short psm, short w, short h, short ztest, short zpsm)
{
	int nloop = 9;     /* A+D registers in the packet: 8 from sceGsSetDefDrawEnv + DIMX */
	int size = _getBufSize(psm, w, h);

	sceGsSetDefDrawEnv(&env->draw, psm, w, h, ztest, zpsm);

	*(fwGsReg64 *)&env->draw.frame1 = (fwGsReg64)(unsigned)(fbp & 0x1ff)
	    | ((fwGsReg64)(((w + 63) >> 6) & 0x3f) << 16) | ((fwGsReg64)(psm & 0xf) << 24);
	*(fwGsReg64 *)&env->draw.zbuf1 =
	    (fwGsReg64)(unsigned)(fbp + 2 * size) | ((fwGsReg64)(zpsm & 0xf) << 24) | 0x100000000ULL;
	*(fwGsReg64 *)&env->draw.test1 = 0x20000;
	SCEGS_CLEAR_TAG(&env->giftag);
	env->giftag.NREG = 1;
	env->giftag.REGS0 = PS2_GIFTAG_REGS_AD;
	env->head.vif[2] = PS2_VIF_SET_CODE(0, 0, PS2_VIF_FLUSH, 0);
	env->head.tagVif[1] = 0;
	env->head.tagVif[0] = 0;
	env->head.dmaAddr = 0;
	env->head.vif[1] = 0;
	env->head.vif[0] = 0;
	env->head.dma.tag = (PS2_DMATAG_RET << 28) | 11;
	env->head.vif[3] = PS2_VIF_SET_CODE(10, 0, PS2_VIF_DIRECT, 0);
	env->giftag.NLOOP = nloop;
	env->giftag.EOP = 1;
	env->dimxAddr = 68;
	env->dimx = 0x6071243571603524ULL;
}

void fwGsSetCustomClearEnv(int handle, short unused, short w, short h, short ztest, short clear)
{
	fwGsClearPacket *env = g_fwGsBufEnv[handle].clearEnv;
	fwGsSetClearEnv(env, unused, w, h, ztest, clear);
}

/* Builds a clear packet: DMAtag ret, VIF FLUSH/DIRECT, GIFtag with 6 A+D
 * registers, then the ps2_gs_clear. TEST_1 and RGBAQ are rewritten after
 * sceGsSetDefClear. */
void fwGsSetClearEnv(fwGsClearPacket *env, int unused, short w, short h, short ztest, short clear)
{
	if (clear != 0) {
		int x = 2048 - (w >> 1);
		int y = 2048 - (h >> 1);
		sceGsSetDefClear(&env->clear, ztest, x, y, w, h, 0, 0, 0, 0, 0);
	}

	*(fwGsReg64 *)&env->clear.testb = 0x20000;
	SCEGS_CLEAR_TAG(&env->giftag);
	env->giftag.NLOOP = 6;
	env->giftag.EOP = 1;
	env->giftag.NREG = 1;
	env->giftag.REGS0 = PS2_GIFTAG_REGS_AD;
	*(fwGsReg64 *)&env->clear.rgbaq = PS2_GS_SETREG_RGBAQ(0, 0, 0, 0x80, 0x3f800000);
	env->head.tagVif[1] = 0;
	env->head.tagVif[0] = 0;
	env->head.dmaAddr = 0;
	env->head.dma.tag = (PS2_DMATAG_RET << 28) | 8;
	env->head.vif[3] = PS2_VIF_SET_CODE(7, 0, PS2_VIF_DIRECT, 0);
	env->head.vif[2] = PS2_VIF_SET_CODE(0, 0, PS2_VIF_FLUSH, 0);
	env->head.vif[1] = 0;
	env->head.vif[0] = 0;
}

void fwGsSetCustomTexEnv(int handle, int tbp, short psm, short w, short h, short wrap)
{
	fwGsTexPacket *env = g_fwGsBufEnv[handle].texEnv;
	fwGsSetTexEnv(env, tbp, psm, w, h, wrap);
}

/* Builds a texture environment packet: w and h are rounded up to powers of two
 * for sceGsSetDefTexEnv, then the alpha registers (sceGsSetDefAlphaEnv) and
 * TEST. */
void fwGsSetTexEnv(fwGsTexPacket *env, int tbp, short psm, short w, short h, short wrap)
{
	int lw = 0, lh = 0;
	int nw = 1, nh = 1;

	while (nw < w) {
		nw <<= 1;
		lw++;
	}
	while (nh < h) {
		nh <<= 1;
		lh++;
	}

	sceGsSetDefTexEnv(&env->tex, 1, tbp << 5, w / 64, psm, lw, lh, 0, 0, 0, 0, 1);

	env->giftag[3] = 0;
	env->giftag[0] = 0x8009;
	env->giftag[1] = 0x10000000;
	env->giftag[2] = 14;

	sceGsSetDefAlphaEnv(&env->alpha, 0);

	*(fwGsReg64 *)&env->alpha.alpha1 = 68;
	env->test = wrap ? (((fwGsReg64)(wrap & 3) << 17) | 0x10000) : 0x20000;
	*(int *)&env->testAddr = 71;
	env->head.tagVif[1] = 0;
	env->head.tagVif[0] = 0;
	env->head.dmaAddr = 0;
	env->head.dma.tag = (PS2_DMATAG_RET << 28) | 11;
	env->head.vif[3] = PS2_VIF_SET_CODE(10, 0, PS2_VIF_DIRECT, 0);
	env->head.vif[2] = PS2_VIF_SET_CODE(0, 0, PS2_VIF_FLUSH, 0);
	env->head.vif[1] = 0;
	env->head.vif[0] = 0;
}

/* Fills the buffer crtBlend environment with fwGsSetDispEnv, then rewrites
 * DISPLAY (offset 24) with custom 1-based magnification factors magh/magv.
 * Per video mode (gs_env.videoMode); any other mode writes nothing:
 *   mode 2 : DX +636, DY +50 interlaced (DH doubled in FRAME mode) / +25
 *   mode 3 : DX +656, DY +72 interlaced / +36
 *   mode 59: DX +580, DY = (dy * (mv + 1) + 34) & 0xfff
 *   mode 82: DX +302, DY = (dy * (mv + 1) + 24) & 0xfff
 */
void fwGsSetCustomDispEnv(int handle, int fbp, short psm, int w, int h, int dx, int dy, int magh, int magv)
{
	int mh, mv, vt, dwt;

	fwGsSetDispEnv(g_fwGsBufEnv[handle].crtBlendEnv, fbp, psm, w, h);
	mh = magh < 1 ? 0 : magh - 1;
	mv = magv < 1 ? 0 : magv - 1;

	if (gs_env.videoMode == 2) {
		if (2560 < mh * w + w)
			mh = (w + 2559) / w - 1;
		vt = mv * h + h;
		dwt = w * (mh + 1);
		if (gs_env.interlace == 1)
			g_fwGsBufEnv[handle].crtBlendEnv->display = ((long long)mh << 23) | ((long long)mv << 27)
			    | ((long long)((gs_env.ffMode ? vt * 2 : vt) - 1) << 44)
			    | (((long long)(dx * (mh + 1)) + 636) & 0xfff) | ((long long)((dy + 50) & 0xfff) << 12)
			    | ((long long)(dwt - 1) << 32);
		else
			g_fwGsBufEnv[handle].crtBlendEnv->display = ((long long)mh << 23) | ((long long)mv << 27)
			    | ((long long)(vt - 1) << 44) | (((long long)(dx * (mh + 1)) + 636) & 0xfff)
			    | ((long long)((dy + 25) & 0xfff) << 12) | ((long long)(dwt - 1) << 32);
	} else if (gs_env.videoMode == 3) {
		if (2560 < mh * w + w)
			mh = (w + 2559) / w - 1;
		vt = mv * h + h;
		dwt = w * (mh + 1);
		if (gs_env.interlace == 1)
			g_fwGsBufEnv[handle].crtBlendEnv->display = ((long long)mh << 23) | ((long long)mv << 27)
			    | ((long long)((gs_env.ffMode ? vt * 2 : vt) - 1) << 44)
			    | (((long long)(dx * (mh + 1)) + 656) & 0xfff) | ((long long)((dy + 72) & 0xfff) << 12)
			    | ((long long)(dwt - 1) << 32);
		else
			g_fwGsBufEnv[handle].crtBlendEnv->display = ((long long)mh << 23) | ((long long)mv << 27)
			    | ((long long)(vt - 1) << 44) | (((long long)(dx * ((dwt + 2559) / dwt)) + 656) & 0xfff)
			    | ((long long)((dy + 36) & 0xfff) << 12) | ((long long)(dwt - 1) << 32);
	} else if (gs_env.videoMode == 59) {
		if (2048 < mh * w + w)
			mh = (w + 2047) / w - 1;
		vt = h * (mv + 1);
		dwt = w * (mh + 1);
		g_fwGsBufEnv[handle].crtBlendEnv->display = ((long long)mh << 23) | ((long long)mv << 27)
		    | ((long long)(vt - 1) << 44) | (((long long)(dx * (mh + 1)) + 580) & 0xfff)
		    | ((long long)((dy * (mv + 1) + 34) & 0xfff) << 12) | ((long long)(dwt - 1) << 32);
	} else if (gs_env.videoMode == 82) {
		if (1280 < mh * w + w)
			mh = (w + 1279) / w - 1;
		vt = h * (mv + 1);
		dwt = w * (mh + 1);
		g_fwGsBufEnv[handle].crtBlendEnv->display = ((long long)mh << 23) | ((long long)mv << 27)
		    | ((long long)(vt - 1) << 44) | (((long long)(dx * (mh + 1)) + 302) & 0xfff)
		    | ((long long)((dy * (mv + 1) + 24) & 0xfff) << 12) | ((long long)(dwt - 1) << 32);
	}
}

/* Fills a display packet with sceGsSetDefDispEnv (the V variant for the
 * progressive modes, gs_env.videoMode >= 26), then rewrites DISPFB: FBP = fbp,
 * FBW from w, PSM = psm. */
void fwGsSetDispEnv(sceGsDispEnv *env, int fbp, short psm, short w, short h)
{
	if (gs_env.videoMode >= 26)
		sceGsSetDefDispEnvV(env, psm, w, h, 0, 0);
	else
		sceGsSetDefDispEnv(env, psm, w, h, 0, 0);

	env->dispfb =
	    ((long long)(psm & 0xf) << 15) | ((long long)(((w + 63) >> 6) & 0x3f) << 9) | (long long)(fbp & 0x1ff);
}

int fwGsGetBufWidth(int bufId)
{
	fwGsBufEnvEntry *p;
	if (bufId == 0xffff)
		return -1;
	p = g_fwGsBufEnv + bufId;
	return p->width;
}

int fwGsGetBufHeight(int bufId)
{
	fwGsBufEnvEntry *p;
	if (bufId == 0xffff)
		return -1;
	p = g_fwGsBufEnv + bufId;
	return p->height;
}

int fwGsGetBufAddr(int bufId)
{
	fwGsBufEnvEntry *p;
	if (bufId == 0xffff)
		return -1;
	p = g_fwGsBufEnv + bufId;
	return p->addr;
}

/* Allocates slot `idx`, fills its fields and sets up its five GS environments
 * from the pools. */
static int fwGs_setBufEnvImpl(int idx, int addr, short psm, short w, short h, short ztest, short zpsm, short clear)
{
	fwGsBufEnvEntry *p;

	p = g_fwGsBufEnv + idx;
	g_fwGsBufEnvAlloc[idx] = 1;
	p->addr = addr;
	p->psm = psm;
	p->zpsm = zpsm;
	p->width = w;
	p->height = h;
	p->size = _getBufSize(psm, w, h);
	p->zsize = _getBufSize(zpsm, w, h);
	p->ztest = ztest;
	p->clear = clear;
	p->clearEnv = &g_fwGsClearPool[idx];
	p->transEnv = &g_fwGsTransPool[idx];
	p->crtBlendEnv = &g_fwGsDispPool[idx];
	p->drawEnv = &g_fwGsDrawPool[idx];
	p->texEnv = &g_fwGsTexPool[idx];
	fwGsSetDrawEnv(p->drawEnv, addr, psm, w, h, ztest, zpsm);
	fwGsSetClearEnv(p->clearEnv, psm, w, h, ztest, clear);
	fwGsSetTexEnv(p->texEnv, addr, psm, w, h, ztest);
	fwGsSetTransEnv(p->transEnv, addr, w, psm, 0, 0, w, h);
	return idx;
}

/* Registers a buffer in the first free slot (0..31); -1 if the table is full. */
int fwGsSetBufEnv(int addr, short psm, short w, short h, short ztest, short zpsm, short clear)
{
	int idx = 0;

	if (g_fwGsBufEnvAlloc[0] == 1) {
		do {
			if (idx >= 32)
				return -1;
			idx++;
		} while (g_fwGsBufEnvAlloc[idx] == 1);
	}
	return fwGs_setBufEnvImpl(idx, addr, psm, w, h, ztest, zpsm, clear);
}

void fwGsDelBufEnv(int handle)
{
	g_fwGsBufEnvAlloc[handle] = 0;
}

/* Makes `handle` the visible buffer: point its crtBlend environment into the
 * visible pool, fills its display packet and copies psm, Z psm, ztest, width and
 * height into gs_env. */
void fwGsSetVisibleBufEnv(int handle)
{
	fwGsBufEnvEntry *e;

	g_fwGsBufEnv[handle].crtBlendEnv = &g_fwGsDispPool[handle];

	e = g_fwGsBufEnv + handle;
	fwGsSetDispEnv(g_fwGsBufEnv[handle].crtBlendEnv, e->addr, e->psm, e->width, e->height);

	gs_env.visPsm = e->psm;
	gs_env.visZpsm = e->zpsm;
	gs_env.visZtest = e->ztest;
	gs_env.visWidth = e->width;
	gs_env.visHeight = e->height;
}

/* Rewrites DISPLAY of the buffer crtBlend environment. dw/dh are
 * clamped to the buffer size (whole buffer when <= 0), w/h forced to >= 1.
 * Video modes 2 (NTSC, DX 636) and 3 (PAL, DX 656) depend on interlace
 * (interlace); mode 59 doubles w; mode 82 writes nothing and returns 0; any other
 * mode returns -1. */
int fwGsSetCrtBlendDisplay(int handle, int dx, int dy, int w, int h, int dw, int dh)
{
	sceGsDispEnv *p = g_fwGsBufEnv[handle].crtBlendEnv;

	if (dw <= 0 || g_fwGsBufEnv[handle].width < dw)
		dw = g_fwGsBufEnv[handle].width;
	if (dh <= 0 || g_fwGsBufEnv[handle].height < dh)
		dh = g_fwGsBufEnv[handle].height;
	w = 0 < w ? w : 1;
	h = 0 < h ? h : 1;

	if (gs_env.videoMode == 2) {
		if (gs_env.interlace == 1)
			p->display = ((long long)((w + 2559) / dw - 1) << 23) | ((long long)(h / dh - 1) << 27)
			    | ((long long)((gs_env.ffMode ? h * 2 : h) - 1) << 44)
			    | (((long long)(dx * ((w + 2559) / w)) + 636) & 0xfff)
			    | ((long long)((dy + 50) & 0xfff) << 12) | ((long long)((w + 2559) / w * w - 1) << 32);
		else
			p->display = ((long long)((w + 2559) / dw - 1) << 23) | ((long long)(h / dh - 1) << 27)
			    | ((long long)(h - 1) << 44) | (((long long)(dx * ((w + 2559) / w)) + 636) & 0xfff)
			    | ((long long)((dy + 25) & 0xfff) << 12) | ((long long)((w + 2559) / w * w - 1) << 32);
	} else if (gs_env.videoMode == 3) {
		if (gs_env.interlace == 1)
			p->display = ((long long)((w + 2559) / dw - 1) << 23) | ((long long)(h / dh - 1) << 27)
			    | ((long long)((gs_env.ffMode ? h * 2 : h) - 1) << 44)
			    | (((long long)(dx * ((w + 2559) / w)) + 656) & 0xfff)
			    | ((long long)((dy + 72) & 0xfff) << 12) | ((long long)((w + 2559) / w * w - 1) << 32);
		else
			p->display = ((long long)((w + 2559) / dw - 1) << 23) | ((long long)(h / dh - 1) << 27)
			    | ((long long)(h - 1) << 44) | (((long long)(dx * ((w + 2559) / w)) + 656) & 0xfff)
			    | ((long long)((dy + 36) & 0xfff) << 12) | ((long long)((w + 2559) / w * w - 1) << 32);
	} else if (gs_env.videoMode == 59) {
		int dwt = w * 2;

		p->display = ((long long)((0 < dwt / dw ? dwt / dw : 1) - 1) << 23)
		    | ((long long)((0 < h / dh ? h / dh : 1) - 1) << 27) | ((long long)(h - 1) << 44)
		    | (((long long)(dx * 2) + 580) & 0xfff) | ((long long)((dy + 34) & 0xfff) << 12)
		    | ((long long)(dwt - 1) << 32);
	} else if (gs_env.videoMode != 82) {
		return -1;
	}

	return 0;
}

/* SLBG (PMODE bit 7) from flag, and BGCOLOR. */
void fwGsSetCrtBlendBgColor(int handle, int color, int flag)
{
	unsigned int bit = (flag & 1) << 7;

	g_fwGsBufEnv[handle].crtBlendEnv->pmode &= ~0x80ULL;
	g_fwGsBufEnv[handle].crtBlendEnv->pmode |= bit;
	g_fwGsBufEnv[handle].crtBlendEnv->bgcolor = (unsigned int)(color & 0x00FFFFFF);
}

/* ALP (bits 8-15) and MMOD (bit 5) of PMODE in the crtBlend environment. */
void fwGsSetCrtBlendAlpha(int handle, int alpha, int flag)
{
	g_fwGsBufEnv[handle].crtBlendEnv->pmode &= ~0xff20ULL;
	g_fwGsBufEnv[handle].crtBlendEnv->pmode |= (unsigned int)(((alpha & 0xff) << 8) | ((flag & 1) << 5));
}

/* Points crtBlendEnv into the display pool, sets PMODE to 0xff23, packs DISPFB
 * and sets DISPLAY to the whole buffer. */
void fwGsSetCrtBlendBufEnv(int handle)
{
	fwGsBufEnvEntry *e;
	sceGsDispEnv *env;

	g_fwGsBufEnv[handle].crtBlendEnv = &g_fwGsDispPool[handle];
	env = g_fwGsBufEnv[handle].crtBlendEnv;
	env->pmode = 0xff23;

	e = g_fwGsBufEnv + handle;
	g_fwGsBufEnv[handle].crtBlendEnv->dispfb =
	    (long long)e->addr | ((long long)(short)(e->width / 64) << 9) | ((long long)e->psm << 15);

	fwGsSetCrtBlendDisplay(handle, 0, 0, e->width, e->height, e->width, e->height);
}

/* Pushes the buffer environment (32 levels) and appends its packet to the
 * DMA chain. Returns the previous handle, or -1. */
int fwGsPushTransEnv(int handle)
{
	unsigned int c = g_fwGsTransStack.count;
	if (c >= 32)
		goto fail;
	g_fwGsTransStack.handle[c] = handle;
	if (handle != 0xffff) {
		fwGsPacketHead *env = &g_fwGsBufEnv[handle].transEnv->head;
		g_fwGsTransStack.env[g_fwGsTransStack.count++] = env;
		fwPkAddDmaChain_withRef(env->vif, env->dma.qwc);
	} else {
		g_fwGsTransStack.env[g_fwGsTransStack.count++] = 0;
	}
	{
		unsigned int e = g_fwGsTransStack.count;
		if (e < 2) {
fail:
			return -1;
		}
		return g_fwGsTransStack.handle[e - 2];
	}
}

/* Pops the buffer environment and appends its packet to the DMA chain.
 * Returns the popped handle, 0xffff for an empty slot, -1 on underflow. */
int fwGsPopTransEnv(void)
{
	fwGsEnvStack *s = &g_fwGsTransStack;
	fwGsPacketHead *env;
	if (s->count == 0)
		return -1;
	s->count = s->count - 1;
	env = s->env[s->count];
	if (env) {
		fwPkAddDmaChain_withRef(env->vif, env->dma.tag);
		return s->handle[s->count];
	}
	return 0xffff;
}

void fwGsResetTransStack(void)
{
	g_fwGsTransStack.count = 0;
}

/* The draw environment also copies the buffer width/height into gs_env. */
int fwGsPushDrawEnv(int handle)
{
	unsigned int c = g_fwGsDrawStack.count;
	if (c >= 32)
		goto fail;
	g_fwGsDrawStack.handle[c] = handle;
	if (handle != 0xffff) {
		fwGsBufEnvEntry *e = &g_fwGsBufEnv[handle];
		fwGsPacketHead *env = &e->drawEnv->head;
		g_fwGsDrawStack.env[g_fwGsDrawStack.count++] = env;
		fwPkAddDmaChain_withRef(env->vif, env->dma.qwc);
		gs_env.visWidth = e->width;
		gs_env.visHeight = e->height;
	} else {
		g_fwGsDrawStack.env[g_fwGsDrawStack.count++] = 0;
	}
	{
		unsigned int e2 = g_fwGsDrawStack.count;
		if (e2 < 2) {
fail:
			return -1;
		}
		return g_fwGsDrawStack.handle[e2 - 2];
	}
}

int fwGsPopDrawEnv(void)
{
	fwGsEnvStack *s = &g_fwGsDrawStack;
	fwGsPacketHead *env;
	int handle;
	int *o;
	fwGsBufEnvEntry *e;
	if (s->count == 0)
		return -1;
	s->count = s->count - 1;
	env = s->env[s->count];
	if (env) {
		fwPkAddDmaChain_withRef(env->vif, env->dma.tag);
		handle = s->handle[s->count];
		o = &gs_env.visWidth;
		e = g_fwGsBufEnv + handle;
		o[0] = e->width;
		o[1] = e->height;
		return handle;
	}
	return 0xffff;
}

void fwGsResetDrawStack(void)
{
	g_fwGsDrawStack.count = 0;
}

int fwGsPushClearBuf(int handle)
{
	unsigned int c = g_fwGsClearStack.count;
	if (c >= 32)
		goto fail;
	g_fwGsClearStack.handle[c] = handle;
	if (handle != 0xffff) {
		fwGsPacketHead *env = &g_fwGsBufEnv[handle].clearEnv->head;
		g_fwGsClearStack.env[g_fwGsClearStack.count++] = env;
		fwPkAddDmaChain_withRef(env->vif, env->dma.qwc);
	} else {
		g_fwGsClearStack.env[g_fwGsClearStack.count++] = 0;
	}
	{
		unsigned int e = g_fwGsClearStack.count;
		if (e < 2) {
fail:
			return -1;
		}
		return g_fwGsClearStack.handle[e - 2];
	}
}

int fwGsPopClearBuf(void)
{
	fwGsEnvStack *s = &g_fwGsClearStack;
	fwGsPacketHead *env;
	if (s->count == 0)
		return -1;
	s->count = s->count - 1;
	env = s->env[s->count];
	if (env) {
		fwPkAddDmaChain_withRef(env->vif, env->dma.tag);
		return s->handle[s->count];
	}
	return 0xffff;
}

void fwGsResetClearStack(void)
{
	g_fwGsClearStack.count = 0;
}

int fwGsPushTexEnv(int handle)
{
	unsigned int c = g_fwGsTexStack.count;
	if (c >= 32)
		goto fail;
	g_fwGsTexStack.handle[c] = handle;
	if (handle != 0xffff) {
		fwGsPacketHead *env = &g_fwGsBufEnv[handle].texEnv->head;
		g_fwGsTexStack.env[g_fwGsTexStack.count++] = env;
		fwPkAddDmaChain_withRef(env->vif, env->dma.qwc);
	} else {
		g_fwGsTexStack.env[g_fwGsTexStack.count++] = 0;
	}
	{
		unsigned int e = g_fwGsTexStack.count;
		if (e < 2) {
fail:
			return -1;
		}
		return g_fwGsTexStack.handle[e - 2];
	}
}

int fwGsPopTexEnv(void)
{
	fwGsEnvStack *s = &g_fwGsTexStack;
	fwGsPacketHead *env;
	if (s->count == 0)
		return -1;
	s->count = s->count - 1;
	env = s->env[s->count];
	if (env) {
		fwPkAddDmaChain_withRef(env->vif, env->dma.tag);
		return s->handle[s->count];
	}
	return 0xffff;
}

void fwGsResetTexStack(void)
{
	g_fwGsTexStack.count = 0;
}

/* Attaches the Z buffer of entry `zhandle` to the draw and clears environments of
 * `handle`. width and height are compared as one word. The final sync makes
 * the writes visible to DMA. */
void fwGsAttachZBuf(int handle, int zhandle)
{
	fwGsBufEnvEntry *d = &g_fwGsBufEnv[handle];
	fwGsBufEnvEntry *z = &g_fwGsBufEnv[zhandle];

	if (*(int *)&d->width != *(int *)&z->width)
		return;

	if (z->ztest == 0)
		*(fwGsReg64 *)&d->drawEnv->draw.zbuf1 =
		    PS2_GS_SETREG_ZBUF(z->addr, z->zpsm & 0xf, PS2_GS_ZBUF_ZMSK_MASK);
	else
		*(fwGsReg64 *)&d->drawEnv->draw.zbuf1 =
		    PS2_GS_SETREG_ZBUF(z->addr, z->zpsm & 0xf, PS2_GS_ZBUF_ZMSK_NOMASK);

	if (z->ztest)
		*(fwGsReg64 *)&d->drawEnv->draw.test1 =
		    PS2_GS_SETREG_TEST(0, 0, 0, 0, 0, 0, PS2_GS_TEST_ZTE_ON, z->ztest & 3);
	else
		*(fwGsReg64 *)&d->drawEnv->draw.test1 =
		    PS2_GS_SETREG_TEST(0, 0, 0, 0, 0, 0, PS2_GS_TEST_ZTE_OFF, PS2_GS_TEST_ZTST_ALWAYS);

	if (z->ztest)
		*(fwGsReg64 *)&d->clearEnv->clear.testb =
		    PS2_GS_SETREG_TEST(0, 0, 0, 0, 0, 0, PS2_GS_TEST_ZTE_ON, z->ztest & 3);
	else
		*(fwGsReg64 *)&d->clearEnv->clear.testb =
		    PS2_GS_SETREG_TEST(0, 0, 0, 0, 0, 0, PS2_GS_TEST_ZTE_OFF, PS2_GS_TEST_ZTST_ALWAYS);

	PS2_DMA_SYNC();
}

/* Busy-wait delay, longer when the buffer has a crtBlend environment. */
void fwGsDumpBufEnv(int handle)
{
	int i;
	sceGsDispEnv *env;

	for (i = 10; i >= 0; i--)
		;
	for (i = 8; i >= 0; i--)
		;
	for (i = 11; i >= 0; i--)
		;
	for (i = 7; i >= 0; i--)
		;

	env = g_fwGsBufEnv[handle].crtBlendEnv;
	if (env) {
		for (i = 4; i >= 0; i--)
			;
	}
}

/* Valid only for allocated handles. */
int fwGsGetBufSize(int handle)
{
	fwGsBufEnvEntry *p;
	if (!g_fwGsBufEnvAlloc[handle])
		return 0;
	p = g_fwGsBufEnv + handle;
	return p->size;
}

/* Enables dithering (bit 0) in the DTHE register of the buffer draw
 * environment. `tmpl` is not used. */
void fwGsSetDthe(int handle, long long *tmpl)
{
	fwGsBufEnvEntry *p = g_fwGsBufEnv + handle;
	int *reg = (int *)&p->drawEnv->draw.dthe;
	*reg = *reg | 1;
}

/* ==== libgraph sceGsSetDef* ==== */

/* LoadImage packet: GIFtag (A+D, NLOOP 4) + BITBLTBUF + TRXPOS + TRXREG +
 * TRXDIR + GIFtag (IMAGE, NLOOP = qwords of pixel data for psm). */
int sceGsSetDefLoadImage(sceGsLoadImage1 *p, short fbp, short fbw, short psm, short x, short y, short w, short h)
{
	int n = 0;

	switch (psm) {
	case PS2_GS_PSMCT32:
	case PS2_GS_PSMZ32:
		n = (w * h) >> 2;
		break;
	case PS2_GS_PSMCT24:
	case PS2_GS_PSMZ24:
		n = (w * h * 3) >> 4;
		break;
	case PS2_GS_PSMCT16:
	case PS2_GS_PSMCT16S:
	case PS2_GS_PSMZ16:
	case PS2_GS_PSMZ16S:
		n = (w * h) >> 3;
		break;
	case PS2_GS_PSMT8:
	case PS2_GS_PSMT8H:
		n = (w * h) >> 4;
		break;
	case PS2_GS_PSMT4:
	case PS2_GS_PSMT4HL:
	case PS2_GS_PSMT4HH:
		n = (w * h) >> 5;
		break;
	}

	if (n > 32767)
		return 0;

	SCEGS_CLEAR_TAG(&p->giftag0);
	p->giftag0.NLOOP = 4;
	p->giftag0.NREG = 1;
	p->giftag0.REGS0 = PS2_GIFTAG_REGS_AD;

	*(__u64 *)&p->bitbltbuf = PS2_GS_SETREG_BITBLTBUF(0, 0, 0, fbp, fbw, psm);
	p->bitbltbufaddr = PS2_GS_BITBLTBUF;

	*(__u64 *)&p->trxpos = PS2_GS_SETREG_TRXPOS(0, 0, x, y, 0);
	p->trxposaddr = PS2_GS_TRXPOS;

	*(__u64 *)&p->trxreg = PS2_GS_SETREG_TRXREG(w, h);
	p->trxregaddr = PS2_GS_TRXREG;

	p->trxdiraddr = PS2_GS_TRXDIR;
	*(__u64 *)&p->trxdir = PS2_GS_SETREG_TRXDIR(PS2_GS_TRXDIR_HOST_TO_LOCAL);

	SCEGS_CLEAR_TAG(&p->giftag1);
	p->giftag1.NLOOP = n;
	p->giftag1.EOP = 1;
	p->giftag1.FLG = PS2_GIFTAG_FLG_IMAGE;

	PS2_DMA_SYNC();
	return sizeof(sceGsLoadImage1) / 16;
}

short sceGszbufaddr(int psm, short w, short h)
{
	int wpages = (w + 63) / 64;
	int hpages;

	if (psm & 2)
		hpages = (h + 63) / 64;
	else
		hpages = (h + 31) / 32;

	return (short)(wpages * hpages);
}

/* Draw environment: FRAME_1, ZBUF_1, XYOFFSET_1, SCISSOR_1, PRMODECONT,
 * COLCLAMP, DTHE, TEST_1 (as ps2_gs_set_drawenv of libps2dev). */
int sceGsSetDefDrawEnv(ps2_gs_drawenv1 *draw, short psm, short w, short h, short ztest, short zpsm)
{
	draw->frame1addr = PS2_GS_FRAME_1;
	*(__u64 *)&draw->frame1 = PS2_GS_SETREG_FRAME(0, ((w + 63) >> 6) & 0x3f, psm & 0xf, 0);

	draw->zbuf1addr = PS2_GS_ZBUF_1;
	if (ztest == 0)
		*(__u64 *)&draw->zbuf1 =
		    PS2_GS_SETREG_ZBUF(sceGszbufaddr(psm, w, h), zpsm & 0xf, PS2_GS_ZBUF_ZMSK_MASK);
	else
		*(__u64 *)&draw->zbuf1 =
		    PS2_GS_SETREG_ZBUF(sceGszbufaddr(psm, w, h), zpsm & 0xf, PS2_GS_ZBUF_ZMSK_NOMASK);

	draw->xyoffset1addr = PS2_GS_XYOFFSET_1;
	*(__u64 *)&draw->xyoffset1 =
	    PS2_GS_SETREG_XYOFFSET((2048 - (__u64)(short)(w >> 1)) << 4, (2048 - (__u64)(short)(h >> 1)) << 4);

	draw->scissor1addr = PS2_GS_SCISSOR_1;
	*(__u64 *)&draw->scissor1 = PS2_GS_SETREG_SCISSOR(0, w - 1, 0, h - 1);

	draw->prmodecontaddr = PS2_GS_PRMODECONT;
	draw->prmodecont.AC = PS2_GS_PRMODECONT_REFPRIM;

	draw->colclampaddr = PS2_GS_COLCLAMP;
	draw->colclamp.CLAMP = PS2_GS_COLCLAMP_CLAMP;

	draw->dtheaddr = PS2_GS_DTHE;
	if (psm & 0x2)
		draw->dthe.DTHE = PS2_GS_DTHE_ON;
	else
		draw->dthe.DTHE = PS2_GS_DTHE_OFF;

	draw->test1addr = PS2_GS_TEST_1;
	if (ztest)
		*(__u64 *)&draw->test1 = PS2_GS_SETREG_TEST(0, 0, 0, 0, 0, 0, 1, ztest & 3);
	else
		*(__u64 *)&draw->test1 = PS2_GS_SETREG_TEST(0, 0, 0, 0, 0, 0, 1, PS2_GS_TEST_ZTST_ALWAYS);

	PS2_DMA_SYNC();
	return sizeof(ps2_gs_drawenv1) / 16;
}

/* Clear packet: TEST_1, PRIM, RGBAQ, two XYZ2 and TEST_1, a full-screen sprite
 * that clears the frame and Z buffers (as ps2_gs_set_clear of libps2dev). */
int sceGsSetDefClear(ps2_gs_clear *cp, short ztest, short x, short y, short w, short h, unsigned char r,
    unsigned char g, unsigned char b, unsigned char a, __u32 z)
{
	cp->testaaddr = PS2_GS_TEST_1;
	*(__u64 *)&cp->testa = PS2_GS_SETREG_TEST(0, 0, 0, 0, 0, 0, 1, 1);

	*(__u64 *)&cp->prim = PS2_GS_SETREG_PRIM(PS2_GS_PRIM_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0);
	cp->primaddr = PS2_GS_PRIM;

	*(__u64 *)&cp->rgbaq = PS2_GS_SETREG_RGBAQ(r, g, b, a, 0x3f800000);
	cp->rgbaqaddr = PS2_GS_RGBAQ;

	*(__u64 *)&cp->xyz2a = PS2_GS_SETREG_XYZ(x << 4, y << 4, z);
	cp->xyz2aaddr = PS2_GS_XYZ2;

	*(__u64 *)&cp->xyz2b = PS2_GS_SETREG_XYZ((x + w) << 4, (y + h) << 4, z);
	cp->xyz2baddr = PS2_GS_XYZ2;

	cp->testbaddr = PS2_GS_TEST_1;
	if (ztest)
		*(__u64 *)&cp->testb =
		    PS2_GS_SETREG_TEST(PS2_GS_TEST_ATE_OFF, PS2_GS_TEST_ATST_NEVER, 0, PS2_GS_TEST_AFAIL_KEEP,
		        PS2_GS_TEST_DATE_OFF, PS2_GS_TEST_DATM_PASS0, PS2_GS_TEST_ZTE_ON, ztest & 3);
	else
		*(__u64 *)&cp->testb =
		    PS2_GS_SETREG_TEST(PS2_GS_TEST_ATE_OFF, PS2_GS_TEST_ATST_NEVER, 0, PS2_GS_TEST_AFAIL_KEEP,
		        PS2_GS_TEST_DATE_OFF, PS2_GS_TEST_DATM_PASS0, PS2_GS_TEST_ZTE_ON, PS2_GS_TEST_ZTST_ALWAYS);

	PS2_DMA_SYNC();
	return sizeof(ps2_gs_clear) / 16;
}

/* Texture environment: TEXFLUSH, TEX1_1, TEX0_1, CLAMP_1 (as
 * ps2_gs_set_texenv of libps2dev). */
int sceGsSetDefTexEnv(ps2_gs_texenv *tp, short flush, short tbp0, short tbw, short psm, short w2, short h2, short tfx,
    short cbp, short cpsm, short cld, short filter)
{
	if (flush)
		tp->texflushaddr = PS2_GS_TEXFLUSH;
	else
		tp->texflushaddr = PS2_GS_NOP;
	*(__u64 *)&tp->texflush = 0;

	tp->tex01addr = PS2_GS_TEX0_1;
	*(__u64 *)&tp->tex01 = PS2_GS_SETREG_TEX0(tbp0, tbw, psm, w2, h2, PS2_GS_TEX_TCC_RGBA, tfx, cbp, cpsm,
	    PS2_GS_TEX_CSM_CSM1, 0, cld);

	tp->tex11addr = PS2_GS_TEX1_1;
	*(__u64 *)&tp->tex11 =
	    PS2_GS_SETREG_TEX1(PS2_GS_TEX1_LCM_CALC, 0, filter & 1, filter, PS2_GS_TEX1_MTBA_NOAUTO, 0, 0);

	tp->clamp1addr = PS2_GS_CLAMP_1;
	*(__u64 *)&tp->clamp1 = PS2_GS_SETREG_CLAMP(PS2_GS_CLAMP_CLAMP, PS2_GS_CLAMP_CLAMP, 0, 0, 0, 0);

	PS2_DMA_SYNC();
	return sizeof(ps2_gs_texenv) / 16;
}

/* Alpha environment: ALPHA_1, PABE, TEXA, FBA_1 (as ps2_gs_set_alphaenv of
 * libps2dev). */
int sceGsSetDefAlphaEnv(ps2_gs_alphaenv *ap, short pabe)
{
	ap->alpha1addr = PS2_GS_ALPHA_1;
	*(__u64 *)&ap->alpha1 =
	    PS2_GS_SETREG_ALPHA(PS2_GS_ALPHA_A_CS, PS2_GS_ALPHA_B_CD, PS2_GS_ALPHA_C_AS, PS2_GS_ALPHA_D_CD, 0);

	ap->pabeaddr = PS2_GS_PABE;
	*(__u64 *)&ap->pabe = pabe;

	ap->texaaddr = PS2_GS_TEXA;
	*(__u64 *)&ap->texa = PS2_GS_SETREG_TEXA(127, 1, 129);

	ap->fba1addr = PS2_GS_FBA_1;
	*(__u64 *)&ap->fba1 = PS2_GS_SETREG_FBA(0);

	PS2_DMA_SYNC();
	return sizeof(ps2_gs_alphaenv) / 16;
}

/* Display packet: out[0] register address 102, out[1] SMODE from interlace/ffMode,
 * out[2] FBP|FBW, out[3] DISPLAY with magnification (w + 2559) / w (2560/w
 * rounded up), out[4] = 0. */
void sceGsSetDefDispEnv(void *out, short psm, short w, short h, short dx, short dy)
{
	long long *p = (long long *)out;

	p[0] = 102;

	if (gs_env.interlace) {
		if (gs_env.ffMode)
			p[1] = 3;
		else
			p[1] = 1;
	} else {
		p[1] = 2;
	}

	p[2] = ((long long)(psm & 0xf) << 15) | ((long long)(((w + 63) >> 6) & 0x3f) << 9);

	if (gs_env.videoMode == 2) {
		if (gs_env.interlace == 1)
			p[3] = ((long long)((w + 2559) / w - 1) << 23)
			    | ((long long)((gs_env.ffMode ? h * 2 : h) - 1) << 44)
			    | (((long long)(dx * ((w + 2559) / w)) + 636) & 0xfff)
			    | ((long long)((dy + 50) & 0xfff) << 12) | ((long long)((w + 2559) / w * w - 1) << 32);
		else
			p[3] = ((long long)((w + 2559) / w - 1) << 23) | ((long long)(h - 1) << 44)
			    | (((long long)(dx * ((w + 2559) / w)) + 636) & 0xfff)
			    | ((long long)((dy + 25) & 0xfff) << 12) | ((long long)((w + 2559) / w * w - 1) << 32);
	} else if (gs_env.videoMode == 3) {
		if (gs_env.interlace == 1)
			p[3] = ((long long)((w + 2559) / w - 1) << 23)
			    | ((long long)((gs_env.ffMode ? h * 2 : h) - 1) << 44)
			    | (((long long)(dx * ((w + 2559) / w)) + 656) & 0xfff)
			    | ((long long)((dy + 72) & 0xfff) << 12) | ((long long)((w + 2559) / w * w - 1) << 32);
		else
			p[3] = ((long long)((w + 2559) / w - 1) << 23) | ((long long)(h - 1) << 44)
			    | (((long long)(dx * ((w + 2559) / w)) + 656) & 0xfff)
			    | ((long long)((dy + 36) & 0xfff) << 12) | ((long long)((w + 2559) / w * w - 1) << 32);
	}

	p[4] = 0;
}

/* VESA/DTV variant of sceGsSetDefDispEnv. In DISPLAY, MAGH, DH and DW are
 * constants per video mode (gs_env.videoMode: 26-29 640x480, 42-46 800x600,
 * 59 1024x768, 60-62 1024x1024, 74/75 1280x1024, 80 720x480, 81 1920x1080,
 * 82 1280x720); DX = dx * (MAGH + 1) + C and DY = (dy + K) & 0xfff. Any other
 * mode leaves out[3] unwritten. */
void sceGsSetDefDispEnvV(void *out, short psm, short w, short h, short dx, short dy)
{
	long long *p = (long long *)out;

	p[0] = 102;

	if (gs_env.interlace) {
		if (gs_env.ffMode)
			p[1] = 3;
		else
			p[1] = 1;
	} else {
		p[1] = 2;
	}

	p[2] = ((long long)(psm & 0xf) << 15) | ((long long)(((w + 63) >> 6) & 0x3f) << 9);

	if (gs_env.videoMode == 26)
		p[3] = ((long long)1 << 23) | ((long long)479 << 44) | (((long long)(dx * 2) + 276) & 0xfff)
		    | ((long long)((dy + 34) & 0xfff) << 12) | ((long long)1279 << 32);
	else if (gs_env.videoMode == 27)
		p[3] = ((long long)1 << 23) | ((long long)479 << 44) | (((long long)(dx * 2) + 324) & 0xfff)
		    | ((long long)((dy + 30) & 0xfff) << 12) | ((long long)1279 << 32);
	else if (gs_env.videoMode == 28)
		p[3] = ((long long)1 << 23) | ((long long)479 << 44) | (((long long)(dx * 2) + 356) & 0xfff)
		    | ((long long)((dy + 18) & 0xfff) << 12) | ((long long)1279 << 32);
	else if (gs_env.videoMode == 29)
		p[3] = ((long long)1 << 23) | ((long long)479 << 44) | (((long long)(dx * 2) + 260) & 0xfff)
		    | ((long long)((dy + 27) & 0xfff) << 12) | ((long long)1279 << 32);
	else if (gs_env.videoMode == 42)
		p[3] = ((long long)1 << 23) | ((long long)599 << 44) | (((long long)(dx * 2) + 388) & 0xfff)
		    | ((long long)((dy + 23) & 0xfff) << 12) | ((long long)1599 << 32);
	else if (gs_env.videoMode == 43)
		p[3] = ((long long)1 << 23) | ((long long)599 << 44) | (((long long)(dx * 2) + 420) & 0xfff)
		    | ((long long)((dy + 26) & 0xfff) << 12) | ((long long)1599 << 32);
	else if (gs_env.videoMode == 44)
		p[3] = ((long long)1 << 23) | ((long long)599 << 44) | (((long long)(dx * 2) + 356) & 0xfff)
		    | ((long long)((dy + 28) & 0xfff) << 12) | ((long long)1599 << 32);
	else if (gs_env.videoMode == 45)
		p[3] = ((long long)1 << 23) | ((long long)599 << 44) | (((long long)(dx * 2) + 468) & 0xfff)
		    | ((long long)((dy + 23) & 0xfff) << 12) | ((long long)1599 << 32);
	else if (gs_env.videoMode == 46)
		p[3] = ((long long)1 << 23) | ((long long)599 << 44) | (((long long)(dx * 2) + 420) & 0xfff)
		    | ((long long)((dy + 29) & 0xfff) << 12) | ((long long)1599 << 32);
	else if (gs_env.videoMode == 59)
		p[3] = ((long long)1 << 23) | ((long long)767 << 44) | (((long long)(dx * 2) + 580) & 0xfff)
		    | ((long long)((dy + 34) & 0xfff) << 12) | ((long long)2047 << 32);
	else if (gs_env.videoMode == 60)
		p[3] = ((long long)0 << 23) | ((long long)767 << 44) | (((long long)(dx) + 268) & 0xfff)
		    | ((long long)((dy + 34) & 0xfff) << 12) | ((long long)1023 << 32);
	else if (gs_env.videoMode == 61)
		p[3] = ((long long)0 << 23) | ((long long)767 << 44) | (((long long)(dx) + 260) & 0xfff)
		    | ((long long)((dy + 30) & 0xfff) << 12) | ((long long)1023 << 32);
	else if (gs_env.videoMode == 62)
		p[3] = ((long long)0 << 23) | ((long long)767 << 44) | (((long long)(dx) + 292) & 0xfff)
		    | ((long long)((dy + 38) & 0xfff) << 12) | ((long long)1023 << 32);
	else if (gs_env.videoMode == 74)
		p[3] = ((long long)0 << 23) | ((long long)1023 << 44) | (((long long)(dx) + 348) & 0xfff)
		    | ((long long)((dy + 40) & 0xfff) << 12) | ((long long)1279 << 32);
	else if (gs_env.videoMode == 75)
		p[3] = ((long long)0 << 23) | ((long long)1023 << 44) | (((long long)(dx) + 380) & 0xfff)
		    | ((long long)((dy + 40) & 0xfff) << 12) | ((long long)1279 << 32);
	else if (gs_env.videoMode == 80)
		p[3] = ((long long)1 << 23) | ((long long)479 << 44) | (((long long)(dx * 2) + 232) & 0xfff)
		    | ((long long)((dy + 35) & 0xfff) << 12) | ((long long)1439 << 32);
	else if (gs_env.videoMode == 81)
		p[3] = ((long long)0 << 23) | ((long long)1079 << 44) | (((long long)(dx) + 238) & 0xfff)
		    | ((long long)((dy + 40) & 0xfff) << 12) | ((long long)1919 << 32);
	else if (gs_env.videoMode == 82)
		p[3] = ((long long)0 << 23) | ((long long)719 << 44) | (((long long)(dx) + 302) & 0xfff)
		    | ((long long)((dy + 24) & 0xfff) << 12) | ((long long)1279 << 32);

	p[4] = 0;
}
