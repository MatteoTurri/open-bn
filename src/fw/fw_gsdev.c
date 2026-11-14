/*
 * GS device of the fw framework: open/reset/close (private copies of parts of
 * libps2dev), the registered double buffer and its per-frame flip, the
 * framebuffer dump and the three callback queues.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <linux/ps2/gs.h>
#include <linux/ps2/dev.h>
#include <linux/ps2/ee.h>
#include "ps2gs.h"
#include "ps2vpu.h"
#include "fw.h"
#include "fw_ev.h"
#include "fw_gsdev.h"
#include "fw_pk.h"
#include "gfx/tm.h"


/* Callbacks registered on one of the three framework hooks (up to 1024). */
typedef struct {
	int count;
	void (*slots[1024])(void);
} fwCbQueue;

int g_fd_gs;
ps2_vpu *g_vpu1;
static const char *g_fwGsStoreImageFile;
fwCbQueue preGsSync;
fwCbQueue postGsSync;
fwCbQueue preDmaKick;

/* Registered double buffer and crtBlend pair; -1 = no buffer. */
static int g_fwGsDBufDisp = -1;
static int g_fwGsDBufDraw = -1;
static int g_fwGsDBufFlag = -1;
static int g_fwGsCrtDisp = -1;
static int g_fwGsCrtSpare = -1;
static pthread_mutex_t g_fwGsRegMutex = PTHREAD_MUTEX_INITIALIZER;
static int g_fwGsUpdated = 0;
static pthread_mutex_t g_fwGsUMutex = PTHREAD_MUTEX_INITIALIZER;
static int g_fwGsStoreImageBuf = -1;
static int g_fwGsNotReadyFrames = 0;
static int g_fwGsPackerReady = 0;

/* Private copies of pieces of libps2dev (gs_open.c, gs_gparam.c, gs_reset.c);
 * they use the exported ps2_gs_get_gparam(). */
static int fwVc_gsSyncV(void);
static int fwVc_releaseDevice(void);
static int fwVc_setCrtMode(void);
static int fwVc_openDevices(void);
static int fwGs_resetVideoMode(int mode, int inter, int scrmode, int ffmode);
static int fwGs_closeGsDevice(void);

/* Resets the four environment stacks and pushes draw/clear of the current draw
 * kick base buffer. The argument is not used. */
int fwGsClearDrawBuf(int handle)
{
	fwGsResetTexStack();
	fwGsResetTransStack();
	fwGsResetClearStack();
	fwGsResetDrawStack();
	fwGsPushDrawEnv(fwGsGetKickBaseDrawBuf());
	fwGsPushClearBuf(fwGsGetKickBaseDrawBuf());
	return 0;
}

int fwGsUpdateStatus(void)
{
	pthread_mutex_lock(&g_fwGsUMutex);
	g_fwGsUpdated = 1;
	pthread_mutex_unlock(&g_fwGsUMutex);
	return 1;
}

int fwGsCheckUpdateStatus(void)
{
	int old;
	pthread_mutex_lock(&g_fwGsUMutex);
	old = g_fwGsUpdated;
	g_fwGsUpdated = 0;
	pthread_mutex_unlock(&g_fwGsUMutex);
	return old;
}

/* Registers the double buffer triple under the mutex; always returns 1. */
int fwGsRegistDBuf(int disp, int draw, int flag)
{
	pthread_mutex_lock(&g_fwGsRegMutex);
	g_fwGsDBufDisp = disp;
	g_fwGsDBufDraw = draw;
	g_fwGsDBufFlag = flag;
	pthread_mutex_unlock(&g_fwGsRegMutex);
	return 1;
}

/* Returns disp/draw, and the flag if both are >= 0, else -1. */
int fwGsCheckRegDBuf(int *disp, int *draw)
{
	int result;
	pthread_mutex_lock(&g_fwGsRegMutex);
	*disp = g_fwGsDBufDisp;
	*draw = g_fwGsDBufDraw;
	result = g_fwGsDBufFlag;
	if (g_fwGsDBufDisp < 0 || g_fwGsDBufDraw < 0)
		result = -1;
	pthread_mutex_unlock(&g_fwGsRegMutex);
	return result;
}

