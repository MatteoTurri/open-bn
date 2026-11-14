/*
 * Current GS state of the fw framework (frame counter, output device, gs_env)
 * with its getters, and the GS buffer chain setup.
 */

#include "fw.h"
#include "fw_gsenv.h"
#include "fw_pk.h"

int frame;
int odev;
fwGsEnvState gs_env;

int fwGsGetZValue(void)
{
	return gs_env.visZpsm;
}

int fwGsGetFieldFrame(void)
{
	return gs_env.ffMode;
}

void fwGetScreenSize(int *w, int *h)
{
	*w = gs_env.visWidth;
	*h = gs_env.visHeight;
}

/* Current field (1 = odd) in FRAME mode, 0 in FIELD mode. */
int fwGsGetOddEven(void)
{
	gs_env.multiBuffer = 1;
	if (gs_env.ffMode == FW_FF_FRAME) {
		if (fwHigIsAfterKick())
			return odev == 0;
		return odev;
	}
	return 0;
}

/* As fwGsGetOddEven, without polling the kick state. */
int fwGsGetOddEven_SingleBuffer(void)
{
	gs_env.multiBuffer = 0;
	if (gs_env.ffMode == FW_FF_FRAME)
		return odev;
	return 0;
}

/* Drawing origin offset in 12.4 fixed point, half a line lower on the odd
 * field. The multi-buffer variant polls fwHigIsAfterKick and sets the field
 * flag afterwards, the single-buffer one clears it. */
void fwGetXYOffset(int *outX, int *outY, int *outOddEven)
{
	int oddEven;

	if (gs_env.ffMode == FW_FF_FRAME) {
		if (fwHigIsAfterKick() != 0)
			oddEven = (unsigned)gs_env.oddEven < 1;
		else
			oddEven = gs_env.oddEven;
	} else {
		oddEven = 0;
	}

	*outX = (gs_env.originX - (gs_env.visWidth >> 1)) << 4;
	*outY = ((gs_env.originY - (gs_env.visHeight >> 1)) << 4) + (oddEven << 3);

	*outOddEven = oddEven;
	gs_env.multiBuffer = 1;
}

void fwGetXYOffset_SingleBuffer(int *outX, int *outY, int *outOddEven)
{
	int oddEven = 0;
	int *p;
	int w, h, x, y;

	if (gs_env.ffMode == FW_FF_FRAME)
		oddEven = gs_env.oddEven;

	p = &gs_env.originX;
	w = p[-2];
	x = p[0];
	*outX = (x - (w >> 1)) << 4;

	h = p[-1];
	y = p[1];
	*outY = ((y - (h >> 1)) << 4) + (oddEven << 3);

	*outOddEven = oddEven;
	p[3] = 0;
}

void fwGetFBPandZBP(int *fbp, int *unk, int *zbp)
{
	*fbp = gs_env.fbp;
	*unk = gs_env.unk_34;
	*zbp = gs_env.zbp;
}

int fwGsGetFrameCounter(void)
{
	return frame;
}

/* Allocates the framework GS buffers for video mode 1 (1024x768), 2 (640x448)
 * or default (640x224): each buffer starts where the previous ones end. Mode 1
 * has no Z buffer (*pZ = -1) and enables dithering instead; *pNone is always -1.
 * pTexLarge (448x320) and pTexSmall (256x192) are off-screen buffers after them. */
int fwGsBufInit(int mode, int *pDraw, int *pDisp, int *pZ, int *pNone, int *pTexLarge, int *pTexSmall)
{
	long long dthe = 0x6071243571603524LL;

	switch (mode) {
	case 1:
		*pDraw = fwGsSetBufEnv(0, 2, 1024, 768, 0, 50, 1);
		*pDisp = fwGsSetBufEnv(fwGsGetBufSize(*pDraw), 2, 1024, 768, 0, 50, 1);
		*pZ = -1;
		*pNone = -1;
		*pTexLarge = fwGsSetBufEnv(fwGsGetBufSize(*pDraw) + fwGsGetBufSize(*pDisp), 0, 448, 320, 0, 48, 1);
		*pTexSmall = fwGsSetBufEnv(fwGsGetBufSize(*pDraw) + fwGsGetBufSize(*pDisp) + fwGsGetBufSize(*pTexLarge),
		    0, 256, 192, 0, 48, 1);
		fwGsSetDthe(*pDraw, &dthe);
		fwGsSetDthe(*pDisp, &dthe);
		break;
	case 2:
		*pDraw = fwGsSetBufEnv(0, 0, 640, 448, 0, 50, 1);
		*pDisp = fwGsSetBufEnv(fwGsGetBufSize(*pDraw), 0, 640, 448, 0, 50, 1);
		*pZ = fwGsSetBufEnv(fwGsGetBufSize(*pDraw) + fwGsGetBufSize(*pDisp), 2, 640, 448, 3, 58, 1);
		*pNone = -1;
		*pTexLarge = fwGsSetBufEnv(fwGsGetBufSize(*pDraw) + fwGsGetBufSize(*pDisp) + fwGsGetBufSize(*pZ), 0,
		    448, 320, 0, 48, 1);
		*pTexSmall = fwGsSetBufEnv(fwGsGetBufSize(*pDraw) + fwGsGetBufSize(*pDisp) + fwGsGetBufSize(*pZ)
		        + fwGsGetBufSize(*pTexLarge),
		    0, 256, 192, 0, 48, 1);
		fwGsAttachZBuf(*pDraw, *pZ);
		fwGsAttachZBuf(*pDisp, *pZ);
		break;
	case 0:
	default:
		*pDraw = fwGsSetBufEnv(0, 0, 640, 224, 0, 48, 1);
		*pDisp = fwGsSetBufEnv(fwGsGetBufSize(*pDraw), 0, 640, 224, 0, 48, 1);
		*pZ = fwGsSetBufEnv(fwGsGetBufSize(*pDraw) + fwGsGetBufSize(*pDisp), 0, 640, 224, 3, 48, 1);
		*pNone = -1;
		*pTexLarge = fwGsSetBufEnv(fwGsGetBufSize(*pDraw) + fwGsGetBufSize(*pDisp) + fwGsGetBufSize(*pZ), 0,
		    448, 320, 0, 48, 1);
		*pTexSmall = fwGsSetBufEnv(fwGsGetBufSize(*pDraw) + fwGsGetBufSize(*pDisp) + fwGsGetBufSize(*pZ)
		        + fwGsGetBufSize(*pTexLarge),
		    0, 256, 192, 0, 48, 1);
		fwGsAttachZBuf(*pDraw, *pZ);
		fwGsAttachZBuf(*pDisp, *pZ);
		break;
	}

	fwGsSetVisibleBufEnv(*pDraw);
	fwGsSetVisibleBufEnv(*pDisp);
	fwGsResetTexStack();
	fwGsResetTransStack();
	fwGsResetClearStack();
	fwGsResetDrawStack();
}
