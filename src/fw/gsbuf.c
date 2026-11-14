/*
 * GS frame buffer setup for the display modes (GsBufInit) and two full-screen
 * sprite effects built on it: defocus (blur by down/up-scaling) and a scaled
 * preview window.
 */

#include "fw.h"
#include "gsbuf.h"

/* An opaque 128-byte GS packet (DMAtag, GIFtag and register data). */
typedef struct GsOp {
	unsigned char opaque[128];
} __attribute__((aligned(16))) GsOp;

/* Frame buffer handles returned by fwGsSetBufEnv (integers, not pointers):
   fbuf0/fbuf1 draw and display buffers, zbuf0/zbuf1 z-buffers (-1 when the
   mode has none), tbuf0/tbuf1 the defocus work buffer and its large visible
   target. */
int fbuf0;
int fbuf1;
int zbuf0;
int zbuf1;
int tbuf0;
int tbuf1;
/* Never read. */
int dispbuf;
static GsOp gsOp0, gsOp1, gsOp2, gsOp3, gsOp4;

/* Stores one GS qword, highest word first. */
static __inline__ void gsQwSet(int *p, int w, int z, int y, int x)
{
	p[3] = w;
	p[2] = z;
	p[1] = y;
	p[0] = x;
}

/* Builds in `op` the DMA/GIF packet of a textured SPRITE that copies `src`
   onto `dst`, centred at (x, y) and scaled by `scale`: DMAtag + VIF DIRECT
   7 qw, GIFtag PACKED NREG 6 (PRIM RGBAQ UV XYZ2 UV XYZ2), PRIM 0x116, colour
   128x4, start and end UV/XYZ. `flags * 12` is the half-pixel offset in 1/16. */
static void gsOpBuild(int dst, int src, GsOp *op, int x, int y, float scale, int flags)
{
	int sw, sh;
	int *p;
	int px, py;
	int cx, cy;
	int dw, dh;
	int w, h;
	int ofs = flags * 12;

	/* Centre in GS coordinate space (origin at 2048), then half the source to
		   the left/top. */
	w = fwGsGetBufWidth(src);
	cx = x + 2048;
	px = cx - (w >> 1);
	h = fwGsGetBufHeight(src);
	cy = y + 2048;
	py = cy - (h >> 1);
	sw = (int)(fwGsGetBufWidth(src) * scale);
	sh = (int)(fwGsGetBufHeight(src) * scale);

	p = (int *)op;
	gsQwSet(p, 0x50000007, 0, 0, 0x60000007);
	gsQwSet(p + 4, 0, 0x535310, 0x60000000, 0x8001);
	gsQwSet(p + 8, 0, 0, 0, 278);
	gsQwSet(p + 12, 128, 128, 128, 128);
	gsQwSet(p + 16, 0, 0, 4, 4);
	gsQwSet(p + 20, 0, 0, (py << 4) + ofs, (px << 4) + ofs);
	dh = (fwGsGetBufHeight(dst) << 4) - 4;
	dw = (fwGsGetBufWidth(dst) << 4) - 4;
	gsQwSet(p + 24, 0, 0, dh, dw);
	gsQwSet(p + 28, 0, 0, (py << 4) + (sh << 4) + ofs, (px << 4) + (sw << 4) + ofs);
}

/* `op` is never read. */
static void gsOpExec(int dst, int src, GsOp *op)
{
	fwGsPushDrawEnv(src);
	fwGsPushTexEnv(dst);
	fwGsPopDrawEnv();
	fwGsPopTexEnv();
}

/* Same setter sequence as fwGsBufInit (fw.c), but the handles go to the
   globals above and there are four modes. Buffer offsets are the sum of the
   fwGsGetBufSize() of the previous buffers, recomputed each time. */