/* Registers the crtBlend pair under the mutex; always returns 1. */
int fwGsRegistCrtBlendBuf(int disp, int spare)
{
	pthread_mutex_lock(&g_fwGsRegMutex);
	g_fwGsCrtDisp = disp;
	g_fwGsCrtSpare = spare;
	pthread_mutex_unlock(&g_fwGsRegMutex);
	return 1;
}

/* As fwGsCheckRegDBuf on the crtBlend pair, returning 0 on success. */
int fwGsCheckCrtBlendBuf(int *disp, int *spare)
{
	int result;
	pthread_mutex_lock(&g_fwGsRegMutex);
	*disp = g_fwGsCrtDisp;
	*spare = g_fwGsCrtSpare;
	result = 0;
	if (g_fwGsCrtDisp < 0 || g_fwGsCrtSpare < 0)
		result = -1;
	pthread_mutex_unlock(&g_fwGsRegMutex);
	return result;
}

/* ps2_gs_sync_v() of gs_dbuff.c without the GETEVENT/WAITEVENT ioctls: returns
 * the FIELD bit of CSR. */
static int fwVc_gsSyncV(void)
{
	ps2_gs_gparam *gp = ps2_gs_get_gparam();
	struct ps2_gssreg reg;

	if (gp->interlace_mode == PS2_GS_INTERLACE) {
		reg.reg = PS2_GSSREG_CSR;
		if (ioctl(gp->fd_gs, PS2IOC_GGSSREG, &reg) < 0) {
			perror("ps2_gs_sync_v, ioctl");
		}
		return ((ps2_gssreg_csr *)&reg.val)->FIELD;
	}

	return 1; /* odd */
}

int fwGsCountFrameOdev(void)
{
	int r;
	frame++;
	r = fwVc_gsSyncV();
	odev = ((unsigned)r < 1);
	gs_env.oddEven = odev;
	return frame;
}

/* Puts the display environment of `disp`, makes `draw` the disp kick base,
 * registers (draw, disp, flag) as the double buffer and returns draw. */
int fwGsFlipRegDispBuf(int disp, int draw, int flag)
{
	int result;

	fwGsPutDispEnv(disp);
	fwGsSetKickBaseDispBuf(draw);

	pthread_mutex_lock(&g_fwGsRegMutex);
	g_fwGsDBufDisp = draw;
	g_fwGsDBufDraw = disp;
	g_fwGsDBufFlag = flag;
	pthread_mutex_unlock(&g_fwGsRegMutex);

	result = g_fwGsDBufDisp;
	return result;
}

/* If disp != draw registers (spare, disp) as the crtBlend pair and puts disp;
 * if disp == draw puts spare, or fails with -1 when spare == disp too. */
int fwGsFlipCrtBlendBuf(int disp, int spare, int draw)
{
	int result;
	if (disp != draw) {
		result = disp;
		pthread_mutex_lock(&g_fwGsRegMutex);
		g_fwGsCrtDisp = spare;
		g_fwGsCrtSpare = result;
		pthread_mutex_unlock(&g_fwGsRegMutex);
	} else {
		if (spare == disp)
			return -1;
		result = spare;
	}
	fwGsPutCrtBlendEnv(result);
	return result;
}

/* The first argument is not used. Flip the registered double buffer: -1 if it
 * is incomplete, -2 if its flag is 1 and notReady is set; also flips the
 * crtBlend pair when one is registered. */
int fwGsFlipRegDBuf(int unused, int notReady)
{
	int disp, draw, flag, result, crtDisp, crtSpare, crtStatus;

	flag = fwGsCheckRegDBuf(&disp, &draw);
	if (flag < 0 || disp < 0 || draw < 0)
		return -1;

	if (flag == 1 && notReady != 0)
		return -2;

	result = fwGsFlipRegDispBuf(disp, draw, flag);

	crtStatus = fwGsCheckCrtBlendBuf(&crtDisp, &crtSpare);
	if (crtStatus >= 0)
		fwGsFlipCrtBlendBuf(crtDisp, crtSpare, draw);

	return result;
}

/* Per-frame callback: count the frame, flip the double buffer (passing the
 * number of frames the DMA packer was not ready), reapply the CRT mode after a
 * status update, then kick the DMA chain. */
int fwGsFlipRegDBuf_cb(void)
{
	int count = fwGsCountFrameOdev();
	int ready = fwPkCheckReady();
	int result;

	g_fwGsPackerReady = ready;

	if (ready != 0)
		g_fwGsNotReadyFrames = 0;
	else
		g_fwGsNotReadyFrames = g_fwGsNotReadyFrames + 1;

	result = fwGsFlipRegDBuf(count, g_fwGsNotReadyFrames);

	if (fwGsCheckUpdateStatus() != 0)
		fwVc_setCrtMode();

	return fwPkKickChainBuf((void *)g_fwGsPackerReady, result);
}

int fwPkSyncV(void)
{
	int result, draw;
	fwPkFlipChainBuf();
	tmSync();
	result = fwEvSyncWait(PS2EV_N_VBSTART, 0);
	draw = fwGsGetKickBaseDrawBuf();
	fwGsClearDrawBuf(draw);
	return result;
}

/* VU0 is not directly accessible from PS2 Linux userland. */
int fwVcInitEnable(int enable)
{
	return -1;
}

/* As fwGsReleaseDevice, but closes the GS device with ps2_gs_close(). */
static int fwVc_releaseDevice(void)
{
	if (g_vpu1) {
		ps2_vpu_close(g_vpu1);
		g_vpu1 = 0;
	}
	if (g_fd_gs >= 0) {
		ps2_gs_close();
		g_fd_gs = -1;
	}
	return 0;
}

/* ps2_gs_open() without opening /dev/ps2event. */
static int fwVc_gsOpen(int fd_event)
{
	ps2_gs_gparam *gp = ps2_gs_get_gparam();

	gp->fd_gs = open(PS2_DEV_GS, O_RDWR);
	if (gp->fd_gs < 0) {
		perror("ps2_gs_open, gs");
		return -1;
	}
	ioctl(gp->fd_gs, PS2IOC_GSCREENINFO, &gp->oldinfo);
	gp->fd_event_open = 0;
	return gp->fd_gs;
}

/* __ps2_gs_calc_offset() of gs_gparam.c. */
static void fwVc_calcOffset(void)
{
	ps2_gs_gparam *gp = ps2_gs_get_gparam();

	gp->offset_x = gp->center_x - gp->width / 2;
	gp->offset_y = gp->center_y - gp->height / 2;
}

/* __ps2_gs_set_screen_size() of gs_gparam.c. */
static void fwVc_setScreenSize(int w, int h)
{
	ps2_gs_gparam *gp = ps2_gs_get_gparam();

	gp->width = w;
	gp->height = h;
	gp->pixel_ratio = (gp->out_mode == PS2_GS_VESA) ? 1.0f : (0.47f / 0.5f);
	if (gp->interlace_format && (gp->interlace_mode == PS2_GS_NOINTERLACE || gp->ff_mode == PS2_GS_FRAME)) {
		gp->pixel_ratio /= 2;
	}
	fwVc_calcOffset();
}

/* The PS2IOC_SCRTMODE part of ps2_gs_reset(). */
static int fwVc_setCrtMode(void)
{
	ps2_gs_gparam *gp = ps2_gs_get_gparam();
	struct ps2_crtmode crtmode;

	crtmode.mode = gp->out_mode;

	switch (gp->out_mode) {
	case PS2_GS_DTV:
	case PS2_GS_NTSC:
	case PS2_GS_PAL:
		crtmode.res = gp->interlace_mode | gp->ff_mode;
		break;

	case PS2_GS_VESA:
		crtmode.res = gp->resolution | gp->refresh_rate;
		break;

	default:
		return -1;
	}

	if (ioctl(gp->fd_gs, PS2IOC_SCRTMODE, &crtmode) < 0) {
		perror("ps2_gs_reset, PS2IOC_SSCREENINFO");
		return -1;
	}
	return 1;
}

/* ps2_gs_reset() of gs_reset.c: mode 0x8000 is a local full reset, any other
 * mode goes to the library ps2_gs_reset(). */