void GsBufInit(int mode)
{
	long long dthe = 0x6071243571603524LL;

	switch (mode) {
	case GSBUF_1024x768:
		fbuf0 = fwGsSetBufEnv(0, 2, 1024, 768, 0, 50, 1);
		fbuf1 = fwGsSetBufEnv(fwGsGetBufSize(fbuf0), 2, 1024, 768, 0, 50, 1);
		tbuf0 = fwGsSetBufEnv(fwGsGetBufSize(fbuf0) + fwGsGetBufSize(fbuf1), 0, 512, 256, 0, 48, 1);
		tbuf1 = fwGsSetBufEnv(0, 0, 1024, 768, 0, 50, 1);
		zbuf0 = -1;
		zbuf1 = -1;
		fwGsSetDthe(fbuf0, &dthe);
		fwGsSetDthe(fbuf1, &dthe);
		fwGsSetVisibleBufEnv(fbuf0);
		fwGsSetVisibleBufEnv(fbuf1);
		break;
	case GSBUF_640x448:
		fbuf0 = fwGsSetBufEnv(0, 0, 640, 448, 0, 50, 1);
		fbuf1 = fwGsSetBufEnv(fwGsGetBufSize(fbuf0), 0, 640, 448, 0, 50, 1);
		zbuf0 = fwGsSetBufEnv(fwGsGetBufSize(fbuf0) + fwGsGetBufSize(fbuf1), 2, 640, 448, 3, 58, 1);
		zbuf1 = fwGsSetBufEnv(fwGsGetBufSize(fbuf0) + fwGsGetBufSize(fbuf1), 2, 640, 448, 0, 58, 1);
		fwGsAttachZBuf(fbuf0, zbuf0);
		fwGsAttachZBuf(fbuf1, zbuf0);
		fwGsSetVisibleBufEnv(fbuf0);
		fwGsSetVisibleBufEnv(fbuf1);
		break;
	case GSBUF_896x512:
		fbuf0 = fwGsSetBufEnv(0, 0, 896, 512, 0, 50, 1);
		fbuf1 = fwGsSetBufEnv(fwGsGetBufSize(fbuf0), 0, 896, 512, 0, 50, 1);
		tbuf0 = fwGsSetBufEnv(fwGsGetBufSize(fbuf0) + fwGsGetBufSize(fbuf1), 0, 512, 256, 0, 48, 1);
		tbuf1 = fwGsSetBufEnv(0, 0, 1024, 768, 0, 50, 1);
		zbuf0 = -1;
		zbuf1 = -1;
		fwGsSetDthe(fbuf0, &dthe);
		fwGsSetDthe(fbuf1, &dthe);
		fwGsSetVisibleBufEnv(fbuf0);
		fwGsSetVisibleBufEnv(fbuf1);
		fwGsSetCustomDispEnv(fbuf0, 0, 0, 896, 512, 64, 128, 2, 1);
		fwGsSetCustomDispEnv(fbuf1, fwGsGetBufSize(fbuf0), 0, 896, 512, 64, 128, 2, 1);
		break;
	case GSBUF_640x224:
	default:
		fbuf0 = fwGsSetBufEnv(0, 0, 640, 224, 0, 48, 1);
		fbuf1 = fwGsSetBufEnv(fwGsGetBufSize(fbuf0), 0, 640, 224, 0, 48, 1);
		zbuf0 = fwGsSetBufEnv(fwGsGetBufSize(fbuf0) + fwGsGetBufSize(fbuf1), 2, 640, 224, 3, 58, 1);
		zbuf1 = -1;
		tbuf0 = fwGsSetBufEnv(fwGsGetBufSize(fbuf0) + fwGsGetBufSize(fbuf1) + fwGsGetBufSize(zbuf0), 2, 128,
		    128, 0, 50, 1);
		tbuf1 = fwGsSetBufEnv(fwGsGetBufSize(fbuf0) + fwGsGetBufSize(fbuf1) + fwGsGetBufSize(zbuf0)
		        + fwGsGetBufSize(tbuf0),
		    0, 256, 192, 0, 48, 1);
		fwGsAttachZBuf(fbuf0, zbuf0);
		fwGsAttachZBuf(fbuf1, zbuf0);
		fwGsSetVisibleBufEnv(fbuf0);
		fwGsSetVisibleBufEnv(fbuf1);
		break;
	}

	fwGsSetCrtBlendBufEnv(tbuf0);
	fwGsSetVisibleBufEnv(tbuf1);
	fwGsResetTexStack();
	fwGsResetTransStack();
	fwGsResetClearStack();
	fwGsResetDrawStack();
}

void DefocusInit(void)
{
	gsOpBuild(fbuf0, tbuf0, &gsOp0, 0, 0, 1.0f, 0);
	gsOpBuild(tbuf0, fbuf0, &gsOp1, 0, 0, 1.0f, 0);
	gsOpBuild(tbuf0, tbuf1, &gsOp2, 0, 0, 1.0f, 0);
	gsOpBuild(tbuf1, tbuf0, &gsOp3, 0, 0, 1.0f, 0);
}

void DefocusExec(int target, int passes)
{
	int i;

	gsOpExec(target, tbuf0, &gsOp0);
	for (i = 0; i < passes; i++) {
		gsOpExec(tbuf0, tbuf1, &gsOp2);
		gsOpExec(tbuf1, tbuf0, &gsOp3);
	}
	gsOpExec(tbuf0, target, &gsOp1);
}

void VisualWindowInit(void)
{
	gsOpBuild(tbuf0, fbuf0, &gsOp4, 355, 230, 0.3f, 0);
}

void VisualWindowExec(int target)
{
	gsOpExec(target, tbuf0, &gsOp0);
	gsOpExec(tbuf0, target, &gsOp4);
}