static int fwVc_gsReset(int mode, int inter, int omode, int ffmode, int resolution, int refreshRate)
{
	ps2_gs_gparam *gp = ps2_gs_get_gparam();

	switch (mode) {
	case FW_GS_RESET_GIF: {
		int ret;
		struct ps2_pmode pmode;
		unsigned long long csr;
		struct ps2_gssreg r;
		int reg;
		int width, height;
		int interlaceFormat;

		ret = ioctl(gp->fd_gs, PS2IOC_GSRESET, PS2_GSRESET_GIF);
		if (ret < 0) {
			perror("ps2_gs_reset, ioctl PS2_GSRESET_FULL");
			return -1;
		}

		ret = ioctl(gp->fd_gs, PS2IOC_GPMODE, &pmode);
		pmode.sw = 0;
		ret = ioctl(gp->fd_gs, PS2IOC_SPMODE, &pmode);

		interlaceFormat = (omode == PS2_GS_NTSC || omode == PS2_GS_PAL
		    || (omode == PS2_GS_DTV && resolution == PS2_GS_1080I));

		reg = PS2_GSSREG_CSR;
		r.reg = reg;
		/* NOTE: bug: the ioctl gets &reg, not &r, so r.val is never filled. */
		ioctl(gp->fd_gs, PS2IOC_GGSSREG, &reg);
		csr = r.val;
		gp->version = (csr >> 16) & 0xff;

		gp->interlace_mode = inter;
		gp->out_mode = omode;
		gp->ff_mode = ffmode;
		gp->resolution = resolution;
		gp->refresh_rate = refreshRate;
		gp->interlace_format = interlaceFormat;
		width = 640;
		height = 480;

		switch (omode) {
		case PS2_GS_NTSC:
		case PS2_GS_PAL:
			width = 640;
			height = (omode == PS2_GS_NTSC) ? 448 : 512;
			break;

		case PS2_GS_DTV:
			switch (resolution) {
			case PS2_GS_480P:
				width = 720;
				height = 480;
				break;
			case PS2_GS_1080I:
				width = 1920;
				height = 1080;
				break;
			case PS2_GS_720P:
				width = 1024;
				height = 720;
				break;
			}
			break;

		case PS2_GS_VESA:
			switch (resolution) {
			case PS2_GS_640x480:
				width = 640;
				height = 480;
				break;
			case PS2_GS_800x600:
				width = 800;
				height = 600;
				break;
			case PS2_GS_1024x768:
				width = 1024;
				height = 768;
				break;
			case PS2_GS_1280x1024:
				width = 1280;
				height = 1024;
				break;
			}
		}

		if (interlaceFormat && (inter == PS2_GS_NOINTERLACE || ffmode == PS2_GS_FRAME)) {
			height /= 2;
		}

		ps2_gs_set_center(2048, 2048);
		fwVc_setScreenSize(width, height);
		fwVc_setCrtMode();
		break;
	}

	case 0:
	case 1:
	default:
		ps2_gs_reset(0, inter, omode, ffmode, resolution, refreshRate);
		break;
	}

	return 0;
}

/* Opens GS and VPU1 and sets FD_CLOEXEC on both; on error close everything. */
static int fwVc_openDevices(void)
{
	g_fd_gs = fwVc_gsOpen(0);
	g_vpu1 = ps2_vpu_open(1);
	if (g_fd_gs < 0 || g_vpu1 == 0) {
		fwVc_releaseDevice();
		return -1;
	}
	fcntl(g_fd_gs, F_SETFD, FD_CLOEXEC);
	fcntl(ps2_vpu_fd(g_vpu1), F_SETFD, FD_CLOEXEC);
	return g_fd_gs;
}

/* Makes the two buffers kick base, pushes draw+clear for both, runs a first DMA
 * chain, puts the display environment and registers the pair as double buffer. */
int fwGsStartDisplay(int disp, int draw, int flag)
{
	int hDisp = fwGsSetKickBaseDispBuf(disp);
	int hDraw = fwGsSetKickBaseDrawBuf(draw);

	fwGsPushDrawEnv(fwGsGetKickBaseDispBuf());
	fwGsPushClearBuf(fwGsGetKickBaseDispBuf());
	fwGsPushDrawEnv(fwGsGetKickBaseDrawBuf());
	fwGsPushClearBuf(fwGsGetKickBaseDrawBuf());
	fwPkFlipChainBuf();
	fwGsClearDrawBuf(fwGsGetKickBaseDrawBuf());
	fwPkKickChainBuf((void *)1, hDraw);
	fwPkWaitDma();
	fwGsPutDispEnv(hDisp);
	fwGsRegistDBuf(disp, draw, flag);
	fwPkSyncV();
	return 0;
}

/* Translates (mode, inter, scrmode, ffmode) for fwVc_gsReset: scrmode 26..75
 * is VESA, >= 76 is DTV 720P, anything else NTSC; always 60 Hz. */
static int fwGs_resetVideoMode(int mode, int inter, int scrmode, int ffmode)
{
	int omode = PS2_GS_NTSC;
	int res = PS2_GS_640x480;
	int ff = (ffmode == FW_FF_FRAME) ? PS2_GS_FRAME : PS2_GS_FIELD;
	int interlace = (inter == 1);

	if (scrmode >= 26 && scrmode < 76) {
		omode = PS2_GS_VESA;
		switch (scrmode) {
		case 42:
		case 43:
		case 44:
		case 45:
		case 46:
			res = PS2_GS_800x600;
			break;

		case 59:
		case 60:
		case 61:
		case 62:
			res = PS2_GS_1024x768;
			break;

		case 26:
		default:
			res = PS2_GS_640x480;
			break;
		}
	} else if (scrmode >= 76) {
		omode = PS2_GS_DTV;
		res = PS2_GS_720P;
	}

	fwVc_gsReset(mode, interlace, omode, ff, res, PS2_GS_60Hz);
	return 0;
}

int fwGsInitDevice(int mode, int inter, int scrmode, int ffmode)
{
	int result;

	gs_env.ffMode = ffmode;
	gs_env.interlace = inter;
	gs_env.videoMode = scrmode;
	gs_env.multiBuffer = 0;
	gs_env.originX = 2048;
	gs_env.originY = 2048;
	gs_env.oddEven = 0;
	gs_env.unk_44 = 0;

	result = fwVc_openDevices();
	fwGs_resetVideoMode(mode, inter, scrmode, ffmode);
	return result;
}

/* Resets the frame counter, sets ffMode/interlace/videoMode of gs_env from the low byte
 * of mode (FW_GS_*), opens the device and
 * set up the buffer table. */
int fwGsInitAll(int mode)
{
	int hiByte = mode & 0xff00;
	int kind = mode & 0xff;
	int result;

	frame = 0;

	switch (kind) {
	case FW_GS_VESA_1024x768:
		gs_env.ffMode = FW_FF_FIELD;
		gs_env.interlace = 0;
		gs_env.videoMode = 59;
		break;
	case FW_GS_NTSC_FIELD:
		gs_env.ffMode = FW_FF_FIELD;
		gs_env.interlace = 1;
		gs_env.videoMode = 2;
		break;
	case FW_GS_DTV_720P:
		gs_env.ffMode = FW_FF_FRAME;
		gs_env.interlace = 0;
		gs_env.videoMode = 82;
		break;
	case FW_GS_NTSC_FRAME:
	default:
		gs_env.ffMode = FW_FF_FRAME;
		gs_env.interlace = 1;
		gs_env.videoMode = 2;
		break;
	}

	result = fwGsInitDevice(hiByte, gs_env.interlace, gs_env.videoMode, gs_env.ffMode);
	fwGsInitBufEnv();
	return result;
}

/* ps2_gs_close() of gs_open.c without restoring the screen info. */
static int fwGs_closeGsDevice(void)
{
	ps2_gs_gparam *gp = ps2_gs_get_gparam();

	if (gp->fd_gs < 0) {
		return -1;
	}

	if (gp->fd_event_open) {
		if (close(gp->fd_event) < 0) {
			return -1;
		}
		gp->fd_event = -1;
	}

	if (close(gp->fd_gs) < 0) {
		return -1;
	}

	gp->fd_gs = -1;

	return 0;
}

int fwGsReleaseDevice(void)
{
	if (g_vpu1) {
		ps2_vpu_close(g_vpu1);
		g_vpu1 = 0;
	}
	if (g_fd_gs >= 0) {
		fwGs_closeGsDevice();
		g_fd_gs = -1;
	}
	return 0;
}

int fwGsReleaseAll(void)
{
	fwGsReleaseDevice();
	fwGsReleaseBufEnv();
	return 0;
}

/* Arms the framebuffer dump to `file` for the next _fwGsStoreImage_cb; -1 when no
 * double buffer is registered. */
int fwGsStoreImage(const char *file)
{
	int disp = g_fwGsDBufDisp;
	if (disp < 0 || !file)
		return -1;
	g_fwGsStoreImageBuf = disp;
	g_fwGsStoreImageFile = file;
	return 0;
}

/* Framebuffer dump armed by fwGsStoreImage: suspend the periodic flip, read the
 * image back from the GS, write it to the file and restore the flip. */
int _fwGsStoreImage_cb(void)
{
	int disp, draw;
	void *mem;
	int buf, size, fd;

	if (g_fwGsStoreImageBuf < 0)
		return -1;

	fwGsCheckRegDBuf(&disp, &draw);
	fwGsRegistDBuf(-1, -1, 1);
	fwEvReleaseCbfunc(PS2EV_N_VBSTART, (int)&fwGsFlipRegDBuf_cb);
	fwEvReleaseHandle(PS2EV_N_VBSTART);
	fwEvReleaseHandle(PS2EV_N_FINISH);

	mem = malloc(0x12c010);
	buf = ((int)mem + 15) >> 4 << 4;
	size = _fwGsStoreImage(g_fwGsStoreImageBuf, buf);
	g_fwGsStoreImageBuf = -1;
	fd = open(g_fwGsStoreImageFile, O_CREAT | O_RDWR, 0666);
	if (fd > 0) {
		/* NOTE: bug: the file is left open when nothing was read. */
		if (size > 0) {
			write(fd, (const void *)buf, size);
			close(fd);
		}
	}
	free(mem);

	fwEvRegistHandle(PS2EV_N_VBSTART, 0, 0);
	fwEvRegistHandle(PS2EV_N_FINISH, 0, 0);
	fwEvRegistCbfunc(PS2EV_N_VBSTART, (int)&fwGsFlipRegDBuf_cb);
	fwGsRegistDBuf(disp, draw, 1);
	return size;
}

void fwResetCallBack_PreGsSync(void)
{
	preGsSync.count = 0;
}

void fwResetCallBack_PostGsSync(void)
{
	postGsSync.count = 0;
}

void fwResetCallBack_PreDmaKick(void)
{
	preDmaKick.count = 0;
}

void fwInitCallBack_All(void)
{
	preGsSync.count = 0;
	postGsSync.count = 0;
	preDmaKick.count = 0;
}

int fwSetCallBack_PreGsSync(void (*cb)(void))
{
	if (preGsSync.count >= 1024)
		return -1;
	preGsSync.slots[preGsSync.count] = cb;
	preGsSync.count++;
	return 1024 - preGsSync.count;
}

int fwSetCallBack_PostGsSync(void (*cb)(void))
{
	if (postGsSync.count >= 1024)
		return -1;
	postGsSync.slots[postGsSync.count] = cb;
	postGsSync.count++;
	return 1024 - postGsSync.count;
}

int fwSetCallBack_PreDmaKick(void (*cb)(void))
{
	if (preDmaKick.count >= 1024)
		return -1;
	preDmaKick.slots[preDmaKick.count] = cb;
	preDmaKick.count++;
	return 1024 - preDmaKick.count;
}

/* Runs all the callbacks registered on the queue, in order. */
void fwExecCallBack_PreGsSync(void)
{
	int i;
	for (i = 0; i < preGsSync.count; i++)
		preGsSync.slots[i]();
}

void fwExecCallBack_PostGsSync(void)
{
	int i;
	for (i = 0; i < postGsSync.count; i++)
		postGsSync.slots[i]();
}

void fwExecCallBack_PreDmaKick(void)
{
	int i;
	for (i = 0; i < preDmaKick.count; i++)
		preDmaKick.slots[i]();
}
